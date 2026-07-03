#include <services/redirect_service.h>

namespace byteurl::service {
    RedirectService::RedirectService(std::shared_ptr<byteurl::ILinkRepository> postgresRepo,
                                     std::shared_ptr<byteurl::ILinkRepository> redisRepo)
            : postgresRepo_(std::move(postgresRepo)), redisRepo_(std::move(redisRepo)) {}

    drogon::Task<std::optional<std::string> > RedirectService::getOriginalUrl(const std::string &shortCode) {
        auto cachedUrl = co_await redisRepo_->getOriginalUrl(shortCode);
        if (cachedUrl.has_value()) {
            co_return cachedUrl;
        }

        auto url = co_await postgresRepo_->getOriginalUrl(shortCode);
        if (url.has_value()) {
            co_await redisRepo_->saveLink(shortCode, *url);
        }

        co_return url;
    }

    drogon::Task<void> RedirectService::incrementHits(const std::string &shortCode) {
        co_await postgresRepo_->incrementHits(shortCode);

        co_await redisRepo_->incrementHits(shortCode);

        co_return;
    }

} // namespace byteurl::service