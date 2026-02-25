#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <string>
#include <map>
#include <set>
#include <algorithm>
#include <ctime>
#include <iomanip>
#include <chrono>

#include <git2.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
namespace fs = std::filesystem;
using namespace std::chrono;

int MAX_COMMITS = 50;
std::string INPUT_FILE = "input.json";
std::string OUTPUT_DIR = "./results";
const std::string TEMP_DIR = "./_temp_repos";

std::map<std::string, std::set<std::string>> LANGUAGE_EXTENSIONS = {
    {"python",     {".py"}},
    {"javascript", {".js", ".jsx", ".mjs", ".ts"}},
    {"go",         {".go"}},
    {"java",       {".java"}},
    {"rust",       {".rs"}},
    {"haskell",    {".hs", ".lhs"}},
    {"c++",        {".cpp", ".hpp", ".cc", ".cxx", ".h", ".c"}},
    {"c",          {".c", ".h"}},
    {"swift",      {".swift"}},
    {"kotlin",     {".kt", ".kts"}}
};

const std::vector<std::string> IGNORED_PATHS = {
    "docs/", "documentation/", "vendor/", "node_modules/", 
    "test/", "tests/", "dist/", "build/", "assets/", "examples/",
    "cmake/", ".github/", ".idea/", ".vscode/"
};

void log_info(const std::string& msg) {
    std::cout << "[INFO] " << msg << std::endl;
}

void log_warn(const std::string& msg) {
    std::cout << "[WARN] " << msg << std::endl;
}

void log_error(const std::string& msg) {
    std::cerr << "[ERROR] " << msg << std::endl;
}

struct GitResource {
    git_repository* repo = nullptr;
    git_revwalk* walker = nullptr;
    git_commit* commit = nullptr;
    git_commit* parent = nullptr;
    git_tree* tree = nullptr;
    git_tree* parent_tree = nullptr;
    git_diff* diff = nullptr;
    git_patch* patch = nullptr;

    ~GitResource() {
        if (patch) git_patch_free(patch);
        if (diff) git_diff_free(diff);
        if (tree) git_tree_free(tree);
        if (parent_tree) git_tree_free(parent_tree);
        if (commit) git_commit_free(commit);
        if (parent) git_commit_free(parent);
        if (walker) git_revwalk_free(walker);
        if (repo) git_repository_free(repo);
    }
};

bool contains(const std::string& str, const std::string& substr) {
    return str.find(substr) != std::string::npos;
}

bool is_valid_file(const std::string& filepath, const std::string& lang_key) {
    std::string lower_path = filepath;
    std::transform(lower_path.begin(), lower_path.end(), lower_path.begin(), ::tolower);

    for (const auto& ignored : IGNORED_PATHS) {
        if (contains(lower_path, ignored)) return false;
    }

    std::string ext = fs::path(filepath).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (LANGUAGE_EXTENSIONS.count(lang_key)) {
        const auto& valid_exts = LANGUAGE_EXTENSIONS.at(lang_key);
        return valid_exts.find(ext) != valid_exts.end();
    }
    return false;
}

std::string get_reponame_from_url(const std::string& url) {
    size_t last_slash = url.rfind('/');
    if (last_slash == std::string::npos) return "unknown";
    
    std::string name = url.substr(last_slash + 1);
    if (name.size() > 4 && name.substr(name.size() - 4) == ".git") {
        name = name.substr(0, name.size() - 4);
    }
    
    size_t second_slash = url.rfind('/', last_slash - 1);
    if (second_slash != std::string::npos) {
        std::string owner = url.substr(second_slash + 1, last_slash - second_slash - 1);
        return owner + "_" + name;
    }
    return name;
}

void parse_arguments(int argc, char* argv[]) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-i" && i + 1 < argc) {
            INPUT_FILE = argv[++i];
        } else if (arg == "-d" && i + 1 < argc) {
            OUTPUT_DIR = argv[++i];
        } else if (arg == "-n" && i + 1 < argc) {
            MAX_COMMITS = std::stoi(argv[++i]);
        }
    }
}

json analyze_single_repo(const std::string& url, const std::string& repo_dir, const std::string& lang) {
    GitResource res;
    json commits_array = json::array();

    int error = git_repository_open(&res.repo, repo_dir.c_str());
    if (error < 0) {
        std::cout << "     [Stage: CLONE] Started..." << std::flush;
        auto clone_start = high_resolution_clock::now();
        
        git_clone_options clone_opts = GIT_CLONE_OPTIONS_INIT;
        error = git_clone(&res.repo, url.c_str(), repo_dir.c_str(), &clone_opts);
        
        auto clone_end = high_resolution_clock::now();
        double clone_ms = duration<double, std::milli>(clone_end - clone_start).count();
        
        if (error < 0) {
            std::cout << " FAILED" << std::endl;
            std::string err_msg = git_error_last() ? git_error_last()->message : "Unknown error";
            log_error("Failed to clone " + url + ": " + err_msg);
            return commits_array;
        }
        std::cout << " Done (" << std::fixed << std::setprecision(2) << clone_ms / 1000.0 << "s)" << std::endl;
    } else {
        std::cout << "     [Stage: CLONE] Skipped (Repo exists)" << std::endl;
    }

    std::cout << "     [Stage: METRICS] Analyzing commits..." << std::flush;
    auto metrics_start = high_resolution_clock::now();

    git_revwalk_new(&res.walker, res.repo);
    git_revwalk_sorting(res.walker, GIT_SORT_TOPOLOGICAL | GIT_SORT_TIME);
    git_revwalk_push_head(res.walker);

    git_oid oid;
    int count = 0;

    while (git_revwalk_next(&oid, res.walker) == 0) {
        if (MAX_COMMITS > 0 && count >= MAX_COMMITS) break;

        if (res.commit) { git_commit_free(res.commit); res.commit = nullptr; }
        if (res.parent) { git_commit_free(res.parent); res.parent = nullptr; }
        if (res.tree) { git_tree_free(res.tree); res.tree = nullptr; }
        if (res.parent_tree) { git_tree_free(res.parent_tree); res.parent_tree = nullptr; }
        if (res.diff) { git_diff_free(res.diff); res.diff = nullptr; }
        if (res.patch) { git_patch_free(res.patch); res.patch = nullptr; }

        git_commit_lookup(&res.commit, res.repo, &oid);

        if (git_commit_parentcount(res.commit) > 1) continue;

        const git_signature* author = git_commit_author(res.commit);
        time_t commit_time = author->when.time + (author->when.offset * 60);
        struct tm* timeinfo = gmtime(&commit_time);
        
        char day_buf[20];
        strftime(day_buf, sizeof(day_buf), "%A", timeinfo);

        git_commit_tree(&res.tree, res.commit);
        
        if (git_commit_parentcount(res.commit) > 0) {
            git_commit_parent(&res.parent, res.commit, 0);
            git_commit_tree(&res.parent_tree, res.parent);
        }
        
        git_diff_tree_to_tree(&res.diff, res.repo, res.parent_tree, res.tree, nullptr);

        size_t num_deltas = git_diff_num_deltas(res.diff);
        
        int lines_added = 0;
        int lines_deleted = 0;
        int files_changed_count = 0;
        std::vector<std::string> changed_files;

        for (size_t i = 0; i < num_deltas; ++i) {
            const git_diff_delta* delta = git_diff_get_delta(res.diff, i);
            std::string filepath = delta->new_file.path;

            if (!is_valid_file(filepath, lang)) continue;

            if (res.patch) { git_patch_free(res.patch); res.patch = nullptr; }
            
            if (git_patch_from_diff(&res.patch, res.diff, i) == 0 && res.patch) {
                size_t add = 0, del = 0, ctx = 0;
                git_patch_line_stats(&ctx, &add, &del, res.patch);
                
                lines_added += add;
                lines_deleted += del;
                files_changed_count++;
                changed_files.push_back(filepath);
            }
        }

        if (files_changed_count > 0) {
            json commit_obj;
            commit_obj["hash"] = git_oid_tostr_s(&oid);
            commit_obj["commit_size"] = lines_added + lines_deleted;
            commit_obj["weekday"] = day_buf;
            commit_obj["hour"] = timeinfo->tm_hour;
            commit_obj["author"] = author->name;
            commit_obj["lines_added"] = lines_added;
            commit_obj["lines_deleted"] = lines_deleted;
            commit_obj["files_count"] = files_changed_count;
            commit_obj["files"] = changed_files;
            commits_array.push_back(commit_obj);
            count++;
        }
    }

    auto metrics_end = high_resolution_clock::now();
    double metrics_ms = duration<double, std::milli>(metrics_end - metrics_start).count();
    
    std::cout << " Done (" << count << " commits, " << std::fixed << std::setprecision(2) << metrics_ms / 1000.0 << "s)" << std::endl;

    return commits_array;
}

int main(int argc, char* argv[]) {
    parse_arguments(argc, argv);

    log_info("Configuration:");
    std::cout << "  Input File:     " << INPUT_FILE << std::endl;
    std::cout << "  Output Root:    " << OUTPUT_DIR << std::endl;
    std::string limit_str = (MAX_COMMITS == 0) ? "Unlimited" : std::to_string(MAX_COMMITS);
    std::cout << "  Commits Limit:  " << limit_str << std::endl;

    git_libgit2_init();

    json input_data;
    try {
        std::ifstream f(INPUT_FILE);
        if (!f.good()) {
            log_error("Input file not found: " + INPUT_FILE);
            return 1;
        }
        input_data = json::parse(f);
    } catch (const std::exception& e) {
        log_error("JSON parsing error: " + std::string(e.what()));
        return 1;
    }

    if (!fs::exists(TEMP_DIR)) fs::create_directories(TEMP_DIR);
    if (!fs::exists(OUTPUT_DIR)) fs::create_directories(OUTPUT_DIR);

    int lang_idx = 0;
    int total_langs = input_data.size();

    for (auto& [lang, urls] : input_data.items()) {
        lang_idx++;
        log_info("[" + std::to_string(lang_idx) + "/" + std::to_string(total_langs) + "] Processing language: " + lang);
        
        std::string lang_dir = OUTPUT_DIR + "/" + lang;
        if (!fs::exists(lang_dir)) {
            fs::create_directories(lang_dir);
        }

        if (urls.is_array()) {
            int repo_idx = 0;
            int total_repos = urls.size();

            for (const auto& url_val : urls) {
                repo_idx++;
                std::string url = url_val.get<std::string>();
                std::string repo_name = get_reponame_from_url(url);
                std::string local_path = TEMP_DIR + "/" + repo_name;
                
                // Проверяем наличие выходного файла ПЕРЕД началом работы
                std::string output_filename = lang_dir + "/" + repo_name + ".json";
                
                if (fs::exists(output_filename)) {
                    std::cout << "  -> (" << repo_idx << "/" << total_repos << ") " << repo_name 
                              << " [SKIPPED - Metric file exists]" << std::endl;
                    continue;
                }

                std::cout << "  -> (" << repo_idx << "/" << total_repos << ") " << repo_name << std::endl;
                
                json commits = analyze_single_repo(url, local_path, lang);

                json repo_output;
                repo_output["language"] = lang;
                repo_output["repo_name"] = repo_name;
                repo_output["url"] = url;
                repo_output["commits"] = commits;

                std::ofstream out_file(output_filename);
                out_file << repo_output.dump(4);
            }
        }
    }

    log_info("All Analysis Completed.");
    git_libgit2_shutdown();
    return 0;
}
