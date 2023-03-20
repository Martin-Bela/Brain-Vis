#include <array>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <ranges>

#include "neuronProperties.hpp"

std::filesystem::path dataFolder = "data/viz-calcium";

int main() {
    using namespace std::chrono;
    std::cout << "Write \"yes\" if you want to remove all files in monitors-bin and begin preprocessing." << std::endl;
    std::string str;
    std::getline(std::cin, str);
    if (str != "yes") {
        return 0;
    }

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

    auto global_start = steady_clock::now();
    auto start = global_start;
    constexpr int step_count = 50'000;

    constexpr int open_files = 500;

    for (int in_outer = 0; in_outer < step_count; in_outer += open_files) {
        if (in_outer % 500 == 0) {
            auto end = steady_clock::now();
            std::cout << in_outer * 100 / 50'000 << "% " << duration_cast<duration<double>>(end - start) << "\n";
            start = end;
        }

        std::array<std::ifstream, open_files> in_files;
        for (int in = 0; in < open_files; in++) {
            auto path = input_path + std::to_string(in + in_outer) + ".csv";
            in_files[in].open(path);
        }

        for (int out = 0; out < 10'000; out++) {
            std::ofstream out_file(output_path + std::to_string(out), std::ios::binary | std::ios::app | std::ios::ate);
            for (int in = 0; in < open_files; in++) {
                auto neuron = NeuronProperties::Parse(in_files[in]);
                out_file.write(reinterpret_cast<const char*>(&neuron), sizeof(neuron));
            }
            if (!out_file.good()) {
                std::cout << "Output file error\n" << std::endl;
            }
        }

        for (auto& file : in_files) {
            if (!file.good()) {
                std::cout << "Input file error\n" << std::endl;
            }
        }
    }
    std::cout << "Duration: " << duration_cast<duration<double>>(steady_clock::now() - global_start) << "\n";
}