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

    Utils::removeDuplicates(urls);

    // TODO: Move to thread

    curl_global_init(CURL_GLOBAL_DEFAULT);

    CURL *curl = curl_easy_init();
    std::string res = Utils::curlGetReq(curl, urls.front());
    curl_easy_cleanup(curl);

    HTMLParser bc = HTMLParser(res);

    // Contact URL
    std::pair<bool, Element> contactUrl_res = bc.findByCallback(
        "a",
        [] (Element el) -> bool {
            if (Utils::lowerCaseString(el.text).find("contact") != std::string::npos) return true;
            if (Utils::lowerCaseString(el.attributes["href"]).find("contact") != std::string::npos) return true;
            return false;
        }
    );

    // Site description
    std::pair<bool, Element> description_res = bc.findByAttribute("meta", "name", "description");

    // Site title
    std::pair<bool, Element> title_res = bc.findByAttribute("meta", "name", "og:site_name");
    if (!title_res.first) title_res = bc.findByAttribute("meta", "name", "og:title");
    if (!title_res.first) title_res = std::pair<bool, Element>(1, bc.getTitle());
    
    // Favicon
    std::pair<bool, Element> favicon_res = bc.findByCallback(
        "link",
        [] (Element e) -> bool {
            if (Utils::lowerCaseString(e.attributes["rel"]).find("icon") != std::string::npos) return true;
            return false;
        }
    );

    // Regex
    std::set<std::string> emails = Utils::filterEmails(bc.getAllReMatches(R"(([\w.+-]+@[\w-]+\.[\w.-]+))"));
    std::set<std::string> numbers = Utils::filterNumbers(bc.getAllReMatches(R"(([(]?([\d]{3})[)]?[ -]?([\d]{3})[ -]?([\d]{4})))"));
    std::set<std::string> socials = Utils::filterSocials(bc.getAllReMatches(R"((http|ftp|https):\/\/([\w_-]+(?:(?:\.[\w_-]+)+))([\w.,@?^=%&:\/~+#-]*[\w@?^=%&\/~+#-]))"));
    std::set<std::string> streets = Utils::filterStreets(bc.getAllReMatches(R"((([0-9]+) ([a-zA-Z]+) (Street|Avenue|Blvd|Boulevard|Road|Ave|Drive)[ ]?(West|East)?))"));
    std::vector<std::vector<std::string> > locations = bc.getAllReMatches(R"(([a-zA-Z]+)[, ][ ]?([a-zA-Z]+)[, ][ ]?(\d{5}|[A-Z]\d{1}[A-Z]{1} \d{1}[A-Z]{1}\d{1}))");
    
    // Static fields
    std::string contactUrl = contactUrl_res.first ? contactUrl_res.second.attributes.at("href") : "";
    std::string title = title_res.first ? "\"" + title_res.second.text + "\"" : "\"\"";
    std::string description = description_res.first ? "\"" + description_res.second.attributes.at("content") + "\"" : "\"\"";
    std::string favicon = favicon_res.first ? "\"" + Utils::getUrlPath(favicon_res.second.attributes["href"]) + "\"" : "\"\"";
    std::string city = locations.size() > 0 ? locations[0][0] : "\"\"";
    std::string region = locations.size() > 0 ? locations[0][1] : "\"\"";
    std::string postalCode = locations.size() > 0 ? locations[0][2] : "\"\"";

    // Output to file
    std::ofstream outCSV;
    outCSV.open("out.csv");
    outCSV << "url,title,description,favicon,emails,numbers,socials,street,city,region,postalCode\n";

    std::string line = (
        urls.front() + ", " + 
        title + ", " + 
        description + ", " + 
        favicon + ", " + 
        Utils::setToString(emails) + ", " +
        Utils::setToString(numbers) + ", " +
        Utils::setToString(socials) + ", " +
        Utils::setToString(streets) + ", " +
        city + ", " + 
        region + ", " + 
        postalCode + "\n"
    );

    outCSV << line;
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