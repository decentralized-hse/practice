#pragma once

#include <string>
#include <vector>
#include <git2.h>

struct CommitInfo {
    std::string hash;
    std::string author;
    std::string date;
    std::string message;
};

class GitInterface {
public:
    GitInterface(const std::string& repo_path);
    ~GitInterface();

    GitInterface(const GitInterface&) = delete;
    GitInterface& operator=(const GitInterface&) = delete;

    std::vector<CommitInfo> getCommitHistory(const std::string& filename = "", int max_count = -1);

    std::string getFileContent(const std::string& commit_hash, const std::string& filename);
    
    std::vector<std::string> getFilesInCommit(const std::string& commit_hash);
    
    bool isValidRepo() const;
    
private:
    std::string repo_path_;
    git_repository* repo_;
    
    std::string oidToString(const git_oid* oid);
    bool stringToOid(git_oid* oid, const std::string& hash);
};