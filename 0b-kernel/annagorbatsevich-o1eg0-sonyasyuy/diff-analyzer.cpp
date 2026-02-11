#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <getopt.h>

struct DirStats {
    int64_t added   = 0;
    int64_t removed = 0;
    int64_t total   = 0;
    int     files   = 0;
};

class DiffAnalyzer {
private:
    std::map<std::string, DirStats> dirStats;
    int64_t totalAdded   = 0;
    int64_t totalRemoved = 0;
    int64_t totalChanges = 0;
    int     totalFiles   = 0;
    int     depth        = 1;       // directory grouping depth
    int     topN         = 0;       // 0 = show all

    std::string getDirAtDepth(const std::string& filepath) const {
        if (filepath.empty()) return "(root)";
        size_t pos = 0;
        for (int d = 0; d < depth; ++d) {
            pos = filepath.find('/', pos);
            if (pos == std::string::npos) {
                // file lives above requested depth — use its parent or root
                size_t lastSlash = filepath.rfind('/');
                return (lastSlash != std::string::npos)
                       ? filepath.substr(0, lastSlash + 1)
                       : "(root)";
            }
            ++pos; // skip past the '/'
        }
        return filepath.substr(0, pos);
    }

    void parseLine(const std::string& line) {
        std::istringstream iss(line);
        std::string addedStr, removedStr, filepath;

        if (!(iss >> addedStr >> removedStr)) return;
        std::getline(iss, filepath);

        // trim leading whitespace
        size_t start = filepath.find_first_not_of(" \t");
        if (start == std::string::npos) return;
        filepath = filepath.substr(start);

        // handle renames: "old => new" or "{old => new}/rest"
        // numstat shows the destination path after the tab-separated fields
        // but sometimes has {old => new} syntax — take as-is for grouping

        // skip binary files (marked with -)
        if (addedStr == "-" || removedStr == "-") return;

        int64_t a = 0, r = 0;
        try {
            a = std::stoll(addedStr);
            r = std::stoll(removedStr);
        } catch (...) {
            return; // malformed line
        }

        std::string dir = getDirAtDepth(filepath);

        dirStats[dir].added   += a;
        dirStats[dir].removed += r;
        dirStats[dir].total   += a + r;
        dirStats[dir].files   += 1;

        totalAdded   += a;
        totalRemoved += r;
        totalChanges += a + r;
        totalFiles   += 1;
    }

    static std::string fmtNum(int64_t n) {
        if (n >= 10000000)  return std::to_string(n / 1000000) + "." +
                                   std::to_string((n % 1000000) / 100000) + "M";
        if (n >= 1000000)   return std::to_string(n / 1000000) + "." +
                                   std::to_string((n % 1000000) / 100000) + "M";
        if (n >= 100000)    return std::to_string(n / 1000) + "K";
        if (n >= 10000)     return std::to_string(n / 1000) + "." +
                                   std::to_string((n % 1000) / 100) + "K";
        return std::to_string(n);
    }

    static std::string bar(double pct, int width = 30) {
        int filled = static_cast<int>(pct / 100.0 * width + 0.5);
        filled = std::min(filled, width);
        std::string b(filled, '#');
        b += std::string(width - filled, ' ');
        return b;
    }

    // Validate ref string: only allow safe characters for git refs
    static bool isSafeRef(const std::string& ref) {
        for (char c : ref) {
            if (!(std::isalnum(c) || c == '.' || c == '-' || c == '_'
                  || c == '/' || c == '~' || c == '^')) {
                return false;
            }
        }
        return !ref.empty();
    }

public:
    void setDepth(int d) { depth = std::max(1, d); }
    void setTopN(int n)  { topN  = std::max(0, n); }

    // Read numstat from stdin (pipe-friendly)
    bool readStdin() {
        std::string line;
        while (std::getline(std::cin, line)) {
            if (!line.empty()) parseLine(line);
        }
        return totalFiles > 0;
    }

    // Run git diff --numstat between two refs
    bool runGitDiff(const std::string& ref1, const std::string& ref2) {
        if (!isSafeRef(ref1) || !isSafeRef(ref2)) {
            std::cerr << "Error: invalid ref name\n";
            return false;
        }

        // Use -- to prevent ref names from being interpreted as paths
        std::string cmd = "git diff --numstat " + ref1 + ".." + ref2 + " --";

        std::cerr << "Running: " << cmd << "\n";
        std::cerr << "(this may take a while for large diffs)\n\n";

        FILE* pipe = popen(cmd.c_str(), "r");
        if (!pipe) {
            std::cerr << "Error: failed to execute git\n";
            return false;
        }

        char buf[4096];
        while (fgets(buf, sizeof(buf), pipe)) {
            size_t len = strlen(buf);
            if (len > 0 && buf[len - 1] == '\n') buf[len - 1] = '\0';
            if (buf[0] != '\0') parseLine(buf);
        }

        int status = pclose(pipe);
        if (status != 0) {
            std::cerr << "Error: git diff exited with status " << status << "\n";
            return false;
        }
        return totalFiles > 0;
    }

    void print() const {
        if (dirStats.empty()) {
            std::cout << "No changes found.\n";
            return;
        }

        // Sort by total changes descending
        std::vector<std::pair<std::string, DirStats>> sorted(
            dirStats.begin(), dirStats.end());

        std::sort(sorted.begin(), sorted.end(),
                  [](const auto& a, const auto& b) {
                      return a.second.total > b.second.total;
                  });

        if (topN > 0 && static_cast<int>(sorted.size()) > topN) {
            // Collect "other" bucket
            DirStats other;
            for (size_t i = topN; i < sorted.size(); ++i) {
                other.added   += sorted[i].second.added;
                other.removed += sorted[i].second.removed;
                other.total   += sorted[i].second.total;
                other.files   += sorted[i].second.files;
            }
            sorted.resize(topN);
            sorted.emplace_back("(other)", other);
        }

        // Header
        std::cout << "\n" << std::string(96, '=') << "\n";
        std::cout << "  Git Diff Analysis   |   "
                  << fmtNum(totalChanges) << " lines changed across "
                  << totalFiles << " files"
                  << "   (++" << fmtNum(totalAdded)
                  << "  --" << fmtNum(totalRemoved) << ")\n";
        std::cout << std::string(96, '=') << "\n\n";

        std::cout << std::left  << std::setw(22) << "Directory"
                  << std::right << std::setw(8)  << "Files"
                  << std::setw(10) << "Added"
                  << std::setw(10) << "Removed"
                  << std::setw(10) << "Total"
                  << std::setw(7)  << "%"
                  << "  " << "Distribution"
                  << "\n";
        std::cout << std::string(96, '-') << "\n";

        for (const auto& [dir, s] : sorted) {
            double pct = totalChanges > 0
                         ? static_cast<double>(s.total) / totalChanges * 100.0
                         : 0.0;

            std::string dirName = dir;
            if (dirName.size() > 21) dirName = dirName.substr(0, 18) + "...";

            std::cout << std::left  << std::setw(22) << dirName
                      << std::right << std::setw(8)  << s.files
                      << std::setw(10) << fmtNum(s.added)
                      << std::setw(10) << fmtNum(s.removed)
                      << std::setw(10) << fmtNum(s.total)
                      << std::setw(6)  << std::fixed << std::setprecision(1) << pct
                      << "%"
                      << " |" << bar(pct) << "|"
                      << "\n";
        }
        std::cout << std::string(96, '=') << "\n";
    }
};

static void usage(const char* prog) {
    std::cerr
        << "Usage:\n"
        << "  " << prog << " [options] <ref1> <ref2>     Diff two git refs\n"
        << "  git diff --numstat A..B | " << prog << " [options] -\n"
        << "\n"
        << "Options:\n"
        << "  -d, --depth N     Directory grouping depth (default: 1)\n"
        << "  -n, --top N       Show only top N directories\n"
        << "  -h, --help        This message\n";
}

int main(int argc, char* argv[]) {
    DiffAnalyzer analyzer;
    bool fromStdin = false;

    static struct option long_opts[] = {
        {"depth", required_argument, nullptr, 'd'},
        {"top",   required_argument, nullptr, 'n'},
        {"help",  no_argument,       nullptr, 'h'},
        {nullptr, 0, nullptr, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "d:n:h", long_opts, nullptr)) != -1) {
        switch (opt) {
            case 'd': analyzer.setDepth(std::atoi(optarg)); break;
            case 'n': analyzer.setTopN(std::atoi(optarg));  break;
            case 'h': usage(argv[0]); return 0;
            default:  usage(argv[0]); return 1;
        }
    }

    int remaining = argc - optind;

    if (remaining == 1 && std::string(argv[optind]) == "-") {
        fromStdin = true;
    } else if (remaining == 2) {
        // two refs
    } else {
        usage(argv[0]);
        return 1;
    }

    bool ok;
    if (fromStdin) {
        ok = analyzer.readStdin();
    } else {
        ok = analyzer.runGitDiff(argv[optind], argv[optind + 1]);
    }

    if (!ok) {
        std::cerr << "No data. Check that you're inside a git repo "
                     "and the refs exist.\n";
        return 1;
    }

    analyzer.print();
    return 0;
}