#include "utility.hpp"

#include <vtkTable.h>
#include <vtkDelimitedTextReader.h>
#include <vtkNew.h>
#include <vtkVariant.h>

#include "edge.hpp"


int main() {
    set_current_directory();

    confirm_operation("Write \"yes\" if you want to remove all files in network-bin and begin preprocessing.");

    vtkNew<vtkDelimitedTextReader> reader;
    auto path = (dataFolder / "network/rank_0_step_0_in_network.txt").string();
    reader->SetFileName(path.data());
    reader->DetectNumericColumnsOn();
    reader->SetFieldDelimiterCharacters(" \t");
    reader->Update();


    vtkTable* table = reader->GetOutput();

    std::filesystem::remove_all(dataFolder / "network-bin");
    std::filesystem::create_directory(dataFolder / "network-bin");

    std::ofstream out{ dataFolder / "network-bin/rank_0_step_0_in_network", std::ios::binary };
    for (vtkIdType i = 0; i < table->GetNumberOfRows(); i++)
    {
        if (table->GetValue(i, 0).ToString() == "#") continue;
        Edge edge{
            .from = static_cast<uint16_t>(table->GetValue(i, 1).ToInt() - 1),
            .to = static_cast<uint16_t>(table->GetValue(i, 3).ToInt() - 1),
            .weight = static_cast<uint16_t>(table->GetValue(i, 4).ToInt())
        };
        out.write(reinterpret_cast<const char*>(&edge), sizeof(edge));
    }
}