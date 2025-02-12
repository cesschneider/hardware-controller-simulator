#include "HardwareAPI.h"
#include "CommandValidator.h"
#include <curl/curl.h>

#define REQUEST_RETRIES 3

HardwareAPI::HardwareAPI(const std::string &baseUrl)
{
    this->baseUrl = baseUrl;
    CommandValidator validator;
}

// Initialize Hardware tracking variables
void HardwareAPI::initializeVariables() 
{
    fprintf(stdout, "[HardwareAPI] Initializing current hardware variables\n");

    fprintf(stdout, "[HardwareAPI] initializeVariables(): Setting state to 'config'\n");
    this->send("set_state=config");
    validator.setConfig("photometric_mode", this->getValueFromResponse(this->send("get_config=photometric_mode")));
    validator.setConfig("led_pattern", this->getValueFromResponse(this->send("get_config=led_pattern")));

    fprintf(stdout, "[HardwareAPI] initializeVariables(): Setting state to 'idle'\n");
    this->send("set_state=idle");
    this->currentState = this->getValueFromResponse(this->send("get_state"));
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

std::string HardwareAPI::send(const std::string &command)
{
    CURL *curl;
    CURLcode res;
    std::string readBuffer;
    std::string url = baseUrl + command;

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

The value of this mode impacts in the **Hardware** performance in the following manners:

- `photometric_mode:0`
    - Expected inspection time is between 2.0s and 3.0s 
    *(time between the trigger being acknowledged and the image frame buffer becoming available)*
- `photometric_mode:1`
    - Expected inspection time is between 3.5s and 4.5s 
    *(time between the trigger being acknowledged and the image frame buffer becoming available)*
 */

long HardwareAPI::getTimeoutForEndpoint(const std::string& endpoint) {
    if (endpoint == "reset") {
        return 2000;
    } else if (endpoint == "trigger") {
        if (this->photometricMode == "0") {
            return 3000;
        } else if (this->photometricMode == "1") {
            return 4500;
        }
    } 
    // Default timeout for other commands
    return 150;
}

// Handle errors and timeouts
std::string HardwareAPI::sendWithRetry(const std::string& endpoint) {
    CURL* curl = curl_easy_init();
    CURLcode res;
    std::string readBuffer;
    std::string url = baseUrl + endpoint;

    // Add timeout handling for Hardware requests
    long timeoutMs = getTimeoutForEndpoint(endpoint);

    // Initialize Hardware tracking variables
    if (! this->isVariableInitialized) {
        this->initializeVariables();
        this->isVariableInitialized = true;
    }
    
    if  (! validator.validateEndpoint(endpoint)) {
        fprintf(stderr, "[HardwareAPI] Invalid endpoint or parameter: %s\n", endpoint.c_str());
        return "validation_err";
    }

    if  (! validator.validateOperation(endpoint)) {
        fprintf(stderr, "[HardwareAPI] Illegal operation for current state or parameters: %s\n", endpoint.c_str());
        return "operation_err";
    }

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
}