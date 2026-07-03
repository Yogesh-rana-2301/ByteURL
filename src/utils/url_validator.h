#ifndef BYTEURL_URL_VALIDATOR_H
#define BYTEURL_URL_VALIDATOR_H

#include <string>

namespace byteurl::utils {

    /**
     * Lightweight URL validator.
     *
     * Checks performed:
     *  - URL must be non-empty and within the 2048-character limit
     *  - Scheme must be "http" or "https" (blocks javascript:, file:, data:, etc.)
     *  - Host part must be present and contain at least one dot
     */
    class UrlValidator {
    public:
        static constexpr std::size_t MAX_URL_LENGTH = 2048;

        /**
         * Returns true if the given URL is safe to shorten.
         */
        static bool isValid(const std::string &url);

        /**
         * Returns a human-readable error message if isValid() returns false,
         * or an empty string if the URL is valid.
         */
        static std::string errorMessage(const std::string &url);

    private:
        static bool hasValidScheme(const std::string &url);
        static bool hasValidHost(const std::string &url);
    };

} // namespace byteurl::utils

#endif // BYTEURL_URL_VALIDATOR_H
