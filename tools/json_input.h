#pragma once

#include "io/mmap_file.h"
#include "key_generation.h"

#include <memory>
#include <string>
#include <string_view>

namespace json_input {

// Holds Json data and its KeyCatalog for gen_keys
class LoadedJson {
   public:
    LoadedJson(std::string path,
               std::unique_ptr<MMapFile> mmap,
               keygen::KeyCatalog key_catalog);

    const std::string& path() const;
    std::string_view data() const;
    const keygen::KeyCatalog& key_catalog() const;

   private:
    std::string path_;
    std::unique_ptr<MMapFile> mmap_;
    keygen::KeyCatalog key_catalog_;
};

LoadedJson load_json_or_exit(std::string path);

}  // namespace json_input
