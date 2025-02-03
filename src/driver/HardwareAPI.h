#ifndef HARDWAREAPI_H
#define HARDWAREAPI_H

#include <string>

class HardwareAPI
{
public:
    HardwareAPI(const std::string &baseUrl);
    std::string send(const std::string &endpoint);

private:
    static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp);
    std::string baseUrl;
};

#endif // HARDWAREAPI_H