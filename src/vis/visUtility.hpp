#pragma once

#include "utility.hpp"

#include <vtkNew.h>
#include <vtkNamedColors.h>

#include <qcolor.h>


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



class ColorMixer {
    struct Vec3 {
        double x, y, z;
    };
    
    Vec3 coefRed, coefGreen, coefBlue;

    Vec3 getCoef(double y1, double y2, double y3, Vec3 pos, double denom) {
        auto [x1, x2, x3] = pos;
        double a = (x3 * (y2 - y1) + x2 * (y1 - y3) + x1 * (y3 - y2)) / denom;
        double b = (x3 * x3 * (y1 - y2) + x2 * x2 * (y3 - y1) + x1 * x1 * (y2 - y3)) / denom;
        double c = (x2 * x3 * (x2 - x3) * y1 + x3 * x1 * (x3 - x1) * y2 + x1 * x2 * (x1 - x2) * y3) / denom;

        return { a, b, c };
    }

    double applyCoef(double x, Vec3 coef) {
        auto [a, b, c] = coef;
        return a * x * x + b * x + c;
    }

public:
    ColorMixer(QColor color1, QColor color2,
        QColor color3, double middlePos)
    {
        Vec3 pos{ 0, middlePos, 1 };
        auto [pos1, pos2, pos3] = pos;
        double denom = (pos1 - pos2) * (pos1 - pos3) * (pos2 - pos3);
        
        coefRed = getCoef(color1.redF(), color2.redF(), color3.redF(), pos, denom);
        coefGreen = getCoef(color1.greenF(), color2.greenF(), color3.greenF(), pos, denom);
        coefBlue = getCoef(color1.blueF(), color2.blueF(), color3.blueF(), pos, denom);   
    }

    QColor getColor(double x) {
        return QColor::fromRgbF(applyCoef(x, coefRed), applyCoef(x, coefGreen), applyCoef(x, coefBlue));
    }
};