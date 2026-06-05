#include "json_input.h"

#include <cstdlib>
#include <iostream>
#include <utility>

namespace json_input {

LoadedJson::LoadedJson(std::string path,
                       std::unique_ptr<MMapFile> mmap,
                       keygen::KeyCatalog key_catalog)
    : path_(std::move(path)),
      mmap_(std::move(mmap)),
      key_catalog_(std::move(key_catalog)) {}

const std::string& LoadedJson::path() const {
    return path_;
}

std::string_view LoadedJson::data() const {
    return {mmap_->data(), mmap_->size()};
}

const keygen::KeyCatalog& LoadedJson::key_catalog() const {
    return key_catalog_;
}

LoadedJson load_json_or_exit(std::string path) {
    auto mmap = std::make_unique<MMapFile>(path.c_str());
    std::string_view data(mmap->data(), mmap->size());
    if (data.empty()) {
        std::cerr << "JSON input is empty: " << path << '\n';
        std::exit(EXIT_FAILURE);
    }

    keygen::KeyCatalog catalog = keygen::build_key_catalog(data);

    return LoadedJson(std::move(path), std::move(mmap), std::move(catalog));
}

}  // namespace json_input
