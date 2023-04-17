#pragma once
#include <filesystem>
#include <iostream>

const std::filesystem::path dataFolder = "./data/viz-disable";

inline void setCurrentDirectory() {
    std::filesystem::path path = std::filesystem::current_path();
    while (!path.filename().string().starts_with("brain-visualisation")) {
        auto parent = path.parent_path();
        if (parent == path) {
            const char* msg = "brain-visualisation directory wasn't found!";
            std::cout << msg << std::endl;
            throw std::runtime_error{ msg };
        }
        path = std::move(parent);
    }
    std::filesystem::current_path(path);
    std::cout << "Current working directory: " << std::filesystem::current_path() << std::endl;
}

inline bool confirmOperation(const char* msg){
    std::cout << msg << std::endl;
    std::string str;
    std::getline(std::cin, str);
    if (str != "yes") {
        return true;
    }
    return false;
}

inline void checkFile(std::istream& stream) {
    if (!stream.good()) {
        std::string_view msg;
        if (stream.eof()) {
            msg = "End of file was reached!\n";
        }
        if (stream.rdstate() & stream.badbit) {
            msg = "File badbit was set!\n";
        }
        if (stream.rdstate() & stream.failbit) {
            msg = "File failbit was set!\n";
        }
        std::cout << msg;
        throw std::runtime_error{ msg.data() };
    }
}