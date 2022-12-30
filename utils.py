from decimal import ExtendedContext
import re
import threading
from typing import TypedDict
from bs4 import BeautifulSoup
import queue
import traceback
import requests

# ----------------------------------------------------------------------------------- #
# ------------------------------------- UTILITY ------------------------------------- #
# ----------------------------------------------------------------------------------- #


def remove_duplicates(arr: list[any]):
    return list(dict.fromkeys(arr))


def strip_string(string):
    return string.strip().replace('\n', '')


def sanitize_url(url: str):
    result = url.split('/')
    return result[0] + '//' + result[1] + result[2] + '/'


def filter_emails(emails: list[str]):
    def is_email(email):
        domains = ['.co', '.ca', '.net', '.org', '.io', '.info']
        for domain in domains:
            if domain in email:
                return True
        return False

    return remove_duplicates(list(filter(is_email, emails)))


def get_url_path(url: str):
    if (url == ''):
        return url
    if ('http' in url):
        return '/'.join(url.split('/')[3:])
    if (url[0] == '/' or url[0] == '.'):
        if (url[1] == '/'):
            return url[2:]
        return url[1:]
    return url


def filter_urls(urls: list[str]):
    socials = ['facebook', 'twitter', 'instagram', 'youtube', 'linkedin']

    def is_social_url(url: tuple):
        if ('connect' in url[1] or 'embed' in url[2]):
            return False
        for social in socials:
            if (social in url[1]):
                return True
        return False

    return remove_duplicates(list(map(lambda url: url[1].replace('www.', '') + url[2], filter(is_social_url, urls))))


def sanitize_number(number: str):
    # Must have separators
    if (len(re.findall(r'[+-. ()]+', number)) == 0):
        return None
    sanitizedNumber = (
        number.replace(' ', '')
        .replace('(', '')
        .replace(')', '')
        .replace('-', '')
        .replace('.', '')
        .replace('+', '')
    )
    # Filter invalid results
    if (
        len(re.findall(r'(.)\1{4,}', sanitizedNumber)) != 0 or
        sanitizedNumber[0] == '0' or  # 0...
        sanitizedNumber[0] == '1' or  # 1...
        (sanitizedNumber[1] == '1' and sanitizedNumber[2] == '1') or  # x11...
        (sanitizedNumber[0] == '5' and sanitizedNumber[1] == '5' and sanitizedNumber[2] == '5')  # 555...
    ):
        return None
    return sanitizedNumber


def sanitize_numbers(numbers: list[str]):
    # Remove separators
    numbers = list(map(sanitize_number, numbers))
    # Filter unlikely matches
    numbers = list(filter(lambda number: number != None, numbers))
    return remove_duplicates(numbers)


def sanitize_streets(streets: list[str]):
    return remove_duplicates(list(map(lambda street: ' '.join(street), streets)))


# ----------------------------------------------------------------------------------- #
# -------------------------------------- CLASS -------------------------------------- #
# ----------------------------------------------------------------------------------- #

# Regex strings
emailRegex = r'[\w.+-]+@[\w-]+\.[\w.-]+'
numberRegex = r'[(]?[\d]{3}[)]?[ -]?[\d]{3}[ -]?[\d]{4}'
urlRegex = r'(http|ftp|https):\/\/([\w_-]+(?:(?:\.[\w_-]+)+))([\w.,@?^=%&:\/~+#-]*[\w@?^=%&\/~+#-])'
streetRegex = r'([0-9]+) ([a-zA-Z]+) (Street|Avenue|Blvd|Boulevard|Road|Ave|Drive)[ ]?(West|East)?'
locationRegex = r'([a-zA-Z]+)[, ][ ]?([a-zA-Z]+)[, ][ ]?(\d{5}|[A-Z]\d{1}[A-Z]{1} \d{1}[A-Z]{1}\d{1})'

headers = {
    'accept': 'application/json, text/plain, */*',
    'Accept-Language': 'en-US,en;q=0.5',
    'x-application-type': 'WebClient',
    'x-client-version': '2.10.4',
    'Origin': 'https://www.google.com',
    'User-Agent': 'Mozilla/5.0 (Macintosh; Intel Mac OS X 10.12; rv:55.0) Gecko/20100101 Firefox/55.0'
}


class UrlResult(TypedDict):
    isSubPage: bool
    url: str


class FetchResult(TypedDict):
    isSubPage: bool
    data: dict


class ScrapeResult(TypedDict):
    title: str
    emails: list[str]
    numbers: list[str]
    socials: list[str]
    street: list[str]
    region: str
    city: str
    postalCode: str


class CustomThreadArgs(TypedDict):
    # threadId: str
    urlQueue: queue.Queue
    fetchQueue: queue.Queue
    scrapeResults: dict[str, ScrapeResult]


class CustomThread(threading.Thread):
    def __init__(self, args: CustomThreadArgs, type: str):
        threading.Thread.__init__(self)
        self.args = args
        self.type = type

    def run(self):
        if self.type == 'producer':
            while 1:
                urlResult: UrlResult = self.args['urlQueue'].get(True)
                try:
                    data = requests.get(
                        urlResult['url'],
                        headers=headers,
                        timeout=5
                    )
                except Exception as e:
                    self.args['urlQueue'].task_done()
                    continue
                self.args['fetchQueue'].put({'isSubPage': urlResult['isSubPage'], 'data': data})
                self.args['urlQueue'].task_done()

        if self.type == 'consumer':
            while 1:
                fetchResult: FetchResult = self.args['fetchQueue'].get(True)
                if fetchResult['isSubPage']:
                    process_secondary(self.args, fetchResult['data'])
                else:
                    process_primary(self.args, fetchResult['data'])
                self.args['fetchQueue'].task_done()


def process_primary(args: CustomThreadArgs, fetchResult):
    try:
        url = sanitize_url(fetchResult.url)
        data = fetchResult.text

        soup = BeautifulSoup(data, 'html.parser')

        # Attempt to find contact page, otherwise attempt to find about page
        contactUrl = soup.find(
            lambda tag: tag.name == 'a' and 'contact' in tag.text.lower()
        ) or soup.find(lambda tag: tag.name == 'a' and 'About' in tag.text)

        # Prelim data
        description = soup.find('meta', {'name': 'description'})
        title = (soup.find('meta', {'property': 'og:site_name'}) or {}).get('content') # property="og:site_name"
        if (title == None or title == ""):
            title = (
                (
                    (soup.title.string or "")
                    .replace("Home | ", "")
                    .replace("Home - ", "")
                    .replace("Home page", "")
                    .replace("- Home", "")
                    .replace("| Home", "")
                    .replace(":::", "")
                    .replace(":: Home", "")
                    .replace(": Home", "")
                    .replace("Home ::", "")
                    .replace("Home :", "")
                )
                if soup.title
                else ""
            )
        favicon = soup.find(lambda tag: tag.name ==
                            'link' and 'icon' in tag['rel'])
        emails = re.findall(emailRegex, data)
        numbers = re.findall(numberRegex, data)
        socials = re.findall(urlRegex, data)
        street = re.findall(streetRegex, data)
        location = re.findall(locationRegex, data)

        args['scrapeResults'][url] = {
            'url': url,
            'title': title,
            'description': ((description.string or description.get('content')) if description else '') or '',
            'favicon': favicon['href'] if favicon else '',
            'emails': emails,
            'numbers': numbers,
            'socials': socials,
            'street': street
        }

        if (len(location) > 0):
            args['scrapeResults'][url]['city'] = location[0][0]
            args['scrapeResults'][url]['region'] = location[0][1]
            args['scrapeResults'][url]['postalCode'] = location[0][2]

        # Add contact page url to queue
        if (contactUrl and contactUrl.get('href')):
            contactUrl = contactUrl['href']

            if (not('http' in contactUrl or 'www' in contactUrl or '.com' in contactUrl)):
                contactUrl = url + contactUrl

            args['urlQueue'].put({'isSubPage': True, 'url': contactUrl})
    except Exception:
        print('An error occured processing data from ' + url)
        traceback.print_exc()


def process_secondary(args: CustomThreadArgs, fetchResult):
    try:
        data = fetchResult.text
        url = sanitize_url(fetchResult.url)

        soup = BeautifulSoup(data, 'html.parser')

        # Prelim data
        title = soup.title.string if soup.title else None
        emails = re.findall(emailRegex, data)
        numbers = re.findall(numberRegex, data)
        socials = re.findall(urlRegex, data)
        street = re.findall(streetRegex, data)
        location = re.findall(locationRegex, data)

        dataEntry = args['scrapeResults'][url] if url in args['scrapeResults'] else None

        if dataEntry is None:
            return

        dataEntry['emails'] = dataEntry['emails'] + emails
        dataEntry['numbers'] = dataEntry['numbers'] + numbers
        dataEntry['socials'] = dataEntry['socials'] + socials

        if (len(location) > 0 and not dataEntry.get('city')):
            dataEntry['city'] = location[0][0]
            dataEntry['region'] = location[0][1]

            if (not(location[0][0] == '' and location[0][1] == '' and ' ' not in location[0][2])):
                dataEntry['postalCode'] = location[0][2]
    except Exception as e:
        print('An error occured processing data from ' + url)
        print(e)
