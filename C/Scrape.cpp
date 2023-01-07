#include "Scrape.hpp"

void Scrape::producer(SafeQueue &urls, SafeQueue &htmls, std::atomic<unsigned int> &activeThreads) {
    CURL *curl = curl_easy_init();
    Utils::curlSetHeaders(curl);
    while (1) {
        // Dequeue url from producer queue
        QueueArg queueArg = urls.dequeue(std::ref(activeThreads));

        // No more work to be done
        if (queueArg.url == "done") {
            break;
        }

        // Increment active threads counter
        activeThreads.fetch_add(1, std::memory_order_relaxed);

        // Fetch raw HTML with CURL and add it to consumer queue
        queueArg.html = Utils::curlGetReq(curl, queueArg.url);

        if (queueArg.html != "error") {
            htmls.enqueue(std::move(queueArg));
        }

        // Decrement active threads counter
        activeThreads.fetch_sub(1, std::memory_order_relaxed);
    }
    curl_easy_cleanup(curl);
}

void Scrape::consumer(
    SafeQueue &urls, 
    SafeQueue &htmls, 
    std::atomic<unsigned int> &activeThreads, 
    std::ofstream &outCSV, 
    std::mutex &mtx, 
    std::unordered_map<std::string, ScrapeResult> &mem
) {
    while (1) {
        // Dequeue html from consumer queue
        QueueArg queueArg = htmls.dequeue(std::ref(activeThreads));

        // No more work to be done
        if (queueArg.url == "done") {
            break;
        }

        // Get base URL for hashing data to same line in case of contact page
        queueArg.url = Utils::getUrlDomain(queueArg.url);

        // Increment active threads counter
        activeThreads.fetch_add(1, std::memory_order_relaxed);

        HTMLParser bc = HTMLParser(queueArg.html);

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

        ScrapeResult scrapeResult;

        // Regex
        scrapeResult.emails = Utils::filterEmails(bc.getAllReMatches(R"(([\w.+-]+@[\w-]+\.[\w.-]+))"));
        scrapeResult.numbers = Utils::filterNumbers(bc.getAllReMatches(R"(([(]?([\d]{3})[)]?[ -]?([\d]{3})[ -]?([\d]{4})))"));
        scrapeResult.socials = Utils::filterSocials(bc.getAllReMatches(R"((http|ftp|https):\/\/([\w_-]+(?:(?:\.[\w_-]+)+))([\w.,@?^=%&:\/~+#-]*[\w@?^=%&\/~+#-]))"));
        scrapeResult.streets = Utils::filterStreets(bc.getAllReMatches(R"((([0-9]+) ([a-zA-Z]+) (Street|Avenue|Blvd|Boulevard|Road|Ave|Drive)[ ]?(West|East)?))"));
        std::vector<std::vector<std::string> > locations = bc.getAllReMatches(R"(([a-zA-Z]+)[, ][ ]?([a-zA-Z]+)[, ][ ]?(\d{5}|[A-Z]\d{1}[A-Z]{1} \d{1}[A-Z]{1}\d{1}))");
        
        // Static fields
        scrapeResult.title = title_res.first ? "\"" + title_res.second.text + "\"" : "\"\"";
        scrapeResult.description = description_res.first ? "\"" + description_res.second.attributes["content"] + "\"" : "\"\"";
        scrapeResult.favicon = favicon_res.first ? "\"" + Utils::getUrlPath(favicon_res.second.attributes["href"]) + "\"" : "\"\"";
        scrapeResult.city = locations.size() > 0 ? locations[0][0] : "\"\"";
        scrapeResult.region = locations.size() > 0 ? locations[0][1] : "\"\"";
        scrapeResult.postalCode = locations.size() > 0 ? locations[0][2] : "\"\"";

        // Get contact page URL if it is found and not currently scraping a contact page
        std::string contactUrl;
        if (mem.count(queueArg.url) != 1 && contactUrl_res.first) {
            contactUrl = contactUrl_res.second.attributes["href"];

            if (contactUrl.size() > 0) {
                if (contactUrl.find(".com") == std::string::npos) {
                    contactUrl = queueArg.url + "/" + contactUrl;
                }

                urls.enqueue((QueueArg) { .url =  std::move(contactUrl) });

                mem[queueArg.url] = std::move(scrapeResult);

                // Decrement active threads counter
                activeThreads.fetch_sub(1, std::memory_order_relaxed);

                continue;
            }
        }

        // Currently scraping contact page
        if (mem.count(queueArg.url) == 1) {
            scrapeResult = scrapeResult + mem[queueArg.url];
            mem.erase(queueArg.url);
        }

        // Output to file
        std::string line = (
            queueArg.url + ", " + 
            scrapeResult.title + ", " + 
            scrapeResult.description + ", " + 
            scrapeResult.favicon + ", " + 
            Utils::setToString(scrapeResult.emails) + ", " +
            Utils::setToString(scrapeResult.numbers) + ", " +
            Utils::setToString(scrapeResult.socials) + ", " +
            Utils::setToString(scrapeResult.streets) + ", " +
            scrapeResult.city + ", " + 
            scrapeResult.region + ", " + 
            scrapeResult.postalCode + "\n"
        );

        mtx.lock();
        outCSV << line;
        mtx.unlock();

        // Decrement active threads counter
        activeThreads.fetch_sub(1, std::memory_order_relaxed);
    }
}
