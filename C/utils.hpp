#ifndef UTILS_H    // To make sure you don't declare the function more than once by including the header multiple times.
#define UTILS_H

#include <vector>
#include <iostream>
#include <curl/curl.h>
#include <lexbor/html/parser.h>
#include <lexbor/dom/interfaces/element.h>
// #include <lexbor/css/css.h>

// /opt/homebrew/include/lexbor/html/parser.h
// /opt/homebrew/include/lexbor/dom/interfaces/element.h

/* ------------------------------------------- */
/* ----------------- STRUCTS ----------------- */
/* ------------------------------------------- */



/* ------------------------------------------- */
/* ----------------- METHODS ----------------- */
/* ------------------------------------------- */

void print_vector(std::vector<std::string> &vec);
void remove_duplicates(std::vector<std::string> &vec);
std::string curl_get_req(CURL *curl, const std::string url);

#endif