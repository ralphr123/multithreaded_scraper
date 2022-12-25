#include "utils.hpp"

/// @brief Print contents of C++ vector to terminal for debugging
/// @param vec 
void print_vector(std::vector<std::string> &vec) {
    for (std::string str: vec) {
        std::cout << str << std::endl;
    }
}

/// @brief Remove duplicates from C++ vector (list)
/// @param vec 
void remove_duplicates(std::vector<std::string> &vec) {
    vec.erase( unique( vec.begin(), vec.end() ), vec.end() );
}

size_t write_data(void* ptr, size_t size, size_t nmemb, void* str) {
  std::string *s = static_cast<std::string *>(str);
  std::copy((char *) ptr, (char *) ptr + (size + nmemb), std::back_inserter(*s));
  return size * nmemb;
}

/// @param curl Curl handle
/// @param url URL
/// @return Raw HTML contents of page
std::string curl_get_req(CURL *curl, const std::string url) {
    if (!curl) return "invalid";

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

    // Allow 50 redirects
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 50L);

    // Write callback to response string
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);

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
