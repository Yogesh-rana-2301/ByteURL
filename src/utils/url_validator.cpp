#include <utils/url_validator.h>
#include <algorithm>
#include <cctype>

namespace byteurl::utils {

namespace {
    // Schemes that are explicitly blocked — these can be used for XSS / SSRF attacks.
    constexpr const char* BLOCKED_SCHEMES[] = {
        "javascript:", "data:", "file:", "vbscript:", "about:"
    };
}

bool UrlValidator::hasValidScheme(const std::string &url) {
    // Convert prefix to lowercase for case-insensitive comparison
    std::string lower = url.substr(0, std::min(url.size(), std::size_t(16)));
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    // Explicitly block dangerous schemes first
    for (const auto *scheme : BLOCKED_SCHEMES) {
        if (lower.find(scheme) == 0) {
            return false;
        }
    }

    // Only allow http:// and https://
    return lower.find("http://") == 0 || lower.find("https://") == 0;
}

bool UrlValidator::hasValidHost(const std::string &url) {
    // Find the start of the host (after "http://" or "https://")
    std::size_t schemeEnd = url.find("://");
    if (schemeEnd == std::string::npos) return false;

    std::size_t hostStart = schemeEnd + 3;
    if (hostStart >= url.size()) return false;

    // Extract host part (everything up to the next '/', '?', '#', or ':')
    std::size_t hostEnd = url.find_first_of("/?#:", hostStart);
    std::string host = url.substr(hostStart, hostEnd == std::string::npos
                                              ? std::string::npos
                                              : hostEnd - hostStart);

    if (host.empty()) return false;

    // Host must contain at least one dot (rejects bare labels like "localhost" in prod)
    // and must not start or end with a dot or hyphen
    if (host.find('.') == std::string::npos) return false;
    if (host.front() == '.' || host.front() == '-') return false;
    if (host.back()  == '.' || host.back()  == '-') return false;

    return true;
}

bool UrlValidator::isValid(const std::string &url) {
    if (url.empty() || url.size() > MAX_URL_LENGTH) return false;
    if (!hasValidScheme(url)) return false;
    if (!hasValidHost(url))   return false;
    return true;
}

std::string UrlValidator::errorMessage(const std::string &url) {
    if (url.empty()) {
        return "URL must not be empty.";
    }
    if (url.size() > MAX_URL_LENGTH) {
        return "URL exceeds maximum allowed length of 2048 characters.";
    }
    if (!hasValidScheme(url)) {
        return "URL must start with http:// or https://. Schemes like javascript:, file:, and data: are not allowed.";
    }
    if (!hasValidHost(url)) {
        return "URL must contain a valid hostname (e.g. example.com).";
    }
    return {};
}

} // namespace byteurl::utils
