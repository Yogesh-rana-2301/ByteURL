#ifndef BYTEURL_HEALTH_CONTROLLER_H
#define BYTEURL_HEALTH_CONTROLLER_H

#include <drogon/HttpController.h>

/**
 * HealthController exposes a single liveness endpoint:
 *
 *   GET /health  →  200 OK  { "status": "ok", "version": "1.0.0" }
 *
 * This is the standard pattern for Docker HEALTHCHECK directives and
 * Kubernetes liveness / readiness probes.
 */
class HealthController : public drogon::HttpController<HealthController> {
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(HealthController::health, "/health", drogon::Get);
    METHOD_LIST_END

    static drogon::Task<drogon::HttpResponsePtr> health(drogon::HttpRequestPtr req);
};

#endif // BYTEURL_HEALTH_CONTROLLER_H
