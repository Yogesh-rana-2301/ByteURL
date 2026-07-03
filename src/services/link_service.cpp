#include <services/link_service.h>
#include <utils/shortener.h>

namespace byteurl::service {
    LinkService::LinkService(std::shared_ptr<byteurl::ILinkRepository> postgresRepo,
                             std::shared_ptr<byteurl::ILinkRepository> redisRepo)
            : postgresRepo_(std::move(postgresRepo)), redisRepo_(std::move(redisRepo)) {}

    drogon::Task<std::string> LinkService::createShortLink(const std::string &originalUrl) {
        auto cachedCode = co_await redisRepo_->getShortCodeByOriginalUrl(originalUrl);
        if (cachedCode.has_value()) {
            co_return *cachedCode;
        }

        auto existingCode = co_await postgresRepo_->getShortCodeByOriginalUrl(originalUrl);
        if (existingCode.has_value()) {
            co_await redisRepo_->saveLink(*existingCode, originalUrl);
            co_return *existingCode;
        }

        std::string shortCode;
        bool exists = true;
        while (exists) {
            shortCode = utils::Shortener::generate();
            auto cached = co_await redisRepo_->getOriginalUrl(shortCode);
            if (cached.has_value()) {
                continue;
            }
            exists = co_await postgresRepo_->exists(shortCode);
        }

        co_await postgresRepo_->saveLink(shortCode, originalUrl);
        co_await redisRepo_->saveLink(shortCode, originalUrl);

        co_return shortCode;
    }

    drogon::Task<bool> LinkService::shortLinkExists(const std::string &originalUrl) {
        auto cachedCode = co_await redisRepo_->getShortCodeByOriginalUrl(originalUrl);
        if (cachedCode.has_value()) {
            co_return true;
        }

        auto existing = co_await postgresRepo_->getShortCodeByOriginalUrl(originalUrl);
        co_return existing.has_value();
    }

    drogon::Task<std::optional<LinkStats>> LinkService::getLinkStats(const std::string &shortCode) {
        // Stats (hits, created_at) are the source of truth in Postgres.
        // Redis does not cache these values, so we query Postgres directly.
        co_return co_await postgresRepo_->getStats(shortCode);
    }

} // namespace byteurl::service