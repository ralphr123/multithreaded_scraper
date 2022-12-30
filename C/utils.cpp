#include "Utils.hpp"

/// @brief Callback to write HTML response body to buffer
/// @param ptr Pointer to the buffer
/// @param size Fragment size in bytes
/// @param nmemb Number of fragments
/// @param str Pointer to data
/// @return 
size_t Utils::write_curl_data(void* ptr, size_t size, size_t nmemb, void* str) {
    std::string *s = static_cast<std::string *>(str);
    std::copy((char *) ptr, (char *) ptr + (size + nmemb), std::back_inserter(*s));
    return size * nmemb;
}

/// @brief Print contents of C++ vector to terminal for debugging
/// @param vec 
void Utils::print_vector(const std::vector<std::string_view> vec) {
    for (std::string_view str : vec) {
        std::cout << str << std::endl;
    }
}

/// @brief Remove duplicates from C++ vector (list)
/// @param vec 
void Utils::remove_duplicates(std::vector<std::string_view> &vec) {
    vec.erase( unique( vec.begin(), vec.end() ), vec.end() );
}

/// @param curl Curl handle
/// @param url URL
/// @return Raw HTML contents of page
std::string Utils::curl_get_req(CURL *curl, const std::string_view url) {
    if (!curl) return "invalid";

    curl_easy_setopt(curl, CURLOPT_URL, std::string(url).c_str());

    // Allow 50 redirects
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 50L);

    // Write callback to response string
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_curl_data);

    std::string response_string;
    std::string header_string;
    // curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeFunction);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &header_string);

    CURLcode res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
        return strcat((char *) "error: ", curl_easy_strerror(res));
    }

    return response_string;
}

std::string Utils::capitalize_string(std::string_view str) {
    char res[str.size()];

    for (int i = 0; i < str.size(); i++) {
        res[i] = str[i] &~ 0x20;
    }

    return std::string(res);
}