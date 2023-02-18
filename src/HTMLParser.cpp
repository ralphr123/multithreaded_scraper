#include "HTMLParser.hpp"

/* ------------------------------------------- */
/* ----------------- PRIVATE ----------------- */
/* ------------------------------------------- */

/// @brief Get text contents of dom node
/// @tparam Node: type xmlNode or xmlAttribute
/// @param n 
/// @return 
template <typename Node> std::string HTMLParser::getText(Node n) {
    std::string res;

    xmlChar* val = xmlNodeListGetString(n->doc, n->children, 1);
    if (val) res = (const char *) val;
    xmlFree(val);

    return res;
}

/// @brief Pretty print Element struct in raw HTML format
/// @param e: Element to be printed
void HTMLParser::printElement(Element e) {
    std::cout << "<" << e.name;
    for (auto& [key, value]: e.attributes) {  
        std::cout << " " << key << "=" << "\"" << value << "\""; 
    }
    std::cout << ">" << e.text << "</" << e.name << ">" << std::endl;
}

// TODO: Rewrite iteratively
/// @brief Initialize HTMLParser instance
/// @param root 
void HTMLParser::initialize(xmlNode *root) {
    for (xmlNode *n = root; n; n = n->next) {
        std::string name = n->name ? (const char *) n->name : "noname";

        Element el = { .name = name, .text = getText(n), .attributes = std::unordered_map<std::string, std::string>{} };

        for (xmlAttr *a = n->properties; a; a = a->next) {
            el.attributes[(const char *) a->name] = getText(a);
        }

        // HTMl file has no tags of this type yet
        if (document.count(name) == 0) {
            document[name] = std::vector<Element>{};
        }

        document[name].push_back(el);

        if (name == "title") {
            title = el;
        }

        initialize(n->children);
    }
}

/* ------------------------------------------- */
/* ------------------ PUBLIC ----------------- */
/* ------------------------------------------- */

HTMLParser::HTMLParser(std::string &rawHtml) {
    this->html = std::move(rawHtml);

    // Initialize parser object
    htmlParserCtxtPtr parser = htmlCreatePushParserCtxt(NULL, NULL, NULL, 0, NULL, (xmlCharEncoding) 0);
    htmlCtxtUseOptions(parser, HTML_PARSE_NOBLANKS | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING | HTML_PARSE_NONET);
    htmlParseChunk(parser, html.c_str(), html.size(), 1);
    initialize(xmlDocGetRootElement(parser->myDoc));
} 

Element HTMLParser::getTitle() {
    return title;
}

/// @brief Find Element in dom by tag name and a single attribute
/// @param name: tag name
/// @param key: attribute key
/// @param val: attribute value
/// @return Pair. First is bool (1 = successfully found, 0 = not found). Second is Element (blank element if not found).
std::pair<bool, Element> HTMLParser::findByAttribute(const std::string &name, const std::string &key, const std::string &val) {
    for (Element el : document[name]) {
        if (el.attributes.count(key) == 1 && el.attributes[key] == val) {
            return std::pair<bool, Element>(true, el);
        }
    }

    return std::pair<bool, Element>(false, Element{});
}

/// @brief Find Element in dom by tag name and callback function
/// @param name: Element tag name
/// @param callback: Element is passed into callback
/// @return Pair. First is bool (1 = successfully found, 0 = not found). Second is Element (blank element if not found).
std::pair<bool, Element> HTMLParser::findByCallback(std::string name, std::function<bool(Element e)> callback) {
    for (Element e : document[name]) {
        if (callback(e)) {
            return std::pair<bool, Element>(true, e);
        }
    }

    return std::pair<bool, Element>(false, Element{});
}

/// @brief Get all regex matches of pattern inside raw HTML of HTMLparser object
/// @param pattern: Regex pattern to match against
/// @return Vector of match vectors (match vector size = # of capture groups in pattern)
std::vector<std::vector<std::string> > HTMLParser::getAllReMatches(std::string_view pattern) {
    RE2 re = RE2(pattern);
    re2::StringPiece input = re2::StringPiece(html);

    assert(re.ok());

    int n = re.NumberOfCapturingGroups();

    std::vector<std::vector<std::string> > res;

    std::vector<std::string> match(n);
    std::vector<RE2::Arg> args(n);
    std::vector<RE2::Arg *> arg_ptrs(n);

    for (int i = 0; i < n; i++) {
        args[i] = &match[i];
        arg_ptrs[i] = &args[i];
    }

    // std::string s;
    while (RE2::FindAndConsumeN(&input, re, arg_ptrs.data(), n)) {
        res.push_back(match);
    }

    return res;
}
