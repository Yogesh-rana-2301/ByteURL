#ifndef BYTEURL_APP_CONFIG_H
#define BYTEURL_APP_CONFIG_H

#include <string>
#include <dotenv.h>
#include <drogon/nosql/RedisClient.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <cstring>
#include <stdexcept>

namespace byteurl {
    class AppConfig {
    private:
        static std::string resolveHostname(const std::string& hostname) {
            struct addrinfo hints, *res;
            std::memset(&hints, 0, sizeof(hints));
            hints.ai_family = AF_UNSPEC; // Support both IPv4 and IPv6
            hints.ai_socktype = SOCK_STREAM;
            
            if (getaddrinfo(hostname.c_str(), nullptr, &hints, &res) != 0) {
                return hostname; 
            }
            
            char ipstr[INET6_ADDRSTRLEN];
            void *addr;
            if (res->ai_family == AF_INET) {
                struct sockaddr_in *ipv4 = (struct sockaddr_in *)res->ai_addr;
                addr = &(ipv4->sin_addr);
            } else {
                struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)res->ai_addr;
                addr = &(ipv6->sin6_addr);
            }
            
            inet_ntop(res->ai_family, addr, ipstr, sizeof(ipstr));
            freeaddrinfo(res);
            return std::string(ipstr);
        }

    public:
        static void init() {
            dotenv::init("../.env");
        }

        static std::string dbName() { return getEnvOrThrow("POSTGRES_DB"); }

        static std::string dbUser() { return getEnvOrThrow("POSTGRES_USER"); }

        static std::string dbPass() { return getEnvOrThrow("POSTGRES_PASSWORD"); }

        static std::string dbHost() { return getEnvOrThrow("POSTGRES_HOST"); }

        static int dbPort() { return std::stoi(getEnvOrThrow("DB_PORT")); }

        // SSL mode for the Postgres connection (e.g. "disable", "require", "prefer").
        // Defaults to "disable" for Railway's internal private network.
        static std::string dbSslMode() {
            const char *val = std::getenv("POSTGRES_SSLMODE");
            return val ? val : "disable";
        }

        // APP_REDIS_HOST / APP_REDIS_PORT are set by entrypoint.sh after parsing
        // REDIS_URL. We use a custom prefix to avoid colliding with Railway's
        // auto-injected REDIS_HOST=0.0.0.0 (the Redis container bind address).
        static std::string redisHost() { return resolveHostname(getEnvOrThrow("APP_REDIS_HOST")); }

        static int redisPort() { return std::stoi(getEnvOrThrow("APP_REDIS_PORT")); }

        static std::string redisPass() {
            const char *val = std::getenv("APP_REDIS_PASS");
            return val ? val : "";
        }

        static std::string listenAddr() { return getEnvOrThrow("LISTEN_ADDR"); }

        static int listenPort() { return std::stoi(getEnvOrThrow("LISTEN_PORT")); }

    private:
        static const char *getEnvOrThrow(const char *name) {
            const char *val = std::getenv(name);
            if (!val) {
                throw std::runtime_error(std::string("Environment variable not set: ") + name);
            }
            return val;
        }
    };
} // namespace byteurl

#endif //BYTEURL_APP_CONFIG_H
