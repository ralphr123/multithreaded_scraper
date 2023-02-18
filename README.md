Uses producer and consumer threads to fetch, organize, and process site data from a list of URLs efficiently. Measures % hit rate of sites successfully scraped, and tracks failed valid live URLs in a fail file. Gathers emails, numbers, social links, and some location info. Example output included. 

I had a short amount of time to make this originally, so I chose python for it's libraries and convenience. If I would redo this, I'd write it in C++ for speed. Another issue is a bottleneck: the scraper keeps processed data in memory before outputting everything to the file inside the main thread. Given more time, I'll opt to continuously write finished entries to the outfile, saving time and memory.

UPDATE: I ended up rewriting it in C++ (see src folder), increasing execution time by over 50%, and possibly significantly more on machines with more network sockets to allocate to the script. I also refactored the logic for outputting to the outfile - now only sources that need to be kept in memory are stored, and immediately released when no longer needed; the script now uses significantly less memory during execution.
