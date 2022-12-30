#include <iostream>
#include <fstream>
#include <vector>
#include "Utils.hpp"
#include "HTMLParser.hpp"



// Driver Code
int main() {
    /* ------------------------------------------- */
    /* ------------ 1. FETCHING STAGE ------------ */
    /* ------------------------------------------- */
    std::cout << "1. Begin fetching stage" << std::endl;
    std::ifstream file("../in.txt");

    std::vector<std::string_view> urls;

    // Loop through input file
    std::string str;
    while (std::getline(file, str)) {        
        // Append http:// to beginning of url if 'HTTP" is not found in string
        if (str.find("http") == std::string::npos) {
            str = "http://" + str;
        }

        urls.push_back(str);
    }

    Utils::remove_duplicates(urls);
    Utils::print_vector(urls);

    curl_global_init(CURL_GLOBAL_DEFAULT);

    CURL *curl = curl_easy_init();
    const std::string res = Utils::curl_get_req(curl, "www.google.com");
    curl_easy_cleanup(curl);

    // const std::string res = "<div class=\"cool\">Hello World!</div>";

    HTMLParser bc = HTMLParser(res);

    // std::pair<bool, Element> el = bc.findByCallback(
    //     "a",
    //     [] (Element el) -> bool {
    //         if (Utils::capitalize_string(el.text).find("CONTACT") != std::string::npos) return true;
    //         return false;
    //     }
    // );

    // std::cout << el.first << std::endl;
    // std::cout << el.second.text << std::endl;

    curl_global_cleanup();
    return 0;
}

// 1. Add all initial URLs to thread-safe producer queue
// 2. Producer threads poll from producer queue, push raw HTMl to consumer queue -> [..., { string url: string html }]
// 3. Consumer threads poll from consumer queue, process raw HTMl, store results in hashmap -> { string url: struct data }
// 3. Consumer threads poll from consumer queue, process raw HTML and push further URLS to producer queue. 
//    if (url in hashmap): ammend results into line # hashmap[url]
//          (read line #, parse to struct, add new data struct, replace line)
//    else: push results into CSV newline, store line # in hashmap -> { string url: int line_number }
// 4.

// Alternate idea (simpler)
// Only one type of thread, fetch data, process data, update or ammend file, continue
// When all threads exit, terminate program

/*
    ISSUES:
        1. Tunnel down for nested text
        2. Add comments for all functions
*/