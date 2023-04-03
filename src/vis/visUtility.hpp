#pragma once

#include "utility.hpp"

#include <vtkNew.h>
#include <vtkNamedColors.h>


#include <array>
#include <filesystem>
#include <random>
#include <exception>

extern std::mt19937 rand_gen;

extern vtkNew<vtkNamedColors> namedColors;


std::array<float, 3> hslToRgb(std::array<float, 3> hsl);

std::array<unsigned char, 3> generateNiceColor();
