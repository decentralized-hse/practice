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