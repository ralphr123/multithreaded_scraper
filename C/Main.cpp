#include "Scrape.hpp"
#include <thread>
#include <chrono>

// Driver Code
int main() {
    /* ------------------------------------------- */
    /* ------------ 1. FETCHING STAGE ------------ */
    /* ------------------------------------------- */
    auto start = std::chrono::high_resolution_clock::now();
    std::ios_base::sync_with_stdio(false);
    std::ifstream file("../in.txt");

    SafeQueue urls = SafeQueue(true);
    SafeQueue htmls = SafeQueue(false);

    // Loop through input file
    std::string url;
    while (std::getline(file, url)) {
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

    int num_producers = 22;
    int num_consumers = 5;

    std::thread producers[num_producers];
    std::thread consumers[num_consumers];

    for (int i = 0; i < num_producers; i++) {
        producers[i] = std::thread(Scrape::producer, std::ref(urls), std::ref(htmls), std::ref(activeThreads));
    }

    for (int i = 0; i < num_consumers; i++) {
        consumers[i] = std::thread(Scrape::consumer, std::ref(urls), std::ref(htmls), std::ref(activeThreads), std::ref(outCSV), std::ref(mtx), std::ref(mem));
    }

    for (int i = 0; i < num_producers; i++) {
        producers[i].join();
    }

    for (int i = 0; i < num_consumers; i++) {
        consumers[i].join();
    }

    outCSV.close();
    curl_global_cleanup();
    
    auto stop = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(stop - start);

    std::cout << "Execution time: " << duration.count() << " seconds." << std::endl;

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
        2. Add comments for all functions
        3. curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L)
        4. Can save all locations instead of just first
        5. Location must be a capital letter
*/