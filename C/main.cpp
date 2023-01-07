#include "Scrape.hpp"

// Driver Code
int main() {
    /* ------------------------------------------- */
    /* ------------ 1. FETCHING STAGE ------------ */
    /* ------------------------------------------- */
    std::ios_base::sync_with_stdio(false);
    std::cout << "1. Begin fetching stage" << std::endl;
    std::ifstream file("../in.txt");

    SafeQueue urls = SafeQueue(true);
    SafeQueue htmls = SafeQueue(false);

    // Loop through input file
    std::string url;
    while (std::getline(file, url)) {        
        // Append http:// to beginning of url if 'HTTP" is not found in string
        if (!(url[0] == 'h' && url[1] == 't' && url[2] == 't' && url[3] == 'p')) {
            url = "http://" + url;
        }

        urls.enqueue((QueueArg) { .url = std::move(url) });
    }

    curl_global_init(CURL_GLOBAL_DEFAULT);

    // Output first line of CSV
    std::ofstream outCSV;
    outCSV.open("out.csv");
    outCSV << "url,title,description,favicon,emails,numbers,socials,street,city,region,postalCode\n";

    std::atomic<unsigned int> activeThreads = { 0 };
    std::mutex mtx;
    std::unordered_map<std::string, ScrapeResult> mem;

    Scrape::producer(std::ref(urls), std::ref(htmls), std::ref(activeThreads));
    Scrape::consumer(std::ref(urls), std::ref(htmls), std::ref(activeThreads), std::ref(outCSV), std::ref(mtx), std::ref(mem));
    Scrape::producer(std::ref(urls), std::ref(htmls), std::ref(activeThreads));
    Scrape::consumer(std::ref(urls), std::ref(htmls), std::ref(activeThreads), std::ref(outCSV), std::ref(mtx), std::ref(mem));

    outCSV.close();
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
        1. Tunnel down for nested text (!! IMPORTANT)
        2. Add comments for all functions
        3. curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L)
        4. Can save all locations instead of just first
        5. Location capital letter
*/