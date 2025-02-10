#include "HardwareAPI.h"
#include "CommandValidator.h"
#include <curl/curl.h>

// Track Hardware state
std::string hardwareState = "idle";

HardwareAPI::HardwareAPI(const std::string &baseUrl)
{
    this->baseUrl = baseUrl;
}

size_t HardwareAPI::WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string *)userp)->append((char *)contents, size * nmemb);
    return size * nmemb;
}

std::string HardwareAPI::send(const std::string &endpoint)
{
    CURL *curl;
    CURLcode res;
    std::string readBuffer;
    std::string url = baseUrl + endpoint;

    curl = curl_easy_init();
    if (curl)
    {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        res = curl_easy_perform(curl);

        if (res != CURLE_OK)
        {
            fprintf(stderr, "[HardwareAPI] curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }

        curl_easy_cleanup(curl);
    }

    std::cout << "[HardwareAPI] << " << readBuffer << std::endl;

    return readBuffer;
}

/**
 - `/ping`, `/trigger`, `/get_frame`, `/get_state`, `/set_state={STATE}`, `/get_config={PARAMETER}`, `/set_config={PARAMETER}:{VALUE}`.
    - 150ms.
- `/reset`.
    - 2000ms.
- Time between a `trigger_ack` and the **image frame buffer** becoming availabe:
    - Between 1000ms and 4500ms depending on `photometric_mode`.
 */

long HardwareAPI::getTimeoutForEndpoint(const std::string& endpoint) {
    if (endpoint == "reset") {
        return 2000;
    } else if (endpoint == "trigger") {
        return 4500;
    } else {
        // Default timeout for other endpoints
        return 150;
    }
    // TODO: handle special edge cases for trigger command
}

// Add timeout handling for Hardware requests
CURLcode HardwareAPI::performRequestWithTimeout(CURL* curl, long timeout_ms) {
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, timeout_ms);
    return curl_easy_perform(curl);
}

// Handle errors and timeouts
std::string HardwareAPI::sendCommandWithRetry(const std::string& endpoint) {
    CURL* curl = curl_easy_init();
    std::string readBuffer;
    std::string url = baseUrl + endpoint;

    CommandValidator validator;
    if  (! validator.validateCommand(endpoint)) {
        fprintf(stderr, "[HardwareAPI] invalid endpoint: %s\n", endpoint.c_str());
        return "validation_error";
    }

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        long timeout_ms = getTimeoutForEndpoint(endpoint);
        CURLcode res = performRequestWithTimeout(curl, timeout_ms);
        if (res != CURLE_OK) {
            fprintf(stderr, "[HardwareAPI] curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            return "timeout_error";
        }

        curl_easy_cleanup(curl);
    }

    std::cout << "[HardwareAPI] << " << readBuffer << std::endl;
    return readBuffer;
}