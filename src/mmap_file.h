#pragma once
#include <cstddef>

class MMapFile {
   public:
    explicit MMapFile(const char* filepath);
    ~MMapFile();

    MMapFile(const MMapFile&) = delete;
    MMapFile& operator=(const MMapFile&) = delete;

    MMapFile(const MMapFile&&) = delete;
    MMapFile& operator=(const MMapFile&&) = delete;

    const char* data() const { return static_cast<const char*>(data_); }
    size_t size() const { return size_; }

   private:
    int fd_ = -1;
    void* data_ = nullptr;
    size_t size_ = 0;
};
