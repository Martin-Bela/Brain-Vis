#include <array>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <ranges>

#include "utility.hpp"
#include "neuronProperties.hpp"

int main() {
    setCurrentDirectory();

    confirmOperation("Write \"yes\" if you want to remove all files in monitors-bin and begin preprocessing.");

    using namespace std::chrono;

    std::filesystem::remove_all(dataFolder / "monitors-bin");
    if (!std::filesystem::create_directory(dataFolder / "monitors-bin")) {
        //throw std::runtime_error("Directory cannot be created.");
    }

    auto inputPath = (dataFolder / "monitors" / "0_").string();
    auto outputPath = (dataFolder / "monitors-bin" / "timestep").string();

    auto globalStart = steady_clock::now();
    auto start = globalStart;

    constexpr int stepCount = 50'000;
    constexpr int openFiles = 500;

    for (int inOuter = 0; inOuter < stepCount; inOuter += openFiles) {
        if (inOuter % 500 == 0) {
            auto end = steady_clock::now();
            std::cout << inOuter * 100 / 50'000 << "% " << duration_cast<duration<double>>(end - start) << "\n";
            start = end;
        }

        std::array<std::ifstream, openFiles> inFiles;
        for (int in = 0; in < openFiles; in++) {
            auto path = inputPath + std::to_string(in + inOuter) + ".csv";
            inFiles[in].open(path);
        }

        for (int out = 0; out < 10'000; out++) {
            std::ofstream out_file(outputPath + std::to_string(out), std::ios::binary | std::ios::app | std::ios::ate);
            for (int in = 0; in < openFiles; in++) {
                auto neuron = NeuronProperties::Parse(inFiles[in]);
                out_file.write(reinterpret_cast<const char*>(&neuron), sizeof(neuron));
            }
            if (!out_file.good()) {
                std::cout << "Output file error\n" << std::endl;
            }
        }

        for (auto& file : inFiles) {
            if (!file.good()) {
                std::cout << "Input file error\n" << std::endl;
            }
        }
    }
    std::cout << "Duration: " << duration_cast<duration<double>>(steady_clock::now() - globalStart) << "\n";
}