// taken from https://github.com/jgreitemann/colormap/blob/master/include/colormap/palettes.hpp
#pragma once

#include <QColor>

#include <array>

extern std::array<QColor, 256> magmaColorMap;

// i in range (0 , 1)
constexpr QColor getMagmaColor(double i) {
    int index = std::min(i, 1.) * 255;
    return magmaColorMap[index];
}



    