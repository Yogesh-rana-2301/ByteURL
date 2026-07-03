#ifndef BYTEURL_LINK_SERVICE_H
#define BYTEURL_LINK_SERVICE_H

#include <interfaces/i_link_repository.h>
#include <drogon/utils/coroutine.h>

namespace byteurl::service {

    class LinkService {
    public:
        explicit LinkService(std::shared_ptr<byteurl::ILinkRepository> postgresRepo,
                             std::shared_ptr<byteurl::ILinkRepository> redisRepo);

        /**
         * Creates or retrieves a short code for the given URL.
         * Checks Redis cache first, then Postgres, then generates a new code.
         */
        drogon::Task<std::string> createShortLink(const std::string &originalUrl);

        /**
         * Returns true if a short link already exists for the given original URL.
         */
        drogon::Task<bool> shortLinkExists(const std::string &originalUrl);

        /**
         * Returns statistics (hits, original URL, created_at) for a given short code.
         * Queries Postgres directly — stats are not cached in Redis.
         * Returns nullopt if the short code does not exist.
         */
        drogon::Task<std::optional<LinkStats>> getLinkStats(const std::string &shortCode);

    private:
        std::shared_ptr<ILinkRepository> redisRepo_;
        std::shared_ptr<ILinkRepository> postgresRepo_;
    };

} // namespace byteurl::service

#endif // BYTEURL_LINK_SERVICE_H
