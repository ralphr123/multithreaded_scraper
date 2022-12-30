#include "HTMLParser.hpp"

/* ------------------------------------------- */
/* ----------------- PRIVATE ----------------- */
/* ------------------------------------------- */

template <typename Node> std::string HTMLParser::getText(Node n) {
    std::string res;

    xmlChar* val = xmlNodeListGetString(n->doc, n->children, 1);
    if (val) res = (const char *) val;
    xmlFree(val);

    return res;
}

// TODO: Rewrite iteratively
void HTMLParser::initialize(xmlNode *root) {
    for (xmlNode *n = root; n; n = n->next) {
        std::string name = (const char *) n->name;

        Element el = { .name = name, .text = getText(n), .attributes = std::unordered_map<std::string, std::string>{} };

        // std::cout << el.name << " - " << el.text << std::endl;

        for (xmlAttr *a = n->properties; a; a = a->next) {
            el.attributes[(const char *) a->name] = getText(a);
        }

        // HTMl file has no tags of this type yet
        if (document.count(name) == 1) {
            document[name] = std::vector<Element>{};
        }

        document[name].push_back(el);

        initialize(n->children);
    }
}

/* ------------------------------------------- */
/* ------------------ PUBLIC ----------------- */
/* ------------------------------------------- */

HTMLParser::HTMLParser(std::string html) {
    htmlParserCtxtPtr parser = htmlCreatePushParserCtxt(NULL, NULL, NULL, 0, NULL, (xmlCharEncoding) 0);
    htmlCtxtUseOptions(parser, HTML_PARSE_NOBLANKS | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING | HTML_PARSE_NONET);
    htmlParseChunk(parser, html.c_str(), html.size(), 1);

    initialize(xmlDocGetRootElement(parser->myDoc));
} 

std::pair<bool, Element> HTMLParser::findByAttribute(std::string name, std::string key, std::string val) {
    for (Element el : document[name]) {
        if (el.attributes.count(key) == 1 && el.attributes[key] == val) {
            return std::pair<bool, Element>(true, el);
        }
    }

    return std::pair<bool, Element>(false, Element{});
}

std::pair<bool, Element> HTMLParser::findByCallback(std::string name, std::function<bool(Element el)> callback) {
    for (Element el : document[name]) {
        if (callback(el)) {
            return std::pair<bool, Element>(true, el);
        }
    }

    return std::pair<bool, Element>(false, Element{});
}