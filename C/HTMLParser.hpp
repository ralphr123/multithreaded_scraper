#ifndef HTMLPARSER_H    
#define HTMLPARSER_H

#include "libxml2/libxml/HTMLparser.h"
#include <iostream>
#include <unordered_map>
#include <vector>

struct Element {
    std::string name;
    std::string text;
    std::unordered_map<std::string, std::string> attributes;
};

class HTMLParser {
    private:
    // Map tag name to Element
    std::unordered_map<std::string, std::vector<Element> > document;
    
    template <typename Node> std::string getText(const Node n);
    void initialize(xmlNode *root);


    public:
    HTMLParser(std::string html);

    // Get Element by tag name and Attribute, if found first in pair is true
    std::pair<bool, Element> findByAttribute(std::string name, std::string key, std::string val);

    // Get Element by tag name and callback, if found first in pair is true
    std::pair<bool, Element> findByCallback(std::string name, std::function<bool(Element el)> callback);
};

#endif