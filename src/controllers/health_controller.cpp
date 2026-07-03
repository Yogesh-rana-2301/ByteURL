#include <controllers/health_controller.h>

drogon::Task<drogon::HttpResponsePtr> HealthController::health(drogon::HttpRequestPtr req) {
    Json::Value resp;
    resp["status"]  = "ok";
    resp["version"] = "1.0.0";
    co_return drogon::HttpResponse::newHttpJsonResponse(resp);
}
