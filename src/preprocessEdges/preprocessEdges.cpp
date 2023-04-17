#include "utility.hpp"

#include <vtkTable.h>
#include <vtkDelimitedTextReader.h>
#include <vtkNew.h>
#include <vtkVariant.h>

#include <array>
#include <unordered_map>

#include "edge.hpp"

std::vector<uint16_t> loadMapping() {
    vtkNew<vtkDelimitedTextReader> reader;
    std::string path = (dataFolder / "positions/rank_0_positions.txt").string();
    reader->SetFileName(path.data());
    reader->DetectNumericColumnsOn();
    reader->SetFieldDelimiterCharacters(" ");
    reader->Update();

    int point_index = -1;
    vtkTable* table = reader->GetOutput();

    std::vector<uint16_t> mapping(table->GetNumberOfRows());
    //the first row is header

    std::array<double, 3> prev_point{ INFINITY, INFINITY, INFINITY };
    for (vtkIdType i = 1; i < table->GetNumberOfRows(); i++) {
        if (table->GetValue(i, 0).ToString() == "#") continue;

        std::array<double,3> point {
             (table->GetValue(i, 1)).ToDouble(),
             (table->GetValue(i, 2)).ToDouble(),
             (table->GetValue(i, 3)).ToDouble()
        };

        if (manhattanDist(prev_point.data(), point.data()) > 0.5) {
            prev_point = point;
            point_index++;
        }
        mapping[(table->GetValue(i, 0)).ToInt() - 1] = point_index;
    }
    return mapping;
}


int main() {
    setCurrentDirectory();

    confirmOperation("Write \"yes\" if you want to remove all files in network-bin and begin preprocessing.");

    std::filesystem::remove_all(dataFolder / "network-bin");
    std::filesystem::create_directory(dataFolder / "network-bin");

    for (int i = 0; i < 100; i++) {
        std::cout << i << "%\n";
        vtkNew<vtkDelimitedTextReader> reader;
        auto path = (dataFolder / "network/rank_0_step_").string() + std::to_string(i*10000) + "_in_network.txt";
        reader->SetFileName(path.data());
        reader->DetectNumericColumnsOn();
        reader->SetFieldDelimiterCharacters(" \t");
        reader->Update();

        auto mapping = loadMapping();

        vtkTable* table = reader->GetOutput();

        std::unordered_map<uint32_t, int> edge_count;

        auto output_path = (dataFolder / "network-bin/rank_0_step_").string() + std::to_string(i * 10000) + "_in_network";
        std::ofstream out(output_path , std::ios::binary);
        for (vtkIdType i = 0; i < table->GetNumberOfRows(); i++)
        {
            if (table->GetValue(i, 0).ToString() == "#") continue;
            Edge edge{
                .from = static_cast<uint16_t>(table->GetValue(i, 1).ToInt() - 1),
                .to = static_cast<uint16_t>(table->GetValue(i, 3).ToInt() - 1),
                .weight = static_cast<uint16_t>(table->GetValue(i, 4).ToInt())
            };
            edge_count[mapping[edge.from] << 16 | mapping[edge.to]]++;
        }

        std::vector<std::pair<uint32_t, int>> edges(edge_count.begin(), edge_count.end());
        std::sort(edges.begin(), edges.end(), [](auto left, auto right) { return left.second > right.second; });

        for (auto& [val, count] : edges) {
            Edge new_edge{ val >> 16, static_cast<uint16_t>(val), count };
            out.write(reinterpret_cast<const char*>(&new_edge), sizeof(new_edge));
        }
    }
}