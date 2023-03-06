#include "utility.hpp"

std::mt19937 rand_gen(time(0));

std::filesystem::path dataFolder = "./data/viz-calcium";

vtkNew<vtkNamedColors> namedColors;

// from https://stackoverflow.com/questions/2353211/hsl-to-rgb-color-conversion
std::array<float, 3> hslToRgb(std::array<float, 3> hsl) {

    auto hueToRgb = [](float p, float q, float t) {
        if (t < 0.f)
            t += 1.f;
        if (t > 1.f)
            t -= 1.f;
        if (t < 1.f / 6.f)
            return p + (q - p) * 6.f * t;
        if (t < 1.f / 2.f)
            return q;
        if (t < 2.f / 3.f)
            return p + (q - p) * (2.f / 3.f - t) * 6.f;
        return p;
    };

    auto [h, s, l] = hsl;
    float r, g, b;

    if (s == 0.f) {
        r = g = b = l; // achromatic
    }
    else {
        float q = l < 0.5f ? l * (1 + s) : l + s - l * s;
        float p = 2 * l - q;
        r = hueToRgb(p, q, h + 1.f / 3.f);
        g = hueToRgb(p, q, h);
        b = hueToRgb(p, q, h - 1.f / 3.f);
    }
    return { r, g, b };
}


std::array<unsigned char, 3> generateNiceColor() {
    std::uniform_real_distribution<float> dist(0, 1);
    std::array<float, 3> hsl = { dist(rand_gen), 1.0, dist(rand_gen) / 2.f + 0.25f };
    auto rgb = hslToRgb(hsl);
    return { static_cast<unsigned char>(rgb[0] * 255),
        static_cast<unsigned char>(rgb[1] * 255),
        static_cast<unsigned char>(rgb[2] * 255) };
}