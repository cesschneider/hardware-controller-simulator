#include "HardwareAPI.h"
#include <curl/curl.h>
#include <iostream>

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