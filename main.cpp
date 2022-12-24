#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include "utils.hpp"

// Driver Code
int main() {
    /* ------------------------------------------- */
    /* ------------ 1. FETCHING STAGE ------------ */
    /* ------------------------------------------- */
    std::cout << "1. Begin fetching stage" << std::endl;
    std::ifstream file("in.txt");

    std::vector<std::string> urls;

    // Loop through input file
    std::string str;
    while (std::getline(file, str)) {        
        // Append http:// to beginning of url if 'HTTP" is not found in string
        if (str.find("http") == std::string::npos) {
            str = "http://" + str;
        }

        urls.push_back(str);
    }

    remove_duplicates(urls);
    print_vector(urls);

    // std::map<std::string, int> mem;

    curl_global_init(CURL_GLOBAL_DEFAULT);

    // Test CURL call
    CURL *curl = curl_easy_init();
    // std::string raw = curl_get_req(curl, "https://www.dadavan.com/");
    std::string html = "<div>Hello World!</div>";
    curl_easy_cleanup(curl);

    // Create document
    lxb_html_document_t *document = lxb_html_document_create();

    lxb_status_t status = lxb_html_document_parse(document, (const lxb_char_t *) html.c_str(), html.size() - 1);
    if (status != LXB_STATUS_OK) {
        exit(EXIT_FAILURE);
    }
    
    document->dom_document.element;

    // Ready to work with document
    

    // Destroy document
    lxb_html_document_destroy(document);

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