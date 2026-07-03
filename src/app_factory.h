#ifndef BYTEURL_APP_FACTORY_H
#define BYTEURL_APP_FACTORY_H

#include <drogon/drogon.h>
#include <drogon/orm/DbClient.h>
#include <drogon/orm/Exception.h>
#include <drogon/nosql/RedisClient.h>

#include <repositories/postgres/link.h>
#include <repositories/redis/link.h>
#include <services/link_service.h>
#include <services/redirect_service.h>
#include <controllers/link_controller.h>
#include <controllers/health_controller.h>
#include <app_config.h>

namespace byteurl {

    /**
     * AppFactory performs one-time dependency injection at startup.
     *
     * Dependency graph:
     *
     *   DbClient (Postgres)  ──►  postgres::LinkRepository ─┐
     *                                                        ├──► LinkService ──► LinkController
     *   RedisClient          ──►  redis::LinkRepository   ─┤
     *                                                        └──► RedirectService ─►┘
     *
     * All connections are pooled (10 connections each).
     */
    class AppFactory {
    public:
        static void init() {
            initDbClient();
            initRedisClient();
            initRepositories();
            initServices();
            registerControllers();
        }

    private:
        static inline drogon::orm::DbClientPtr        dbClient_;
        static inline drogon::nosql::RedisClientPtr   redisClient_;

        static inline std::shared_ptr<postgres::LinkRepository> postgresRepo_;
        static inline std::shared_ptr<redis::LinkRepository>    redisRepo_;

        static inline std::shared_ptr<service::LinkService>     linkService_;
        static inline std::shared_ptr<service::RedirectService> redirectService_;

    private:
        static void initDbClient() {
            dbClient_ = drogon::orm::DbClient::newPgClient(
                    "dbname="   + AppConfig::dbName()     +
                    " user="    + AppConfig::dbUser()     +
                    " password="+ AppConfig::dbPass()     +
                    " host="    + AppConfig::dbHost()     +
                    " port="    + std::to_string(AppConfig::dbPort()) +
                    " sslmode=" + AppConfig::dbSslMode(),
                    10  // connection pool size
            );

            // Automatically run migrations / create table on startup
            try {
                dbClient_->execSqlSync(
                    "CREATE TABLE IF NOT EXISTS links ("
                    "    short_code   TEXT PRIMARY KEY,"
                    "    original_url TEXT NOT NULL,"
                    "    hits         BIGINT DEFAULT 0,"
                    "    created_at   TIMESTAMPTZ NOT NULL DEFAULT now()"
                    ");"
                );
            } catch (const drogon::orm::DrogonDbException &e) {
                // If it fails (e.g. timeout), it will log. 
            }
        }

        static void initRedisClient() {
            std::string host = AppConfig::redisHost();
            bool isIpv6 = (host.find(':') != std::string::npos);
            redisClient_ = drogon::nosql::RedisClient::newRedisClient(
                    trantor::InetAddress(host, AppConfig::redisPort(), isIpv6),
                    10, // connection pool size
                    AppConfig::redisPass()
            );
        }

        static void initRepositories() {
            postgresRepo_ = std::make_shared<postgres::LinkRepository>(dbClient_);
            redisRepo_    = std::make_shared<redis::LinkRepository>(redisClient_);
        }

        static void initServices() {
            linkService_     = std::make_shared<service::LinkService>(postgresRepo_, redisRepo_);
            redirectService_ = std::make_shared<service::RedirectService>(postgresRepo_, redisRepo_);
        }

        static void registerControllers() {
            LinkController::setServices(linkService_, redirectService_);
            // HealthController has no external dependencies — it is self-contained.
        }
    };

} // namespace byteurl

#endif // BYTEURL_APP_FACTORY_H
