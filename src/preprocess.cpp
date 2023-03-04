#include <array>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <ranges>

#include "neuronProperties.hpp"

std::filesystem::path dataFolder = "data/viz-calcium";

int main() {
    using namespace std::chrono;

    if (std::filesystem::current_path().filename().string().starts_with("build")) {
        // cd ..
        auto newPath = std::filesystem::current_path().parent_path();
        std::filesystem::current_path(newPath);
    }
    std::cout << "Current working directory: " << std::filesystem::current_path() << std::endl;

    std::filesystem::remove_all(dataFolder / "monitors-bin");
    if (!std::filesystem::create_directory(dataFolder / "monitors-bin")) {
        //throw std::runtime_error("Directory cannot be created.");
    }

    auto input_path = (dataFolder / "monitors" / "0_").string();
    auto output_path = (dataFolder / "monitors-bin" / "timestep").string();

    auto start = steady_clock::now();
    constexpr int step_count = 100 /*10'000*/;
    for (int in_outer = 0; in_outer < step_count; in_outer += 50) {
        auto end = steady_clock::now();
        std::cout << in_outer << " files " << duration_cast<duration<double>>(end - start) << "\n";
        start = end;

        std::array<std::ifstream, 50> in_files;
        for (int in = 0; in < 50; in++) {
            auto path = input_path + std::to_string(in + in_outer) + ".csv";
            in_files[in].open(path);
        }

        for (int out = 0; out < 50'000; out++) {
            std::ofstream out_file(output_path + std::to_string(out), std::ios_base::binary | std::ios_base::app);
            for (int in = 0; in < 50; in++) {
                auto neuron = NeuronProperties::Parse(in_files[in]);
                out_file.write(reinterpret_cast<const char*>(&neuron), sizeof(neuron));
            }
        }
    }
    std::cout << step_count << " files " << duration_cast<duration<double>>(steady_clock::now() - start) << "\n";
}