#pragma once

#include "utility.hpp"

#include <vtkNew.h>
#include <vtkNamedColors.h>


#include <array>
#include <filesystem>
#include <fstream>
#include <random>
#include <exception>

extern std::mt19937 rand_gen;

extern vtkNew<vtkNamedColors> namedColors;


std::array<float, 3> hslToRgb(std::array<float, 3> hsl);

std::array<unsigned char, 3> generateNiceColor();

template<typename T>
class DeferredInit {
    std::optional<T> object = std::nullopt;
public:
    template<typename... Args>
    void init(Args&&... args) {
        object.emplace(std::forward<Args>(args)...);
    }

    T* ptr() {
        return &*object;
    }

    operator T& () {
        return *object;
    }

    T& operator*() {
        return *object;
    }

    T* operator->() {
        return &*object;
    }

    operator const T& () const {
        return *object;
    }

    const T& operator*() const {
        return *object;
    }

    const T* operator->() const {
        return &*object;
    }
};

template <typename T>
class BinaryReader {
    std::ifstream file;
    size_t itemCount;
public:
    BinaryReader(const std::filesystem::path& path) :
        file(path, std::ios::binary)
    { 
        if (!file.good()) {
            std::cout << "File \"" << path << "\" couldn't be opened!";
            checkFile(file);
        }

        auto byteSize = std::filesystem::file_size(path);
        assert(itemCount % sizeof(T) == 0);
        itemCount = byteSize / sizeof(T);
    }
    
    // Reads item and increments position
    T read() {
        T t;
        file.read(reinterpret_cast<char*>(&t), sizeof(T));
        checkFile(file);
        return t;
    }

    // Return number of items stored in the file
    size_t count() { return itemCount; }

    // Get position of the current item
    size_t getPos() { 
        auto pos = file.tellg();
        assert(pos % sizeof(T) == 0);
        checkFile(file);
        return pos / sizeof(T);
    }

    // Set position of the current item
    void setPos(size_t pos) {
        assert(pos < itemCount);
        file.seekg(pos * sizeof(T));
        checkFile(file);
    }
};
