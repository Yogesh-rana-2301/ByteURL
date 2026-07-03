#include <controllers/link_controller.h>
#include <utils/url_validator.h>

std::shared_ptr<byteurl::service::LinkService>     LinkController::linkService_     = nullptr;
std::shared_ptr<byteurl::service::RedirectService> LinkController::redirectService_ = nullptr;

void LinkController::setServices(std::shared_ptr<byteurl::service::LinkService>    link,
                                 std::shared_ptr<byteurl::service::RedirectService> redirect) {
    linkService_     = std::move(link);
    redirectService_ = std::move(redirect);
}

// POST /shorten
// Body: { "url": "<long-url>" }
// Response: { "shortCode": "r/<code>" }  or  { "error": "<message>" } on bad input
drogon::Task<drogon::HttpResponsePtr> LinkController::createShortLink(
        drogon::HttpRequestPtr req) {

    auto jsonReq = req->getJsonObject();
    if (!jsonReq || !(*jsonReq)["url"].isString()) {
        auto resp = drogon::HttpResponse::newHttpJsonResponse(
                Json::Value{{"error", "Request body must be JSON with a string 'url' field."}}
        );
        resp->setStatusCode(drogon::k400BadRequest);
        co_return resp;
    }

    const std::string originalUrl = (*jsonReq)["url"].asString();

    // Validate the URL before doing any DB work
    if (!byteurl::utils::UrlValidator::isValid(originalUrl)) {
        Json::Value errBody;
        errBody["error"] = byteurl::utils::UrlValidator::errorMessage(originalUrl);
        auto resp = drogon::HttpResponse::newHttpJsonResponse(errBody);
        resp->setStatusCode(drogon::k400BadRequest);
        co_return resp;
    }

    const std::string shortCode = co_await linkService_->createShortLink(originalUrl);

    Json::Value jsonResp;
    jsonResp["shortCode"] = "r/" + shortCode;
    co_return drogon::HttpResponse::newHttpJsonResponse(jsonResp);
}

// GET /r/{hash}
// Redirects to the original URL (302) or returns 404 if not found.
// Also increments the hit counter so analytics stay accurate.
drogon::Task<drogon::HttpResponsePtr> LinkController::redirectToOriginal(
        drogon::HttpRequestPtr req,
        const std::string &hash) {

    auto urlOpt = co_await redirectService_->getOriginalUrl(hash);

    if (!urlOpt.has_value()) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k404NotFound);
        resp->setBody("Short link not found.");
        co_return resp;
    }

    // BUG FIX: increment the hit counter on every successful redirect.
    // incrementHits was implemented in both repositories but was never called here.
    co_await redirectService_->incrementHits(hash);

    auto resp = drogon::HttpResponse::newHttpResponse();
    resp->setStatusCode(drogon::k302Found);
    resp->addHeader("Location", *urlOpt);
    co_return resp;
}

// GET /stats/{hash}
// Response: { "shortCode", "originalUrl", "hits", "createdAt" }  or  404
drogon::Task<drogon::HttpResponsePtr> LinkController::getStats(
        drogon::HttpRequestPtr req,
        const std::string &hash) {

    auto statsOpt = co_await linkService_->getLinkStats(hash);

    if (!statsOpt.has_value()) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k404NotFound);
        resp->setBody("Short link not found.");
        co_return resp;
    }

    const auto &stats = *statsOpt;
    Json::Value jsonResp;
    jsonResp["shortCode"]   = stats.shortCode;
    jsonResp["originalUrl"] = stats.originalUrl;
    jsonResp["hits"]        = static_cast<Json::Int64>(stats.hits);
    jsonResp["createdAt"]   = stats.createdAt;
    co_return drogon::HttpResponse::newHttpJsonResponse(jsonResp);
}
