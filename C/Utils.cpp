#include "Utils.hpp"

/// @brief Callback to write HTML response body to buffer
/// @param ptr Pointer to the buffer
/// @param size Fragment size in bytes
/// @param nmemb Number of fragments
/// @param str Pointer to data
/// @return 
size_t Utils::writeCurlData(void* ptr, size_t size, size_t nmemb, void* str) {
    ((std::string *) str)->append((char*) ptr, size * nmemb);
    return size * nmemb;
}

/// @brief Print contents of C++ 1D vector to terminal for debugging
/// @param vec Vector of string
std::string Utils::setToString(const std::set<std::string> &set) {
    std::string res = "\"[";
    for (std::set<std::string>::iterator it = set.begin(); it != set.end(); it++) {
        res += *it;
        if (std::next(it) != set.end()) res += ", ";   
    }
    res += "]\"";
    return res;
}

/// @brief Print contents of C++ 2D vector to terminal for debugging
/// @param vec Vector of vectors
void Utils::printVector2D(const std::vector<std::vector<std::string> > &vec) {
    std::cout << "[";
    for (std::vector<std::string> vec2 : vec) {
        std::cout << "[";
        for (std::string_view str : vec2) {
            std::cout << str << ", ";
        }
        std::cout << "], ";
    }
    std::cout << "]" << std::endl;
}

/// @brief Remove duplicates from C++ vector (list)
/// @param vec 
void Utils::removeDuplicates(std::vector<std::string> &vec) {
    vec.erase( unique( vec.begin(), vec.end() ), vec.end() );
}

void Utils::curlSetHeaders(CURL *curl) {
    if (!curl) return;

    curl_slist *chunk = nullptr;

    // Modify HTTP headers
    chunk = curl_slist_append(chunk, "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8");
    chunk = curl_slist_append(chunk, "Accept-Language: en-US,en;q=0.5");
    chunk = curl_slist_append(chunk, "x-application-type: WebClient");
    chunk = curl_slist_append(chunk, "x-client-version: 2.10.4");
    chunk = curl_slist_append(chunk, "Origin: https://www.google.com");
    chunk = curl_slist_append(chunk, "User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10.12; rv:55.0) Gecko/20100101 Firefox/55.0");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);

    // Allow 50 redirects
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 50L);

    // Write callback to response string
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCurlData);

    // Set user agent
    curl_easy_setopt(curl, CURLOPT_USERAGENT, (char *) "Mozilla/5.0 (Macintosh; Intel Mac OS X 10.12; rv:55.0) Gecko/20100101 Firefox/55.0");

    // Set timeout to 5 seconds
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5);
}


/// @param curl Curl handle
/// @param url URL
/// @return Raw HTML contents of page
std::string Utils::curlGetReq(CURL *curl, const std::string &url) {
    if (!curl) return "invalid";

    // Set URL
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

    std::string response_string;
    std::string header_string;
    // curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeFunction);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &header_string);

    CURLcode res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
        std::cout << url.c_str() << ": " << curl_easy_strerror(res) << std::endl;
        return "error";
    }

    return response_string;
}

/// @brief Return lowercase version of string
/// @param str: String to be converted
/// @return Lowercase string
std::string Utils::lowerCaseString(const std::string_view &str) {
    std::string res;
    res.resize(str.size());

    for (int i = 0; i < str.size(); i++) {
        res[i] = str[i] | 32;
    }

    return res;
}

/// @brief Replaces all matches of a pattern replaced by a replacement in a string
/// @param str: String to modify
/// @param original: Substring to replace instances of
/// @param replacement: Substring with which to replace instances of from
/// @return New string with one, some, or all matches of a pattern replaced by a replacement 
bool Utils::stringReplace(std::string& str, const std::string& original, const std::string& replacement) {
    size_t start_pos = str.find(original);
    if (start_pos == std::string::npos) {
        return false;
    }
    str.replace(start_pos, original.length(), replacement);
    return true;
}

/// @brief Get number of regex matches in given string to given pattern
/// @param str: String to check for matches in 
/// @param pattern: String pattern to check for matches against
/// @return Number of matches
int Utils::numberOfMatches(const std::string_view &str, const std::string_view &pattern) {
    int n = 0;
    re2::StringPiece strPiece = re2::StringPiece(str);

    while(RE2::FindAndConsume(&strPiece, pattern.data())) {
        n++;
    }

    return n;
}

std::string Utils::getUrlPath(const std::string &url) {
    if (url == "") {
        return url;
    }
    // If URL starts with http(s)://, only return path
    if (url[0] == 'h' && url[1] == 't' && url[2] == 't' && url[3] == 'p') {
        int i;
        for (i = 7; i < url.size(); i++) {
            if (url[i] == '/') {
                i++;
                break;
            }
        }
        return url.substr(i);
    }
    // Remove double slash or local path prefixes
    if (url[0] == '/' || url[0] == '.') {
        if (url[1] == '/') {
            return url.substr(2);
        }
        return url.substr(1);
    }
    return url;
}

std::string Utils::getUrlDomain(const std::string &url) {
    if (url == "") {
        return url;
    }

    std::string baseUrl = "http://";
    short slashCount = 0;

    // If URL does not start with http://, add http:// prefix to it
    if (!(url[0] == 'h' && url[1] == 't' && url[2] == 't' && url[3] == 'p')) {
        slashCount = 2;
    }

    for (char ch : url) {
        if (slashCount == 2 && ch != '/') baseUrl += ch;
        if (ch == '/') slashCount++;
        if (slashCount == 3) return baseUrl;
    }

    return baseUrl;
}



/// @brief Filter out non-social urls from a list and return unique list
/// @param urls: 2D vector where of URLs split by: [protocol, domain, path]
/// @return 1D vector of unique URLs as strings
std::set<std::string > Utils::filterSocials(const std::vector<std::vector<std::string> > &urls) {
    auto isSocialUrl = [](const std::vector<std::string > url) -> bool {
        if (url[1].find("connect") != std::string::npos || url[2].find("embed") != std::string::npos) {
            return false;
        }

        if (
            url[1].find("facebook") != std::string::npos || 
            url[1].find("twitter") != std::string::npos || 
            url[1].find("instagram") != std::string::npos || 
            url[1].find("youtube") != std::string::npos || 
            url[1].find("linkedin") != std::string::npos
        ) {
            return true;
        }

        return false;
    };

    // Filter out non-social URLS
    std::vector<std::vector<std::string> > validSocials;
    std::copy_if(urls.begin(), urls.end(), std::back_inserter(validSocials), isSocialUrl);

    std::set<std::string> socialStrings;
    std::transform(validSocials.begin(), validSocials.end(), std::inserter(socialStrings, socialStrings.begin()), 
        [](std::vector<std::string> url) -> std::string {
            stringReplace(url[1], "www.", "");
            return url[1] + url[2];
        }
    );

    return socialStrings;
}

/// @brief Filter out non-valid phone numbers from 2D vector
/// @param numbers: 2D vector of phone numbers split by: [full number (w/ separators), 3 digits, 3 digits, 4 digits]
/// @return 1D vector of unique phone numbers as strings
std::set<std::string > Utils::filterNumbers(const std::vector<std::vector<std::string> > &numbers) {
    auto isPhoneNumber = [](const std::vector<std::string> number) -> bool {        
        // Ensure existence of at least one separator
        if (numberOfMatches(number[0], "[+-. ()]+") == 0) {
            return false;
        }

        std::string phoneNumberString = number[1] + number[2] + number[3];

        if (
            numberOfMatches(number[0], "(.)\1{4,}") != 0 || // is decimal number
            phoneNumberString[0] == '0' || // 0...
            phoneNumberString[0] == '1' || // 1...
            (phoneNumberString[1] == '1' && phoneNumberString[2] == '1') || // x11
            (phoneNumberString[0] == '5' && phoneNumberString[1] == '5' && phoneNumberString[2] == '5') // 555...
        ) {
            return false;
        }

        return true;
    };

    std::vector<std::vector<std::string> > validNumbers;
    std::copy_if(numbers.begin(), numbers.end(), std::back_inserter(validNumbers), isPhoneNumber);

    std::set<std::string> phoneNumberStrings;
    std::transform(validNumbers.begin(), validNumbers.end(), std::inserter(phoneNumberStrings, phoneNumberStrings.begin()), 
        [](std::vector<std::string> number) -> std::string {
            return number[1] + number[2] + number[3];
        }
    );

    return phoneNumberStrings;
}

/// @brief Filter out non-valid emails from 2D vector
/// @param emails: 2D vector of emails split by: [full email]
/// @return 1D vector of unique emails as strings
std::set<std::string > Utils::filterEmails(const std::vector<std::vector<std::string> > &emails) {
    auto isEmail = [](const std::vector<std::string> email) -> bool {     
        auto hasEndString = [](const std::string_view str, const std::string_view endStr) -> bool {        
            for (int i = str.size() - endStr.size(), j = 0; i < str.size(); i++, j++) {
                if (str[i] != endStr[j]) {
                    return false;
                }
            }

            return true;
        };

        std::string emailString = email[0];

        if (
            hasEndString(emailString, ".com") ||
            hasEndString(emailString, ".co") ||
            hasEndString(emailString, ".ca") ||
            hasEndString(emailString, ".net") ||
            hasEndString(emailString, ".org") ||
            hasEndString(emailString, ".io") ||
            hasEndString(emailString, ".info")
        ) {
            return true;
        }

        return false;
    };

    std::vector<std::vector<std::string> > validEmail;
    std::copy_if(emails.begin(), emails.end(), std::back_inserter(validEmail), isEmail);

    std::set<std::string> emailStrings;
    std::transform(validEmail.begin(), validEmail.end(), std::inserter(emailStrings, emailStrings.begin()), 
        [](std::vector<std::string> email) -> std::string {
            return email[0];
        }
    );

    return emailStrings;
}

std::set<std::string > Utils::filterStreets(const std::vector<std::vector<std::string> > &streets) {
    std::set<std::string> streetStrings;
    std::transform(streets.begin(), streets.end(), std::inserter(streetStrings, streetStrings.begin()), [](std::vector<std::string> street) -> std::string {
        return street[0];
    });

    return streetStrings;
}