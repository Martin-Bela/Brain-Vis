#include <array>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <ranges>

#include "vis/binaryReader.hpp"
#include "utility.hpp"
#include "neuronProperties.hpp"

void preprocessProperties(std::filesystem::path dataFolder) {
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
                auto neuron = NeuronProperties::parse(inFiles[in]);
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

struct AttributeStack {
    float max = std::numeric_limits<float>::min();
    float min = std::numeric_limits<float>::max();
    float sum = 0.0;
};

std::string attributeToString(int attribute) {
    switch (attribute) {
    case 0: return "fired.txt";
    case 1: return "firedFraction.txt";
    case 2: return "electricActivity.txt";
    case 3: return "secondaryVariable.txt";
    case 4: return "calcium.txt";
    case 5: return "targetCalcium.txt";
    case 6: return "synapticInput.txt";
    case 7: return "backgroundActivity.txt";
    case 8: return "grownAxons.txt";
    case 9: return "connectedAxons.txt";
    case 10: return "grownDendrites.txt";
    case 11: return "connectedDendrites.txt";
    }
    assert(false);
    return "";
}

void preprocessTimestepProperties(std::filesystem::path dataFolder, int timestepCount) {
    const int pointCount = 50000;
    const int attributeCount = 12;

    std::ofstream outputFiles[attributeCount];
    AttributeStack attributeData[attributeCount];

    std::filesystem::remove_all(dataFolder / "monitors-histogram");
    std::filesystem::create_directory(dataFolder / "monitors-histogram");
    for (int i = 0; i < 12; i++) {
        outputFiles[i].open((dataFolder / "monitors-histogram" / attributeToString(i)).string());
        outputFiles[i] << "# timestep mean sum max min" << std::endl;
    }

    for (int i = 0; i < timestepCount; i++) {
        // Reset the structs used for counting the data at each timestep
        for (int j = 0; j < attributeCount; j++) {
            attributeData[j].max = std::numeric_limits<float>::min();
            attributeData[j].min = std::numeric_limits<float>::max();
            attributeData[j].sum = 0;
        }

        auto path = (dataFolder / "monitors-bin/timestep").string() + std::to_string(i);
        BinaryReader<NeuronProperties> reader(path);

        if (reader.count() < pointCount) {
            std::cout << "File:\"" << path << "\" is too small.\n";
            std::cout << sizeof(NeuronProperties) * pointCount << "bytes expected.\n";
        }

        // Iterate through every neuron and add up the values
        for (int i = 0; i < pointCount; i++) {
            NeuronProperties neuron = reader.read();
            
            float value;
            for (int j = 0; j < attributeCount; j++) {
                value = neuron.projection(j);
                // This check is here because i dont know how C++ casts uint to float
                if (j == 0) {
                    value -= 48.0;
                }
                attributeData[j].max = std::max(attributeData[j].max, value);
                attributeData[j].min = std::min(attributeData[j].min, value);
                attributeData[j].sum += value;
            }
        }

        // Print the iteration data into the opened file
        for (int j = 0; j < attributeCount; j++) {
            std::string line = std::to_string(i) + " " + std::to_string(attributeData[j].sum / pointCount) + " " + std::to_string(attributeData[j].sum) + " " + std::to_string(attributeData[j].max) + " " + std::to_string(attributeData[j].min);
            outputFiles[j] << line << std::endl;
        }
    }
    for (int j = 0; j < attributeCount; j++) {
        outputFiles[j].close();
    }
}


int main() {
    const int timestepCount = 10000;
    setCurrentDirectory();
    const std::filesystem::path calcuiumFolder = "./data/viz-calcium";
    const std::filesystem::path stimulusFolder = "./data/viz-stimulus";
    //preprocessProperties(calcuiumFolder);
    //preprocessProperties(stimulusFolder);
    preprocessTimestepProperties(calcuiumFolder, 50);

}