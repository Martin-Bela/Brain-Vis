#pragma once

#include <vtkNew.h>
#include <vtkNamedColors.h>


#include <array>
#include <filesystem>
#include <random>
#include <exception>

extern std::mt19937 rand_gen;

extern std::filesystem::path dataFolder;

extern vtkNew<vtkNamedColors> namedColors;


std::array<float, 3> hslToRgb(std::array<float, 3> hsl);

std::array<unsigned char, 3> generateNiceColor();

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