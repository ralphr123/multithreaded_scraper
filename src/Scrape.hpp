#ifndef SCRAPE_H    // To make sure you don't declare the function more than once by including the header multiple times.
#define SCRAPE_H

#include <vector>
#include <string>
#include <atomic>
#include <fstream>
#include <unordered_map>
#include "SafeQueue.hpp"
#include "Utils.hpp"
#include "HTMLParser.hpp"
#include "Utils.hpp"

struct ScrapeResult {
    // Regex
    std::set<std::string> emails;
    std::set<std::string> numbers;
    std::set<std::string> socials;
    std::set<std::string> streets;
    
    // Static fields
    std::string title;
    std::string description;
    std::string favicon;
    std::string city;
    std::string region;
    std::string postalCode;

    ScrapeResult& operator+(const ScrapeResult& sr) {
        // Merge regex
        emails.insert(sr.emails.begin(), sr.emails.end());
        numbers.insert(sr.numbers.begin(), sr.numbers.end());
        socials.insert(sr.socials.begin(), sr.socials.end());
        streets.insert(sr.streets.begin(), sr.streets.end());

        // Set location information if not existing
        city = city.size() > 0 ? city : sr.city;
        region = region.size() > 0 ? region : sr.region;
        postalCode = postalCode.size() > 0 ? postalCode : sr.postalCode;

        return *this;
    }
};

/* ------------------------------------------- */
/* ------------ CLASS DEFINITION ------------- */
/* ------------------------------------------- */

class Scrape {
    private:
    
    public:
        static void producer(SafeQueue &urls, SafeQueue &htmls, std::atomic<unsigned int> &activeThreads);
        static void consumer(
            SafeQueue &urls, 
            SafeQueue &htmls, 
            std::atomic<unsigned int> &activeThreads, 
            std::ofstream &outCSV, 
            std::mutex &mtx,
            std::unordered_map<std::string, ScrapeResult> &mem
        );
};

#endif