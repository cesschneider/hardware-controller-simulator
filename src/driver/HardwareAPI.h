#ifndef HARDWAREAPI_H
#define HARDWAREAPI_H

#include <string>
#include <curl/curl.h>

class HardwareAPI
{
public:
    HardwareAPI(const std::string &baseUrl);
    std::string send(const std::string &endpoint);
    std::string sendCommandWithRetry(const std::string& command);

private:
    static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp);
    std::string baseUrl;
    bool isValidParameter(const std::string& param, const std::string& value);
    bool isValidCommand(const std::string& command);
    long getTimeoutForEndpoint(const std::string& endpoint);
    CURLcode performRequestWithTimeout(CURL* curl, long timeout_ms);
};

#endif // HARDWAREAPI_H