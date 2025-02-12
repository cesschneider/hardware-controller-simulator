#ifndef HARDWAREAPI_H
#define HARDWAREAPI_H

#include <string>
#include <curl/curl.h>
#include "CommandValidator.h"

class HardwareAPI
{
public:
    HardwareAPI(const std::string &baseUrl);
    std::string send(const std::string &endpoint);
    std::string sendWithRetry(const std::string& endpoint);

private:
    std::string baseUrl;
    CommandValidator validator;
    std::string currentState;
    std::string photometricMode;
    bool isVariableInitialized = false;
    void initializeVariables(); 

    static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp);
    long getTimeoutForEndpoint(const std::string& endpoint);
    std::string getValueFromResponse(const std::string& response);
};

#endif // HARDWAREAPI_H