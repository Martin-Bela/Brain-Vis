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