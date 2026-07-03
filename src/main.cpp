#include <drogon/drogon.h>
#include <app_factory.h>

int main() {
    byteurl::AppConfig::init();
    drogon::app().loadConfigFile("config.json");
    byteurl::AppFactory::init();
    drogon::app().run();

    return 0;
}