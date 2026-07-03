#ifndef BYTEURL_LINK_REDiS_H
#define BYTEURL_LINK_REDiS_H

#include <interfaces/i_link_repository.h>
#include <drogon/nosql/RedisClient.h>
#include <optional>
#include <string>
#include <drogon/utils/coroutine.h>

namespace byteurl::redis {
    class LinkRepository : public byteurl::ILinkRepository {
    public:
        explicit LinkRepository(drogon::nosql::RedisClientPtr redisClient);

        drogon::Task<std::optional<std::string>> getOriginalUrl(const std::string &shortCode) override;

        drogon::Task<bool> exists(const std::string &shortCode) override;

        drogon::Task<bool> saveLink(const std::string &shortCode, const std::string &originalUrl) override;

        drogon::Task<void> deleteLink(const std::string &shortCode) override;

        drogon::Task<void> updateLink(const std::string &shortCode, const std::string &newOriginalUrl) override;

        drogon::Task<void> incrementHits(const std::string &shortCode) override;

        drogon::Task<std::optional<std::string>> getShortCodeByOriginalUrl(const std::string &originalUrl) override;

        // Stats are not cached in Redis — always delegates to Postgres.
        drogon::Task<std::optional<LinkStats>> getStats(const std::string &shortCode) override;

    private:
        static constexpr const char *URL_PREFIX = "link:url:";

        static constexpr const char *HITS_PREFIX = "link:hits:";

        static std::string makeUrlKey(const std::string &shortCode);

        static std::string makeHitsKey(const std::string &shortCode);

    private:
        drogon::nosql::RedisClientPtr redisClient_;
    };
} // namespace byteurl::redis

#endif //BYTEURL_LINK_REDiS_H
