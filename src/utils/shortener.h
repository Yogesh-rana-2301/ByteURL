#ifndef BYTEURL_SHORTENER_H
#define BYTEURL_SHORTENER_H

#include <string>

namespace byteurl::utils {
    class Shortener {
    public:
        static std::string generate();
    };
} // namespace utils

#endif //BYTEURL_SHORTENER_H
