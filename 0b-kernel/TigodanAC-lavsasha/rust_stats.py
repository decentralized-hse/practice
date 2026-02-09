import os
import json
import subprocess
import requests
from datetime import datetime


def get_version_date_from_api(version_tag):
    if version_tag.startswith("linux-"):
        version_tag = "v" + version_tag[6:]
    
    url = f"https://api.github.com/repos/torvalds/linux/releases/tags/{version_tag}"
    token = os.getenv('GITHUB_TOKEN')
    headers = {'Accept': 'application/vnd.github.v3+json'}
    if token:
        headers['Authorization'] = f'token {token}'
    
    try:
        response = requests.get(url, headers=headers, timeout=10)
        if response.status_code == 200:
            data = response.json()
            date_str = data.get('published_at', '')
            if date_str:
                return date_str.split('T')[0]
        
        commit_url = f"https://api.github.com/repos/torvalds/linux/commits/{version_tag}"
        response = requests.get(commit_url, headers=headers, timeout=10)
        if response.status_code == 200:
            data = response.json()
            date_str = data.get('commit', {}).get('author', {}).get('date', '')
            if date_str:
                return date_str.split('T')[0]
                
    except Exception as e:
        print(f"Не удалось получить дату для {version_tag}: {e}")
    
    return None

def analyze_version(version_dir):
    if not os.path.exists(version_dir):
        return None
    
    cmd_files = f"find {version_dir} -name '*.rs' -type f | wc -l"
    result_files = subprocess.run(cmd_files, shell=True, capture_output=True, text=True)
    if result_files.returncode != 0:
        file_count = 0
    else:
        file_count = int(result_files.stdout.strip()) if result_files.stdout.strip() else 0
    
    line_count = 0
    if file_count > 0:
        cmd_lines = f"find {version_dir} -name '*.rs' -type f -exec cat {{}} + | wc -l"
        result_lines = subprocess.run(cmd_lines, shell=True, capture_output=True, text=True)
        
        if result_lines.returncode == 0 and result_lines.stdout.strip():
            line_count = int(result_lines.stdout.strip())
    else:
        return file_count, line_count
    
    return file_count, line_count

def main():
    version_folders = []
    for item in os.listdir('.'):
        if os.path.isdir(item) and item.startswith('linux-'):
            version_folders.append(item)
    
    results = {}
    date_cache = {}
    
    fallback_dates = {
        "linux-5.16-rc8": "2021-12-05",
        "linux-6.2-rc2": "2023-01-15",
        "linux-6.7": "2024-01-07",
        "linux-6.13-rc6": "2025-05-18",
        "linux-6.19-rc4": "2025-12-05",
    }
    
    for version_dir in sorted(version_folders):
        if version_dir not in date_cache:
            api_date = get_version_date_from_api(version_dir)
            if api_date:
                date_cache[version_dir] = api_date
            else:
                date_cache[version_dir] = fallback_dates.get(version_dir, "0000-00-00")
        
        stats = analyze_version(version_dir)
        if stats is not None:
            file_count, line_count = stats
            version_display = version_dir.replace("linux-", "")
            
            results[date_cache[version_dir]] = {
                "files": file_count,
                "lines": line_count,
                "version": version_display,
                "folder": version_dir
            }

    sorted_dates = sorted(results.keys())
    output_file = "rust_kernel_stats.json"
    with open(output_file, 'w', encoding='utf-8') as f:
        sorted_results = {date: results[date] for date in sorted_dates}
        json.dump(sorted_results, f, indent=2, ensure_ascii=False)

if __name__ == "__main__":
    main()