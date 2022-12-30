#ifndef UTILS_H    // To make sure you don't declare the function more than once by including the header multiple times.
#define UTILS_H

#include <vector>
#include <iostream>
#include <curl/curl.h>

/* ------------------------------------------- */
/* ------------ CLASS DEFINITION ------------- */
/* ------------------------------------------- */

class Utils {
    private:
        static size_t write_curl_data(void* ptr, size_t size, size_t nmemb, void* str);
    
    public:
        static void print_vector(const std::vector<std::string_view> vec);
        static void remove_duplicates(std::vector<std::string_view> &vec);
        static std::string curl_get_req(CURL *curl, const std::string_view url);
        static std::string capitalize_string(std::string_view str);
};

#endif