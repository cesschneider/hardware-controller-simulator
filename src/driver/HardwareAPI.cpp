#include "HardwareAPI.h"
#include "CommandValidator.h"
#include <curl/curl.h>
#include <string>

#define REQUEST_RETRIES 3

HardwareAPI::HardwareAPI(const std::string &baseUrl)
{
    this->baseUrl = baseUrl;
    CommandValidator validator;
}

void HardwareAPI::refreshConfig() 
{
    std::cout << "[HardwareAPI] Refresing hardware configuration parameters" << std::endl;

    char endpoint[50];
    std::string response;
    std::unordered_set<std::string> configList = {
        "focus", "exposure", "gain", "led_pattern", "led_intensity", "photometric_mode"
    };

    memset(endpoint, 0, sizeof(endpoint));
    send("set_state=config");

    for (const std::string& config : configList) {
        std::cout << "[HardwareAPI] refreshConfig(): Getting value for parameter: " << config << std::endl;

        snprintf(endpoint, sizeof(endpoint), "get_config=%s", config.c_str());
        response = send(endpoint);

        if (response == "error" || response == "timeout_err") {
            std::cerr << "[HardwareAPI] refreshConfig(): Invalid response from hardware: " << response << std::endl;
            send("reset");
        } else {
            validator.setConfig(config, getValueFromResponse(response));
        }
    }

    send("set_state=idle");
}

std::string HardwareAPI::getValueFromResponse(const std::string& input) {
    size_t pos = input.find(':'); 
    if (pos != std::string::npos && pos + 1 < input.length()) {
        return input.substr(pos + 1); 
    }
    return "";
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

    std::cout << "[HardwareAPI] Sending command to hardware: " << endpoint << std::endl;

    // Add timeout handling for Hardware requests
    long timeoutMs = getTimeoutForEndpoint(endpoint);
    std::cout << "[HardwareAPI] Setting timeout to: " << std::to_string(timeoutMs) << std::endl;

    curl = curl_easy_init();
    if (curl)
    {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, timeoutMs);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK)
        {
            fprintf(stderr, "[HardwareAPI] curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            return "timeout_err";
        }

        curl_easy_cleanup(curl);
    }

    std::cout << "[HardwareAPI] send() readBuffer: " << readBuffer << std::endl;

    return readBuffer;
}

/**
 - `/ping`, `/trigger`, `/get_frame`, `/get_state`, `/set_state={STATE}`, `/get_config={PARAMETER}`, `/set_config={PARAMETER}:{VALUE}`.
    - 150ms.
- `/reset`.
    - 2000ms.
- Time between a `trigger_ack` and the **image frame buffer** becoming availabe:
    - Between 1000ms and 4500ms depending on `photometric_mode`.

The value of this mode impacts in the **Hardware** performance in the following manners:

- `photometric_mode:0`
    - Expected inspection time is between 2.0s and 3.0s 
    *(time between the trigger being acknowledged and the image frame buffer becoming available)*
- `photometric_mode:1`
    - Expected inspection time is between 3.5s and 4.5s 
    *(time between the trigger being acknowledged and the image frame buffer becoming available)*
 */

long HardwareAPI::getTimeoutForEndpoint(const std::string& endpoint) {
    std::cout << "[HardwareAPI] Getting timeout for endpoint: " << endpoint.c_str() << std::endl;

    if (endpoint == "reset") {
        return 2000;
    } else if (endpoint == "trigger") {
        if (validator.getConfig("photometric_mode") == "0") {
            return 3000;
        } else if (validator.getConfig("photometric_mode") == "1") {
            return 4500;
        }
    } 
    // Default timeout for other endpoints
    return 150;
}

// Handle errors and timeouts
std::string HardwareAPI::sendWithRetry(const std::string& endpoint) {
    /*
    CURL* curl = curl_easy_init();
    CURLcode res;
    std::string readBuffer;
    std::string url = baseUrl + endpoint;
    */

    // Initialize Hardware tracking variables
    if (! this->isConfigRefreshed) {
        this->refreshConfig();
        this->isConfigRefreshed = true;
    }
    
    if  (! validator.validateEndpoint(endpoint)) {
        fprintf(stderr, "[HardwareAPI] sendWithRetry(): Invalid endpoint or parameter: %s\n", endpoint.c_str());
        return "validation_err";
    }

    if  (! validator.validateOperation(endpoint)) {
        fprintf(stderr, "[HardwareAPI] sendWithRetry(): Illegal operation for state '%s': %s\n", 
            validator.getState().c_str(), endpoint.c_str());
        return "operation_err";
    }

    std::string response;
    for (int retries = 1; retries <= REQUEST_RETRIES; retries++) {
        fprintf(stdout, "[HardwareAPI] sendWithRetry(): Attempting to perform request #%i\n", retries);
        response = send(endpoint);

        if (response == "error" || response == "timeout_err") {
            fprintf(stderr, "[HardwareAPI] sendWithRetry(): Invalid response: %s\n", response.c_str());
            fprintf(stderr, "[HardwareAPI] sendWithRetry(): Sending reset command to hardware\n");
            send("reset");
            refreshConfig();
        } else {
            break;
        }

    }

    // Avoid duplicated get_state command requests
    if (endpoint != "get_state") {
        validator.setState(getValueFromResponse(send("get_state")));
    }

    return response;

    /*
    // Add timeout handling for Hardware requests
    long timeoutMs = getTimeoutForEndpoint(endpoint);

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, timeoutMs);

        for (int retries = 1; retries <= REQUEST_RETRIES; retries++) {
            fprintf(stdout, "[HardwareAPI] Attempting to perform request #%i\n", retries);
            res = curl_easy_perform(curl);
            if (res != CURLE_OK) {
                fprintf(stderr, "[HardwareAPI] curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
                this->send("reset");
            } else {
                break;
            }
        }

        if (res != CURLE_OK) {
            return "timeout_err";
        }

        curl_easy_cleanup(curl);
    }

    // Handle variables that should be tracked across requests
    if (endpoint.rfind("set_state", 0)) {
        std::string currentState = this->getValueFromResponse(this->send("get_state"));
        validator.setState(currentState);
        fprintf(stdout, "[HardwareAPI] sendWithRetry(): Updated currentState to '%s'\n", currentState.c_str());
    }
    if (endpoint.rfind("set_config", 0) == 0) {
        validator.setConfigFromEndpoint(endpoint);
    }

    std::cout << "[HardwareAPI] << " << readBuffer << std::endl;
    return readBuffer;
    */
}