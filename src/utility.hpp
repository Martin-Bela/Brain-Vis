#pragma once
#include <filesystem>
#include <iostream>

const std::filesystem::path dataFolder = "./data/viz-calcium";

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

inline void confirmOperation(const char* msg){
    std::cout << msg << std::endl;
    std::string str;
    std::getline(std::cin, str);
    if (str == "yes") {
        return;
    }
    exit(1);
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

inline double manhattanDist(double* x, double* y) {
    auto dx = x[0] - y[0];
    auto dy = x[1] - y[1];
    auto dz = x[2] - y[2];
    return abs(dx) + abs(dy) + abs(dz);
};

inline double map_to_unit_range(double lower_bound, double upper_bound, double value) {
    return (value - lower_bound) / (upper_bound - lower_bound);
}