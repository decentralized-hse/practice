import requests
import json
import time
from bs4 import BeautifulSoup

GITHUB_TOKEN = "" # insert your GitHub token here
TRENDING_PERIOD = "monthly"

DEFAULT_SETTINGS = {
    "limit": 5,
    "min_commits": 700,
    "min_contributors": 3,
    "code_threshold": 60.0
}

LANGUAGE_SETTINGS = {
    "python": {
        "min_commits": 200,
        "code_threshold": 50.0
    },
    "javascript": {},
    "go": {},
    "java": {}, 
    "rust": {},
    "haskell": {},
    "c++": {},
    "c": {},
    "swift": { },
    "kotlin": {}
}

API_HEADERS = {
    'Authorization': f'token {GITHUB_TOKEN}',
    'Accept': 'application/vnd.github.v3+json'
}

SCRAPE_HEADERS = {
    'User-Agent': 'Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/142.0.0.0 YaBrowser/25.12.0.0 Safari/537.36'
}

def get_config(lang, key):
    return LANGUAGE_SETTINGS.get(lang, {}).get(key, DEFAULT_SETTINGS[key])

def get_total_items_count(url):
    try:
        response = requests.get(f"{url}?per_page=1", headers=API_HEADERS)
        if response.status_code != 200:
            return 0
        
        if 'Link' in response.headers:
            links = response.headers['Link'].split(',')
            for link in links:
                if 'rel="last"' in link:
                    start = link.find("&page=") + 6
                    end = link.find(">", start)
                    return int(link[start:end])
        
        data = response.json()
        return len(data) if isinstance(data, list) else 0
    except:
        return 0

def is_language_dominant(owner, name, target_lang):
    threshold = get_config(target_lang, "code_threshold")
    url = f"https://api.github.com/repos/{owner}/{name}/languages"
    
    try:
        response = requests.get(url, headers=API_HEADERS)
        if response.status_code != 200:
            return False
            
        languages = response.json()
        total_bytes = sum(languages.values())
        if total_bytes == 0:
            return False
            
        match_bytes = 0
        for lang, count in languages.items():
            if lang.lower() == target_lang.lower():
                match_bytes = count
                break
        
        return (match_bytes / total_bytes * 100) >= threshold
    except:
        return False

def has_enough_contributors(owner, name, target_lang):
    min_contributors = get_config(target_lang, "min_contributors")
    url = f"https://api.github.com/repos/{owner}/{name}/contributors"
    count = get_total_items_count(url)
    return count >= min_contributors

def has_enough_commits(owner, name, target_lang):
    min_commits = get_config(target_lang, "min_commits")
    url = f"https://api.github.com/repos/{owner}/{name}/commits"
    count = get_total_items_count(url)
    return count >= min_commits

def validate_repository(owner, name, language):
    if not is_language_dominant(owner, name, language):
        return False
    
    if not has_enough_contributors(owner, name, language):
        return False

    if not has_enough_commits(owner, name, language):
        return False

    return True

def scrape_trending_candidates(language, since):
    url = f"https://github.com/trending/{language}?since={since}"
    candidates = []
    try:
        response = requests.get(url, headers=SCRAPE_HEADERS)
        soup = BeautifulSoup(response.text, 'html.parser')
        rows = soup.select('article.Box-row h2 a')
        
        for row in rows:
            href = row['href'].strip()
            full_name = href[1:]
            candidates.append(full_name)
    except Exception as e:
        print(f"Scraping error for {language}: {e}")
    
    return candidates

def collect_data():
    dataset = {}
    
    for lang in LANGUAGE_SETTINGS.keys():
        dataset[lang] = []
        limit = get_config(lang, "limit")
        
        candidates = scrape_trending_candidates(lang, TRENDING_PERIOD)
        
        print(f"Processing {lang} (Limit: {limit}, Min Commits: {get_config(lang, 'min_commits')}). Found {len(candidates)} candidates...")
        
        for full_name in candidates:
            if len(dataset[lang]) >= limit:
                break
                
            owner, name = full_name.split('/')

            print(f"  Checking: https://github.com/{full_name}")
            
            if validate_repository(owner, name, lang):
                repo_url = f"https://github.com/{full_name}"
                dataset[lang].append(repo_url)
                print(f"  [+] Accepted: {full_name}")
            else:
                print(f"  [-] Rejected: {full_name}")
            
            time.sleep(0.5)
            
    return dataset

if __name__ == "__main__":
    if not GITHUB_TOKEN:
        raise ValueError("Set GitHub token for correct work")

    results = collect_data()
    
    with open('trending_repos.json', 'w', encoding='utf-8') as f:
        json.dump(results, f, indent=4)
