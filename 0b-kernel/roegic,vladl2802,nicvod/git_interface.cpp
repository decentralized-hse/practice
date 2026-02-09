#include "git_interface.h"
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <ctime>

FileContentReader::FileContentReader(git_blob* blob) 
    : blob_(blob), position_(0) {
    content_ = static_cast<const char*>(git_blob_rawcontent(blob));
    size_ = git_blob_rawsize(blob);
}

FileContentReader::~FileContentReader() {
    if (blob_) {
        git_blob_free(blob_);
    }
}

std::string FileContentReader::readChunk(size_t chunk_size) {
    if (position_ >= size_) {
        return "";
    }
    
    size_t to_read = std::min(chunk_size, size_ - position_);
    std::string chunk(content_ + position_, to_read);
    position_ += to_read;
    
    return chunk;
}

bool FileContentReader::readLine(std::string& line) {
    if (position_ >= size_) {
        return false;
    }
    
    line.clear();
    
    while (position_ < size_) {
        char c = content_[position_++];
        
        if (c == '\n') {
            break;
        }
        
        if (c == '\r' && position_ < size_ && content_[position_] == '\n') {
            position_++;
            break;
        }
        
        line += c;
    }
    
    return true;
}

bool FileContentReader::hasMore() const {
    return position_ < size_;
}

size_t FileContentReader::totalSize() const {
    return size_;
}

GitInterface::GitInterface(const std::string& repo_path) 
    : repo_path_(repo_path), repo_(nullptr) {
    
    git_libgit2_init();
    
    int error = git_repository_open(&repo_, repo_path.c_str());
    if (error < 0) {
        const git_error* e = git_error_last();
        git_libgit2_shutdown();
        throw std::runtime_error("Failed to open repository: " + 
                               std::string(e ? e->message : "unknown error"));
    }
}

GitInterface::~GitInterface() {
    if (repo_) {
        git_repository_free(repo_);
    }
    git_libgit2_shutdown();
}

bool GitInterface::isValidRepo() const {
    return repo_ != nullptr;
}

std::string GitInterface::oidToString(const git_oid* oid) {
    char hash_str[GIT_OID_HEXSZ + 1];
    git_oid_tostr(hash_str, sizeof(hash_str), oid);
    return std::string(hash_str);
}

bool GitInterface::stringToOid(git_oid* oid, const std::string& hash) {
    return git_oid_fromstr(oid, hash.c_str()) == 0;
}

std::vector<CommitInfo> GitInterface::getCommitHistory(
    const std::string& filename, int max_count) {
    
    std::vector<CommitInfo> commits;
    
    git_revwalk* walker = nullptr;
    int error = git_revwalk_new(&walker, repo_);
    
    if (error < 0) {
        return commits;
    }
    
    git_revwalk_push_head(walker);
    git_revwalk_sorting(walker, GIT_SORT_TIME);
    
    git_oid oid;
    int count = 0;
    
    while (git_revwalk_next(&oid, walker) == 0) {
        if (max_count > 0 && count >= max_count) {
            break;
        }
        
        git_commit* commit = nullptr;
        if (git_commit_lookup(&commit, repo_, &oid) != 0) {
            continue;
        }

        bool include_commit = filename.empty();
        
        if (!filename.empty()) {
            git_tree* tree = nullptr;
            if (git_commit_tree(&tree, commit) == 0) {
                git_tree_entry* entry = nullptr;
                if (git_tree_entry_bypath(&entry, tree, filename.c_str()) == 0) {
                    include_commit = true;
                    git_tree_entry_free(entry);
                }
                git_tree_free(tree);
            }
        }
        
        if (include_commit) {
            CommitInfo info;
            info.hash = oidToString(&oid);
            
            const git_signature* author = git_commit_author(commit);
            if (author) {
                info.author = author->name ? author->name : "";
                time_t time = author->when.time;
                char time_buf[64];
                struct tm* tm_info = localtime(&time);
                strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm_info);
                info.date = time_buf;
            }
            
            const char* message = git_commit_message(commit);
            if (message) {
                info.message = message;
                if (!info.message.empty() && info.message.back() == '\n') {
                    info.message.pop_back();
                }
            }
            
            commits.push_back(info);
            count++;
        }
        
        git_commit_free(commit);
    }
    
    git_revwalk_free(walker);
    return commits;
}

FileContentReader* GitInterface::getFileContentReader(
    const std::string& commit_hash, 
    const std::string& filename) {
    
    git_oid oid;
    if (!stringToOid(&oid, commit_hash)) {
        return nullptr;
    }
    
    git_commit* commit = nullptr;
    if (git_commit_lookup(&commit, repo_, &oid) != 0) {
        return nullptr;
    }
    
    git_tree* tree = nullptr;
    if (git_commit_tree(&tree, commit) != 0) {
        git_commit_free(commit);
        return nullptr;
    }
    
    git_tree_entry* entry = nullptr;
    if (git_tree_entry_bypath(&entry, tree, filename.c_str()) != 0) {
        git_tree_free(tree);
        git_commit_free(commit);
        return nullptr;
    }
    
    const git_oid* blob_oid = git_tree_entry_id(entry);
    git_blob* blob = nullptr;
    
    if (git_blob_lookup(&blob, repo_, blob_oid) != 0) {
        git_tree_entry_free(entry);
        git_tree_free(tree);
        git_commit_free(commit);
        return nullptr;
    }
    
    git_tree_entry_free(entry);
    git_tree_free(tree);
    git_commit_free(commit);
    return new FileContentReader(blob);
}

std::string GitInterface::getFileContent(
    const std::string& commit_hash, 
    const std::string& filename,
    size_t max_size_mb) {
    
    FileContentReader* reader = getFileContentReader(commit_hash, filename);
    
    if (!reader) {
        return "";
    }
    
    size_t max_bytes = max_size_mb * 1024 * 1024;
    
    if (reader->totalSize() > max_bytes) {
        std::cerr << "Warning: File " << filename << " is too large (" 
                  << (reader->totalSize() / 1024 / 1024) << " MB), skipping\n";
        delete reader;
        return "";
    }
    std::string result;
    result.reserve(reader->totalSize());
    
    while (reader->hasMore()) {
        result += reader->readChunk(8192);
    }
    
    delete reader;
    return result;
}

struct StreamWalkData {
    GitInterface::FileCallback* callback;
    bool should_stop;
};

static int stream_tree_walk_callback(const char* root, const git_tree_entry* entry, void* payload) {
    StreamWalkData* data = static_cast<StreamWalkData*>(payload);
    
    if (data->should_stop) {
        return 1;
    }

    if (git_tree_entry_type(entry) == GIT_OBJECT_BLOB) {
        std::string path = root;
        if (!path.empty() && path.back() != '/') {
            path += '/';
        }
        path += git_tree_entry_name(entry);
        
        bool continue_walking = (*data->callback)(path);
        
        if (!continue_walking) {
            data->should_stop = true;
            return 1;
        }
    }
    
    return 0;
}

void GitInterface::walkFilesInCommit(
    const std::string& commit_hash,
    FileCallback callback) {
    
    git_oid oid;
    if (!stringToOid(&oid, commit_hash)) {
        return;
    }
    
    git_commit* commit = nullptr;
    if (git_commit_lookup(&commit, repo_, &oid) != 0) {
        return;
    }
    
    git_tree* tree = nullptr;
    if (git_commit_tree(&tree, commit) != 0) {
        git_commit_free(commit);
        return;
    }
    
    StreamWalkData data;
    data.callback = &callback;
    data.should_stop = false;
    
    git_tree_walk(tree, GIT_TREEWALK_PRE, stream_tree_walk_callback, &data);
    
    git_tree_free(tree);
    git_commit_free(commit);
}
