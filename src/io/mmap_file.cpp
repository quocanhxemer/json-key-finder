#include "mmap_file.h"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

MMapFile::MMapFile(const char* filepath) {
    auto perror_exit = [](const char* msg) {
        std::perror(msg);
        std::exit(EXIT_FAILURE);
    };

    fd_ = open(filepath, O_RDONLY);
    if (fd_ == -1) {
        perror_exit("open failed");
    }

    struct stat sb;
    if (fstat(fd_, &sb) == -1) {
        perror_exit("fstat failed");
    }
    if (sb.st_size < 0) {
        errno = EINVAL;
        perror_exit("file size is negative");
    }

    size_ = static_cast<size_t>(sb.st_size);
    if (size_ == 0) {
        data_ = nullptr;
        return;
    }

    data_ = mmap(nullptr, size_, PROT_READ, MAP_PRIVATE, fd_, 0);

    if (data_ == MAP_FAILED) {
        perror_exit("mmap failed");
    }
}

MMapFile::~MMapFile() {
    if (data_ && data_ != MAP_FAILED && size_ > 0) {
        munmap(data_, size_);
    }
    if (fd_ != -1) {
        close(fd_);
    }
}
