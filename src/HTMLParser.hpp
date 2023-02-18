#ifndef HTMLPARSER_H    
#define HTMLPARSER_H

#include "libxml2/libxml/HTMLparser.h"
#include <iostream>
#include <unordered_map>
#include <vector>
#include "re2/re2.h"
#include "Utils.hpp"
#include <type_traits>

struct Element {
    std::string name;
    std::string text;
    std::unordered_map<std::string, std::string> attributes;
};

class HTMLParser {
    private:
    // Map tag name to Element
    Element title;
    std::string html;
    std::unordered_map<std::string, std::vector<Element> > document;
    
    template <typename Node> std::string getText(const Node n);
    void printElement(Element e);
    void initialize(xmlNode *root);

    public:
    HTMLParser(std::string &rawHtml);

    // Get site title
    Element getTitle();

    // Get Element by tag name and Attribute. If found, first in pair is true
    std::pair<bool, Element> findByAttribute(const std::string &name, const std::string &key, const std::string &val);

    // Get Element by tag name and callback. If found, first in pair is true
    std::pair<bool, Element> findByCallback(std::string name, std::function<bool(Element e)> callback);

    // Get all matches for a regex pattern
    std::vector<std::vector<std::string> > getAllReMatches(std::string_view pattern);
};

#endif