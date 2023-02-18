#ifndef UTILS_H    // To make sure you don't declare the function more than once by including the header multiple times.
#define UTILS_H

#include <iostream>
#include <vector>
#include <curl/curl.h>
#include <set>
#include "re2/re2.h"

/* ------------------------------------------- */
/* ------------ CLASS DEFINITION ------------- */
/* ------------------------------------------- */

class Utils {
    private:
        static size_t writeCurlData(void* ptr, size_t size, size_t nmemb, void* str);
    
    public:
        static std::string setToString(const std::set<std::string> &vec);
        static void printVector2D(const std::vector<std::vector<std::string> > &vec);
        static void removeDuplicates(std::vector<std::string> &vec);
        static void curlSetHeaders(CURL *curl);
        static std::string curlGetReq(CURL *curl, const std::string &url);
        static std::string lowerCaseString(const std::string_view &str);
        static void removeLinebreaks(std::string &str);
        static bool stringReplace(std::string& str, const std::string& from, const std::string& to);
        static int numberOfMatches(const std::string_view &str, const std::string_view &pattern);
        static std::string getUrlPath(const std::string &url);
        static std::string getUrlDomain(const std::string &url);
        static std::set<std::string > filterSocials(const std::vector<std::vector<std::string> > &urls);
        static std::set<std::string > filterNumbers(const std::vector<std::vector<std::string> > &numbers);
        static std::set<std::string > filterEmails(const std::vector<std::vector<std::string> > &emails);
        static std::set<std::string > filterStreets(const std::vector<std::vector<std::string> > &streets);
};

#endif