#include <concepts>
#include <filesystem>
#include <fstream>
#include <cassert>

#include "utility.hpp"

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
        assert(byteSize % sizeof(T) == 0);
        itemCount = byteSize / sizeof(T);
    }

    // Reads item and increments position
    T read() {
        T t;
        file.read(reinterpret_cast<char*>(&t), sizeof(T));
        checkFile(file);
		if constexpr (std::same_as<T, struct NeuronProperties>) {
            t.fired -= '0';
        }
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
