#include <algorithm>
#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <getopt.h>
#include <sys/wait.h>
#include <unistd.h>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace fs = std::filesystem;

static const char* OFFICIAL_REPO_URL = "https://github.com/torvalds/linux.git";
static bool g_verbose = false;

static void log_info(const char* fmt, ...) {
    if (!g_verbose) return;
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "INFO: ");
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);
}

static void log_debug(const char* fmt, ...) {
    if (!g_verbose) return;
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "DEBUG: ");
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);
}

static std::string run_cmd(const std::string& cmd) {
    log_debug("Running command: %s", cmd.c_str());
    std::string full_cmd = g_verbose ? cmd : cmd + " 2>/dev/null";
    FILE* fp = popen(full_cmd.c_str(), "r");
    if (!fp) {
        throw std::runtime_error("popen failed: " + cmd);
    }
    std::string output;
    char buf[4096];
    while (fgets(buf, sizeof(buf), fp)) {
        output += buf;
    }
    int status = pclose(fp);
    if (status == -1) {
        throw std::runtime_error("pclose failed: " + cmd);
    }
    if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
        throw std::runtime_error("Command failed (exit " + std::to_string(WEXITSTATUS(status)) + "): " + cmd);
    }
    return output;
}

static std::string shell_escape(const std::string& s) {
    std::string out = "'";
    for (char c : s) {
        if (c == '\'') out += "'\\''";
        else out += c;
    }
    out += "'";
    return out;
}

static std::string build_git_cmd(const std::vector<std::string>& args,
                                  const std::string& git_dir) {
    std::string cmd = "git";
    if (!git_dir.empty()) {
        cmd += " --git-dir " + shell_escape(git_dir);
    }
    for (auto& a : args) {
        cmd += " " + shell_escape(a);
    }
    return cmd;
}

static void ensure_repo(const std::string& repo_path, bool refresh) {
    fs::path dot_git = fs::path(repo_path) / ".git";
    if (fs::is_directory(dot_git) || fs::is_regular_file(dot_git)) {
        if (refresh) {
            log_info("Fetching updates in %s", repo_path.c_str());
            std::string cmd = "cd " + shell_escape(repo_path)
                + " && git fetch --all --prune";
            run_cmd(cmd);
        }
        return;
    }
    fs::path parent = fs::path(repo_path).parent_path();
    if (!parent.empty()) fs::create_directories(parent);

    log_info("Cloning official Linux repo to %s", repo_path.c_str());
    std::string cmd = "git clone --no-tags --filter=blob:none "
        + shell_escape(OFFICIAL_REPO_URL) + " " + shell_escape(repo_path);
    run_cmd(cmd);
}

static std::string resolve_git_dir(const std::string& git_file) {
    if (fs::is_directory(git_file)) return git_file;
    if (fs::is_regular_file(git_file)) {
        std::ifstream f(git_file);
        std::string data((std::istreambuf_iterator<char>(f)),
                          std::istreambuf_iterator<char>());
        while (!data.empty() && (data.back() == '\n' || data.back() == '\r' || data.back() == ' '))
            data.pop_back();
        if (data.rfind("gitdir:", 0) == 0) {
            std::string raw = data.substr(7);
            size_t start = raw.find_first_not_of(' ');
            if (start != std::string::npos) raw = raw.substr(start);
            if (raw.empty() || raw[0] != '/') {
                raw = (fs::path(git_file).parent_path() / raw).string();
            }
            if (fs::is_directory(raw)) return raw;
        }
    }
    throw std::runtime_error("Invalid git file or directory: " + git_file);
}

static std::string email_to_org(const std::string& email) {
    std::string e = email;
    while (!e.empty() && (e.back() == ' ' || e.back() == '\n' || e.back() == '\r'))
        e.pop_back();
    size_t start = e.find_first_not_of(' ');
    if (start != std::string::npos) e = e.substr(start);
    for (auto& c : e) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
    auto at = e.find('@');
    if (at == std::string::npos) return "unknown";
    return e.substr(at + 1);
}

struct OrgStats {
    long long commits = 0;
    long long added   = 0;
    long long deleted = 0;
    long long total   = 0;
};

static long long get_commit_count(const std::string& repo_path,
                                   const std::string& git_dir) {
    std::string cmd;
    if (!git_dir.empty()) {
        cmd = build_git_cmd({"rev-list", "--count", "HEAD"}, git_dir);
    } else {
        cmd = "cd " + shell_escape(repo_path) + " && git rev-list --count HEAD";
    }
    std::string out = run_cmd(cmd);
    while (!out.empty() && (out.back() == '\n' || out.back() == ' '))
        out.pop_back();
    if (out.empty()) return 0;
    return std::stoll(out);
}

static std::unordered_map<std::string, OrgStats> collect_stats(
    const std::string& repo_path,
    const std::string& git_dir,
    bool fast,
    int progress_every)
{
    std::string location = git_dir.empty() ? repo_path : git_dir;
    log_info("Collecting commit stats from %s", location.c_str());

    long long total_commits = -1;
    try {
        total_commits = get_commit_count(repo_path, git_dir);
    } catch (...) {
        log_debug("Could not determine total commit count.");
    }

    std::string cmd;
    if (!git_dir.empty()) {
        cmd = "git --git-dir " + shell_escape(git_dir)
            + " log --numstat '--format=@@@%ae'";
    } else {
        cmd = "cd " + shell_escape(repo_path)
            + " && git log --numstat '--format=@@@%ae'";
    }
    if (fast) cmd += " --no-renames --no-ext-diff";
    cmd += " 2>/dev/null";

    FILE* fp = popen(cmd.c_str(), "r");
    if (!fp) throw std::runtime_error("popen failed for git log");

    std::unordered_map<std::string, OrgStats> stats;
    std::string current_org;
    bool has_org = false;
    long long commit_index = 0;

    char* line_buf = nullptr;
    size_t line_cap = 0;
    ssize_t line_len;

    while ((line_len = getline(&line_buf, &line_cap, fp)) != -1) {
        if (line_len > 0 && line_buf[line_len - 1] == '\n') {
            line_buf[line_len - 1] = '\0';
            line_len--;
        }

        if (line_len >= 3 && line_buf[0] == '@' && line_buf[1] == '@' && line_buf[2] == '@') {
            std::string email(line_buf + 3);
            current_org = email_to_org(email);
            has_org = true;
            stats[current_org].commits++;
            commit_index++;
            if (progress_every > 0 && commit_index % progress_every == 0) {
                if (total_commits > 0) {
                    double pct = (static_cast<double>(commit_index) / total_commits) * 100.0;
                    log_info("Progress: %lld/%lld commits (%.1f%%)",
                             commit_index, total_commits, pct);
                } else {
                    log_info("Progress: %lld commits", commit_index);
                }
            }
            continue;
        }

        if (line_len == 0 || !has_org) continue;

        char* p1 = strchr(line_buf, '\t');
        if (!p1) continue;
        char* p2 = strchr(p1 + 1, '\t');
        if (!p2) continue;

        *p1 = '\0';
        *p2 = '\0';
        const char* added_s = line_buf;
        const char* deleted_s = p1 + 1;

        if (added_s[0] == '-' || deleted_s[0] == '-') continue;

        char* end1;
        char* end2;
        long long added_v = strtoll(added_s, &end1, 10);
        long long deleted_v = strtoll(deleted_s, &end2, 10);
        if (*end1 != '\0' || *end2 != '\0') continue;

        OrgStats& os = stats[current_org];
        os.added   += added_v;
        os.deleted += deleted_v;
        os.total   += added_v + deleted_v;
    }

    free(line_buf);
    int status = pclose(fp);
    if (status == -1) {
        throw std::runtime_error("pclose failed for git log");
    }
    if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
        throw std::runtime_error("git log failed (exit " + std::to_string(WEXITSTATUS(status)) + ")");
    }

    if (total_commits > 0 && commit_index > 0 && commit_index != total_commits) {
        log_info("Progress: %lld/%lld commits (100%%)", commit_index, total_commits);
    }
    log_info("Aggregated stats for %zu orgs", stats.size());
    return stats;
}

static void format_table(
    const std::vector<std::vector<std::string>>& rows,
    const std::vector<std::string>& headers)
{
    std::vector<size_t> widths(headers.size());
    for (size_t i = 0; i < headers.size(); i++)
        widths[i] = headers[i].size();
    for (auto& row : rows)
        for (size_t i = 0; i < row.size() && i < widths.size(); i++)
            widths[i] = std::max(widths[i], row[i].size());

    auto print_row = [&](const std::vector<std::string>& row) {
        for (size_t i = 0; i < row.size(); i++) {
            if (i > 0) std::cout << "  ";
            std::cout << row[i];
            if (i + 1 < row.size()) {
                size_t pad = widths[i] - row[i].size();
                for (size_t p = 0; p < pad; p++) std::cout << ' ';
            }
        }
        std::cout << '\n';
    };

    print_row(headers);
    std::vector<std::string> sep(headers.size());
    for (size_t i = 0; i < widths.size(); i++)
        sep[i] = std::string(widths[i], '-');
    print_row(sep);
    for (auto& row : rows) print_row(row);
}

static std::string format_markdown_table(
    const std::vector<std::vector<std::string>>& rows,
    const std::vector<std::string>& headers)
{
    std::ostringstream out;
    auto print_row = [&](const std::vector<std::string>& row) {
        out << "|";
        for (auto& col : row) {
            out << " " << col << " |";
        }
        out << "\n";
    };
    print_row(headers);
    out << "|";
    for (size_t i = 0; i < headers.size(); i++) out << " --- |";
    out << "\n";
    for (auto& row : rows) print_row(row);
    return out.str();
}

static std::string format_csv(
    const std::vector<std::vector<std::string>>& rows,
    const std::vector<std::string>& headers)
{
    auto escape = [](const std::string& s) {
        bool need = s.find_first_of("\",\n") != std::string::npos;
        std::string out = s;
        size_t pos = 0;
        while ((pos = out.find('"', pos)) != std::string::npos) {
            out.insert(pos, "\"");
            pos += 2;
        }
        if (need) out = "\"" + out + "\"";
        return out;
    };
    std::ostringstream out;
    for (size_t i = 0; i < headers.size(); i++) {
        if (i) out << ",";
        out << escape(headers[i]);
    }
    out << "\n";
    for (auto& row : rows) {
        for (size_t i = 0; i < row.size(); i++) {
            if (i) out << ",";
            out << escape(row[i]);
        }
        out << "\n";
    }
    return out.str();
}

static std::string html_escape(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        switch (c) {
        case '&': out += "&amp;"; break;
        case '<': out += "&lt;"; break;
        case '>': out += "&gt;"; break;
        case '"': out += "&quot;"; break;
        case '\'': out += "&#39;"; break;
        default: out += c; break;
        }
    }
    return out;
}

static std::string format_html(
    const std::vector<std::vector<std::string>>& rows,
    const std::vector<std::string>& headers,
    const std::string& title)
{
    std::ostringstream out;
    out << "<!doctype html><html><head><meta charset=\"utf-8\">";
    out << "<title>" << html_escape(title) << "</title>";
    out << "<style>"
           "body{font-family:Arial,Helvetica,sans-serif;margin:24px;}"
           "table{border-collapse:collapse;width:100%;}"
           "th,td{border:1px solid #ccc;padding:6px 8px;text-align:left;}"
           "th{background:#f4f4f4;}"
           "caption{font-weight:bold;margin-bottom:8px;}"
           "</style></head><body>";
    out << "<table><caption>" << html_escape(title) << "</caption><thead><tr>";
    for (auto& h : headers) out << "<th>" << html_escape(h) << "</th>";
    out << "</tr></thead><tbody>";
    for (auto& row : rows) {
        out << "<tr>";
        for (auto& col : row) out << "<td>" << html_escape(col) << "</td>";
        out << "</tr>";
    }
    out << "</tbody></table></body></html>";
    return out.str();
}

static std::string to_lower(std::string s) {
    for (auto& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

static bool ends_with(const std::string& s, const std::string& suffix) {
    if (s.size() < suffix.size()) return false;
    return std::equal(suffix.rbegin(), suffix.rend(), s.rbegin());
}

static bool command_exists(const std::string& name) {
    std::string cmd = "command -v " + shell_escape(name) + " >/dev/null 2>&1";
    int rc = std::system(cmd.c_str());
    return rc == 0;
}

static void write_file(const std::string& path, const std::string& data) {
    std::ofstream out(path);
    if (!out) throw std::runtime_error("Failed to write file: " + path);
    out << data;
}

static void usage(const char* prog) {
    fprintf(stderr,
        "Usage: %s [options]\n"
        "  -v, --verbose          Enable verbose logging\n"
        "      --repo-path PATH   Path to linux repo (default: ./linux-github)\n"
        "      --git-file PATH    Path to .git dir or gitfile\n"
        "      --refresh          Fetch updates if repo exists\n"
        "      --top N            Number of orgs to show (default: 100)\n"
        "      --loc-metric M     added|deleted|total (default: total)\n"
        "      --output PATH      Write table to file\n"
        "      --output-format F  text|markdown|csv|html (auto by extension)\n"
        "      --fast             Disable rename detection and ext diff (default)\n"
        "      --no-fast          Enable rename detection and ext diff\n"
        "      --progress-every N Log progress every N commits (default: 5000)\n",
        prog);
}

int main(int argc, char* argv[]) {
    std::string repo_path = (fs::current_path() / "linux-github").string();
    std::string git_file;
    bool refresh = false;
    int top_n = 100;
    std::string loc_metric = "total";
    std::string output_path;
    std::string output_format;
    bool fast = true;
    int progress_every = 5000;

    enum LongOpt {
        OPT_REPO_PATH = 1000,
        OPT_GIT_FILE,
        OPT_REFRESH,
        OPT_TOP,
        OPT_LOC_METRIC,
        OPT_OUTPUT,
        OPT_OUTPUT_FORMAT,
        OPT_FAST,
        OPT_NO_FAST,
        OPT_PROGRESS_EVERY,
    };

    static struct option long_options[] = {
        {"verbose",        no_argument,       nullptr, 'v'},
        {"repo-path",      required_argument, nullptr, OPT_REPO_PATH},
        {"git-file",       required_argument, nullptr, OPT_GIT_FILE},
        {"refresh",        no_argument,       nullptr, OPT_REFRESH},
        {"top",            required_argument, nullptr, OPT_TOP},
        {"loc-metric",     required_argument, nullptr, OPT_LOC_METRIC},
        {"output",         required_argument, nullptr, OPT_OUTPUT},
        {"output-format",  required_argument, nullptr, OPT_OUTPUT_FORMAT},
        {"fast",           no_argument,       nullptr, OPT_FAST},
        {"no-fast",        no_argument,       nullptr, OPT_NO_FAST},
        {"progress-every", required_argument, nullptr, OPT_PROGRESS_EVERY},
        {nullptr, 0, nullptr, 0},
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "v", long_options, nullptr)) != -1) {
        switch (opt) {
        case 'v':
            g_verbose = true;
            break;
        case OPT_REPO_PATH:
            repo_path = optarg;
            break;
        case OPT_GIT_FILE:
            git_file = optarg;
            break;
        case OPT_REFRESH:
            refresh = true;
            break;
        case OPT_TOP:
            try { top_n = std::stoi(optarg); }
            catch (...) {
                fprintf(stderr, "Error: --top requires a numeric argument\n");
                return 1;
            }
            break;
        case OPT_LOC_METRIC:
            loc_metric = optarg;
            if (loc_metric != "added" && loc_metric != "deleted" && loc_metric != "total") {
                fprintf(stderr, "Error: --loc-metric must be added, deleted, or total\n");
                return 1;
            }
            break;
        case OPT_OUTPUT:
            output_path = optarg;
            break;
        case OPT_OUTPUT_FORMAT:
            output_format = to_lower(optarg);
            break;
        case OPT_FAST:
            fast = true;
            break;
        case OPT_NO_FAST:
            fast = false;
            break;
        case OPT_PROGRESS_EVERY:
            try { progress_every = std::stoi(optarg); }
            catch (...) {
                fprintf(stderr, "Error: --progress-every requires a numeric argument\n");
                return 1;
            }
            break;
        default:
            usage(argv[0]);
            return 1;
        }
    }

    try {
        std::string git_dir;
        std::string rp = repo_path;

        if (!git_file.empty()) {
            git_dir = resolve_git_dir(git_file);
            rp.clear();
        } else {
            ensure_repo(rp, refresh);
        }

        auto stats = collect_stats(rp, git_dir, fast, progress_every);

        using Entry = std::pair<std::string, OrgStats>;
        std::vector<Entry> entries(stats.begin(), stats.end());

        auto get_metric = [&](const OrgStats& s) -> long long {
            if (loc_metric == "added") return s.added;
            if (loc_metric == "deleted") return s.deleted;
            return s.total;
        };

        std::sort(entries.begin(), entries.end(),
            [&](const Entry& a, const Entry& b) {
                if (a.second.commits != b.second.commits)
                    return a.second.commits > b.second.commits;
                long long ma = get_metric(a.second), mb = get_metric(b.second);
                if (ma != mb) return ma > mb;
                return a.first < b.first;
            });

        if (static_cast<int>(entries.size()) > top_n)
            entries.resize(top_n);

        std::vector<std::string> headers = {"#", "org", "commits", "added", "deleted", "total"};
        std::vector<std::vector<std::string>> rows;
        for (size_t i = 0; i < entries.size(); i++) {
            auto& [org, s] = entries[i];
            rows.push_back({
                std::to_string(i + 1),
                org,
                std::to_string(s.commits),
                std::to_string(s.added),
                std::to_string(s.deleted),
                std::to_string(s.total),
            });
        }

        std::string title = "Top " + std::to_string(entries.size())
            + " kernel contributors by org (email domain)";
        if (!output_path.empty() && output_format.empty()) {
            std::string lower = to_lower(output_path);
            if (ends_with(lower, ".md") || ends_with(lower, ".markdown")) output_format = "markdown";
            else if (ends_with(lower, ".csv")) output_format = "csv";
            else if (ends_with(lower, ".html") || ends_with(lower, ".htm")) output_format = "html";
            else if (ends_with(lower, ".pdf")) output_format = "pdf";
            else if (ends_with(lower, ".png")) output_format = "png";
            else output_format = "text";
        }

        if (output_path.empty()) {
            format_table(rows, headers);
        } else if (output_format == "text") {
            std::ostringstream out;
            auto old_buf = std::cout.rdbuf(out.rdbuf());
            format_table(rows, headers);
            std::cout.rdbuf(old_buf);
            write_file(output_path, out.str());
        } else if (output_format == "markdown") {
            write_file(output_path, format_markdown_table(rows, headers));
        } else if (output_format == "csv") {
            write_file(output_path, format_csv(rows, headers));
        } else if (output_format == "html") {
            write_file(output_path, format_html(rows, headers, title));
        } else if (output_format == "pdf" || output_format == "png") {
            std::string html = format_html(rows, headers, title);
            std::string tmp = (fs::temp_directory_path()
                / ("kernel_contribs_" + std::to_string(::getpid()) + ".html")).string();
            write_file(tmp, html);
            std::string tool = (output_format == "pdf") ? "wkhtmltopdf" : "wkhtmltoimage";
            if (!command_exists(tool)) {
                throw std::runtime_error("Missing " + tool + " in PATH (install it or use --output-format html).");
            }
            std::string cmd = tool + " " + shell_escape(tmp) + " " + shell_escape(output_path);
            run_cmd(cmd);
            fs::remove(tmp);
        } else {
            throw std::runtime_error("Unknown output format: " + output_format);
        }
    } catch (const std::exception& e) {
        fprintf(stderr, "Error: %s\n", e.what());
        return 1;
    }

    return 0;
}
