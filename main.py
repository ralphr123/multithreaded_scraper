import csv
import time
import utils
import queue

# Execution timer
start_time = time.time()

urls = []
readFile = open('./in.txt')
reader = csv.reader(readFile, delimiter=',')

# Get urls from input file
for row in reader:
    url = row[-1]
    if (not('http' in url)):
        url = 'http://' + url
    urls.append(url)

urls = utils.remove_duplicates(urls[1:])

# Globals
scrapeResults: utils.ScrapeResult = {}
urlQueue: queue.Queue = queue.Queue()
fetchQueue: queue.Queue = queue.Queue()

# Params
numProducers = 1
numConsumers = 1

for url in urls:
    urlQueue.put({'isSubPage': False, 'url': url})

for i in range(numProducers):
    thread = utils.CustomThread(
        {'scrapeResults': scrapeResults, 'urlQueue': urlQueue, 'fetchQueue': fetchQueue},
        'producer'
    )
    thread.daemon = True
    thread.start()

for i in range(numConsumers):
    thread = utils.CustomThread(
        {'scrapeResults': scrapeResults, 'urlQueue': urlQueue, 'fetchQueue': fetchQueue},
        'consumer'
    )
    thread.daemon = True
    thread.start()

urlQueue.join()
fetchQueue.join()
urlQueue.join()
fetchQueue.join()

# Setup output file
writeFile = open('out.csv', 'w', newline='', encoding='utf-8')
writer = csv.writer(writeFile)
writer.writerow(['url', 'title', 'description', 'favicon', 'emails',
                'numbers', 'socials', 'street', 'city', 'region', 'postalCode'])

# Setup failure file
failFile = open('fail.csv', 'w', newline='', encoding='utf-8')
failWriter = csv.writer(failFile)
failWriter.writerow(['url', 'title', 'description'])

# Track success rate
successCount = 0

# Write to output file (csv)
for dataEntry in scrapeResults.values():
    if (len(dataEntry['emails']) == 0 and len(dataEntry['numbers']) == 0):
        if (dataEntry['title'] or dataEntry['description']):
            row = [
                dataEntry['url'],
                dataEntry['title'],
                dataEntry['description']
            ]
            failWriter.writerow(row)
    else:
        successCount += 1
        row = [
            dataEntry['url'],
            utils.strip_string(dataEntry['title']),
            utils.strip_string(dataEntry['description']),
            utils.get_url_path(dataEntry['favicon']),
            utils.filter_emails(dataEntry['emails']),
            utils.sanitize_numbers(dataEntry['numbers']),
            utils.filter_urls(dataEntry['socials']),
            utils.sanitize_streets(dataEntry.get('street', [])),
            dataEntry.get('city', ''),
            dataEntry.get('region', ''),
            dataEntry.get('postalCode', ''),
        ]

        writer.writerow(row)

# Print execution time and success rate
print('--- %s seconds, %s %% hit rate ---' %
      (time.time() - start_time, round((successCount / len(urls)) * 100)))
