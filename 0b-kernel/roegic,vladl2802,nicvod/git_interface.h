#pragma once

#include <string>
#include <vector>
#include <functional>
#include <git2.h>

struct CommitInfo {
    std::string hash;
    std::string author;
    std::string date;
    std::string message;
};

class FileContentReader {
public:
    FileContentReader(git_blob* blob);
    ~FileContentReader();
    
    FileContentReader(const FileContentReader&) = delete;
    FileContentReader& operator=(const FileContentReader&) = delete;
    
    std::string readChunk(size_t chunk_size = 4096);
    
    bool readLine(std::string& line);
    
    bool hasMore() const;
    
    size_t totalSize() const;
    
private:
    git_blob* blob_;
    const char* content_;
    size_t size_;
    size_t position_;
};

class GitInterface {
public:
    GitInterface(const std::string& repo_path);
    ~GitInterface();

    GitInterface(const GitInterface&) = delete;
    GitInterface& operator=(const GitInterface&) = delete;

    std::vector<CommitInfo> getCommitHistory(
        const std::string& filename = "", 
        int max_count = -1
    );

    bool isValidRepo() const;
    
    FileContentReader* getFileContentReader(
        const std::string& commit_hash, 
        const std::string& filename
    );
    std::string getFileContent(
        const std::string& commit_hash, 
        const std::string& filename,
        size_t max_size_mb = 10
    );
    using FileCallback = std::function<bool(const std::string& filepath)>;
    
    void walkFilesInCommit(
        const std::string& commit_hash,
        FileCallback callback
    );
    
private:
    std::string repo_path_;
    git_repository* repo_;
    
    std::string oidToString(const git_oid* oid);
    bool stringToOid(git_oid* oid, const std::string& hash);
};