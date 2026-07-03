#ifndef BYTEURL_REDIRECT_SERVICE_H
#define BYTEURL_REDIRECT_SERVICE_H

#include <interfaces/i_link_repository.h>

namespace byteurl::service {
    class RedirectService {
    public:
        explicit RedirectService(std::shared_ptr<byteurl::ILinkRepository> postgresRepo,
                                 std::shared_ptr<byteurl::ILinkRepository> redisRepo);

        drogon::Task<std::optional<std::string> > getOriginalUrl(const std::string &shortCode);

        drogon::Task<void> incrementHits(const std::string &shortCode);

    private:
        std::shared_ptr<ILinkRepository> redisRepo_;
        std::shared_ptr<ILinkRepository> postgresRepo_;
    };
} // namespace byteurl::service

#endif //BYTEURL_REDIRECT_SERVICE_H
