#pragma once

#include <HTTPClient.h>

class HttpRequestManager
{
private:
    HTTPClient http;

    String PrepareQueryParameters(std::vector<std::pair<String, String>>& params);

public:
    HttpRequestManager() = default;
    ~HttpRequestManager() = default;

    String Get(String url, std::vector<std::pair<String, String>> params = {}, std::pair<String, String> basicAuthentication = { "", "" });
};