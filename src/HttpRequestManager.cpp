#include "HttpRequestManager.h"

#include <sstream>

String HttpRequestManager::PrepareQueryParameters(std::vector<std::pair<String, String>>& params)
{
    if (params.empty())
        return {};

    std::stringstream queryStream("?");

    bool first = true;
    for (const auto& [key, value] : params)
    {
        if (!first)
            queryStream << "&";

        queryStream << key << "=" << value;
        first = false;
    }

    return queryStream.str().c_str();
}

String HttpRequestManager::Get(String url, std::vector<std::pair<String, String>> params, std::pair<String, String> basicAuthentication) {
    http.begin(url);

    // Use basic authentication if provided
    if (basicAuthentication.first.length() && basicAuthentication.second.length()) {
        http.setAuthorization(basicAuthentication.first.c_str(), basicAuthentication.second.c_str());
    }

    // todo: query params, connect to opensky

    int responseCode = http.GET();
    String response = "";

    if (responseCode > 0) {
        Serial.print("Response code: ");
        Serial.println(responseCode);
        response = http.getString();
    }
    else {
        Serial.print("Error: ");
        Serial.println(http.errorToString(responseCode));
    }

    http.end();
    return response;
}
