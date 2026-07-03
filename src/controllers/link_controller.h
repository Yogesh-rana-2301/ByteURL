#ifndef BYTEURL_LINK_CONTROLLER_H
#define BYTEURL_LINK_CONTROLLER_H

#include <drogon/HttpController.h>
#include <services/link_service.h>
#include <services/redirect_service.h>

/**
 * LinkController handles all link-related HTTP endpoints:
 *
 *   POST /shorten          — shorten a URL, returns { "shortCode": "r/XxXxXx" }
 *   GET  /r/{hash}         — redirect to original URL (302), increments hit counter
 *   GET  /stats/{hash}     — return analytics: { "shortCode", "originalUrl", "hits", "createdAt" }
 */
class LinkController : public drogon::HttpController<LinkController> {
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(LinkController::createShortLink,    "/shorten",       drogon::Post);
        ADD_METHOD_TO(LinkController::redirectToOriginal, "/r/{hash}",      drogon::Get);
        ADD_METHOD_TO(LinkController::getStats,           "/stats/{hash}",  drogon::Get);
    METHOD_LIST_END

    static void setServices(std::shared_ptr<byteurl::service::LinkService>    link,
                            std::shared_ptr<byteurl::service::RedirectService> redirect);

    static drogon::Task<drogon::HttpResponsePtr> createShortLink(drogon::HttpRequestPtr req);

    static drogon::Task<drogon::HttpResponsePtr> redirectToOriginal(drogon::HttpRequestPtr req,
                                                                    const std::string &hash);

    static drogon::Task<drogon::HttpResponsePtr> getStats(drogon::HttpRequestPtr req,
                                                          const std::string &hash);

private:
    static std::shared_ptr<byteurl::service::LinkService>     linkService_;
    static std::shared_ptr<byteurl::service::RedirectService> redirectService_;
};

#endif // BYTEURL_LINK_CONTROLLER_H
