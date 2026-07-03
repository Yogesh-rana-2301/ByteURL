#ifndef BYTEURL_I_LINK_REPOSITORY_H
#define BYTEURL_I_LINK_REPOSITORY_H

#include <optional>
#include <string>
#include <drogon/utils/coroutine.h>

namespace byteurl {

    /**
     * Value object returned by getStats().
     * Bundles all analytics data for a single short link.
     */
    struct LinkStats {
        std::string shortCode;
        std::string originalUrl;
        long long   hits{0};
        std::string createdAt; // ISO-8601 timestamp string
    };

    /**
     * Pure-virtual repository interface for link storage.
     *
     * Two concrete implementations exist:
     *   - postgres::LinkRepository  (durable, source-of-truth)
     *   - redis::LinkRepository     (fast in-memory cache)
     *
     * All operations are coroutine-based (co_await compatible).
     */
    class ILinkRepository {
    public:
        virtual ~ILinkRepository() = default;

        virtual drogon::Task<std::optional<std::string>>
        getOriginalUrl(const std::string &shortCode) = 0;

        virtual drogon::Task<bool>
        saveLink(const std::string &shortCode, const std::string &originalUrl) = 0;

        virtual drogon::Task<void>
        deleteLink(const std::string &shortCode) = 0;

        virtual drogon::Task<bool>
        exists(const std::string &shortCode) = 0;

        virtual drogon::Task<void>
        updateLink(const std::string &shortCode, const std::string &newOriginalUrl) = 0;

        virtual drogon::Task<void>
        incrementHits(const std::string &shortCode) = 0;

        virtual drogon::Task<std::optional<std::string>>
        getShortCodeByOriginalUrl(const std::string &originalUrl) = 0;

        /**
         * Returns full stats for a short code, or nullopt if not found.
         * Only the Postgres implementation provides real data;
         * the Redis implementation always returns nullopt (stats are not cached).
         */
        virtual drogon::Task<std::optional<LinkStats>>
        getStats(const std::string &shortCode) = 0;
    };

} // namespace byteurl

#endif // BYTEURL_I_LINK_REPOSITORY_H
