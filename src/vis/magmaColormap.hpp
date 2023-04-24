// taken from https://github.com/jgreitemann/colormap/blob/master/include/colormap/palettes.hpp

#include <QColor>

extern QColor magmaColorMap[256];

// i in range (0 , 1)
const QColor getMagmaColor(double i) {
    int index = std::min(i, 1.) * 255;
    return magmaColorMap[index];
}

    