#include "Scrape.hpp"

void Scrape::producer(SafeQueue &urls, SafeQueue &htmls, std::atomic<unsigned int> &activeThreads) {
    CURL *curl = curl_easy_init();
    Utils::curlSetHeaders(curl);
    while (!urls.empty() || !htmls.empty()) {
        QueueArg queueArg = urls.dequeue(&htmls, &activeThreads);

        if (queueArg.url == "xxs") {
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
        QueueArg queueArg = htmls.dequeue(&urls, &activeThreads);

        if (queueArg.url == "xxs") {
            break;
        }

        // Get base URL for hashing data to same line in case of contact page
        std::string org_url = queueArg.url;
        queueArg.url = Utils::getUrlDomain(queueArg.url);

        std::cout << activeThreads.load() << std::endl;
        if (activeThreads.load() == 0) {
            urls.notify_all();
            htmls.notify_all();
        }

        // Increment active threads counter
        activeThreads.fetch_add(1, std::memory_order_relaxed);

        HTMLParser bc = HTMLParser(queueArg.html);
        ScrapeResult scrapeResult;
        bool foundContactPage = false;

        // If not currently scraping contact page, fetch static fields
        if (mem.count(queueArg.url) != 1) {
            // Contact URL
            std::pair<bool, Element> contactUrl_res = bc.findByCallback(
                "a",
                [] (Element el) -> bool {
                    if (Utils::lowerCaseString(el.text).find("contact") != std::string::npos) return true;
                    if (Utils::lowerCaseString(el.attributes["href"]).find("contact") != std::string::npos) return true;
                    return false;
                }
            );

            // Get contact page URL if it is found
            std::string contactUrl;
            if (contactUrl_res.first) {
                contactUrl = contactUrl_res.second.attributes["href"];

                if (contactUrl.size() > 0) {
                    if (contactUrl[0] == 'h' && contactUrl[1] == 't' && contactUrl[2] == 't' && contactUrl[3] == 'p') {
                        queueArg.url = Utils::getUrlDomain(contactUrl);
                    } else {
                        contactUrl = queueArg.url + (contactUrl[0] == '/' ? "" : "/") + contactUrl;
                    }

                    urls.enqueue((QueueArg) { .url =  contactUrl });

                    foundContactPage = true;
                }
            }

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

            // Static fields
            std::vector<std::vector<std::string> > locations = bc.getAllReMatches(R"(([a-zA-Z]+)[, ][ ]?([a-zA-Z]+)[, ][ ]?(\d{5}|[A-Z]\d{1}[A-Z]{1} \d{1}[A-Z]{1}\d{1}))");
            scrapeResult.title = title_res.first ? "\"" + title_res.second.text + "\"" : "\"\"";
            scrapeResult.description = description_res.first ? "\"" + description_res.second.attributes["content"] + "\"" : "\"\"";
            scrapeResult.favicon = favicon_res.first ? "\"" + Utils::getUrlPath(favicon_res.second.attributes["href"]) + "\"" : "\"\"";
            scrapeResult.city = locations.size() > 0 ? locations[0][0] : "\"\"";
            scrapeResult.region = locations.size() > 0 ? locations[0][1] : "\"\"";
            scrapeResult.postalCode = locations.size() > 0 ? locations[0][2] : "\"\"";
        }

        // Regex
        scrapeResult.emails = Utils::filterEmails(bc.getAllReMatches(R"(([\w.+-]+@[\w-]+\.[\w.-]+))"));
        scrapeResult.numbers = Utils::filterNumbers(bc.getAllReMatches(R"(([(]?([\d]{3})[)]?[ -]?([\d]{3})[ -]?([\d]{4})))"));
        scrapeResult.socials = Utils::filterSocials(bc.getAllReMatches(R"((http|ftp|https):\/\/([\w_-]+(?:(?:\.[\w_-]+)+))([\w.,@?^=%&:\/~+#-]*[\w@?^=%&\/~+#-]))"));
        scrapeResult.streets = Utils::filterStreets(bc.getAllReMatches(R"((([0-9]+) ([a-zA-Z]+) (Street|Avenue|Blvd|Boulevard|Road|Ave|Drive)[ ]?(West|East)?))"));

        // If found a contact page, save main page info to map and wait for contact data
        if (foundContactPage) {
            mem[queueArg.url] = std::move(scrapeResult);

            // Decrement active threads counter
            activeThreads.fetch_sub(1, std::memory_order_relaxed);

            continue;
        }

        // If currently scraping contact page, merge data with main page
        if (mem.count(queueArg.url) == 1) {
            scrapeResult = mem[queueArg.url] + scrapeResult;
            mem.erase(queueArg.url);
        }

        // Output to file if valuable data exists
        if (scrapeResult.emails.size() > 0 || scrapeResult.numbers.size() > 0 || scrapeResult.socials.size() > 0) {
            // mtx.lock();
            outCSV << (
                queueArg.url + "," + 
                scrapeResult.title + "," + 
                scrapeResult.description + "," + 
                scrapeResult.favicon + "," + 
                Utils::setToString(scrapeResult.emails) + "," +
                Utils::setToString(scrapeResult.numbers) + "," +
                Utils::setToString(scrapeResult.socials) + "," +
                Utils::setToString(scrapeResult.streets) + "," +
                scrapeResult.city + "," + 
                scrapeResult.region + "," + 
                scrapeResult.postalCode + "\n"
            );
            // mtx.unlock();
        }

        // Decrement active threads counter
        activeThreads.fetch_sub(1, std::memory_order_relaxed);
    }
}
