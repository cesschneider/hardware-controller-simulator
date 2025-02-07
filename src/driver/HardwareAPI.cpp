#include "HardwareAPI.h"
#include <curl/curl.h>
#include <iostream>
#include <set>

// Track Hardware state
std::string hardwareState = "idle";

// Define the list of valid commands
std::set<std::string> validCommands = {
    "ping",
    "reset",
    "trigger",
    "get_frame",
    "get_state",
    "set_state=idle",
    "set_state=config",
    "get_config=focus",
    "set_config=focus:",
    "get_config=gain",
    "set_config=gain:",
    "get_config=exposure",
    "set_config=exposure:",
    "get_config=led_pattern",
    "set_config=led_pattern:",
    "get_config=led_intensity",
    "set_config=led_intensity:",
    "get_config=photometric_mode"
    "set_config=photometric_mode:"
};

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

bool HardwareAPI::isValidParameter(const std::string& param, const std::string& value) {
    if (param == "/set_config=exposure:") {
        int exposure = std::stoi(value);
        return exposure >= 0 && exposure <= 1000; // Example range for exposure
    }
    if (param == "/set_config=led_pattern:") {
        return value == "a" || value == "b" || value == "c"; // Example valid patterns
    }
    return false;
}

bool HardwareAPI::isValidCommand(const std::string& command) {
    // Check if the command is in the validCommands set
    if (validCommands.find(command) != validCommands.end()) {
        return true;
    }

    // Handle commands with dynamic parameters (e.g., /set_config=exposure:100)
    if (command.find("/set_config=") == 0) {
        size_t colonPos = command.find(':');
        if (colonPos != std::string::npos) {
            std::string param = command.substr(0, colonPos + 1); // Include the colon
            std::string value = command.substr(colonPos + 1);

            if (validCommands.find(param) != validCommands.end()) {
                return isValidParameter(param, value);
            }
        }
    }

    return false;
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
    if (endpoint == "/reset") {
        return 2000;
    } else if (endpoint == "/trigger") {
        return 4500;
    } else {
        // Default timeout for other endpoints
        return 150;
    }
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
    unsigned timeout = 100;

    if  (! isValidCommand(endpoint)) {
        fprintf(stderr, "[HardwareAPI] invalid endpoint: %s\n", endpoint.c_str());
        return "validation_error";
    }

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        long timeout_ms = getTimeoutForEndpoint(endpoint);
        CURLcode res = performRequestWithTimeout(curl, timeout);
        if (res != CURLE_OK) {
            fprintf(stderr, "[HardwareAPI] curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            return "timeout_error";
        }

        curl_easy_cleanup(curl);
    }

    std::cout << "[HardwareAPI] << " << readBuffer << std::endl;
    return readBuffer;
}