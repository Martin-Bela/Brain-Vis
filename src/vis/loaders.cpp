#include <vtkActor.h>
#include <vtkDenseArray.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkPointData.h>
#include <vtkProperty.h>
#include <vtkTable.h>
#include <vtkDelimitedTextReader.h>
#include <vtkSphereSource.h>
#include <vtkGlyph3DMapper.h>
#include <vtkMutableDirectedGraph.h>
#include <vtkGraphToPolyData.h>
#include <vtkGraphLayout.h>
#include <vtkGraphLayoutView.h>
#include <vtkArrowSource.h>
#include <vtkPassThroughEdgeStrategy.h>
#include <vtkPassThroughLayoutStrategy.h>
#include <vtkEdgeLayout.h>
#include <vtkSmartPointer.h>
#include <vtkVariantArray.h>

#include <QColor>

#include <vector>
#include <span>
#include <ranges>

#include "../utility.hpp"
#include "../edge.hpp"
#include "binaryReader.hpp"
#include "neuronProperties.hpp"
#include "visUtility.hpp"

#include "loaders.hpp"




void loadPositions(vtkPoints& positions, std::vector<uint16_t>& mapping) {
    vtkNew<vtkDelimitedTextReader> reader;
    std::string path = (dataFolder / "positions/rank_0_positions.txt").string();
    reader->SetFileName(path.data());
    reader->DetectNumericColumnsOn();
    reader->SetFieldDelimiterCharacters(" ");
    reader->Update();

    int point_index = -1;
    vtkTable* table = reader->GetOutput();

    mapping.resize(table->GetNumberOfRows());
    //the first row is header
    for (vtkIdType i = 1; i < table->GetNumberOfRows(); i++) {
        if (table->GetValue(i, 0).ToString() == "#") continue;

        double point[] = {
                (table->GetValue(i, 1)).ToDouble(),
                (table->GetValue(i, 2)).ToDouble(),
                (table->GetValue(i, 3)).ToDouble()
        };

        if (point_index == -1 || manhattanDist(positions.GetPoint(point_index), point) > 0.5) {
            positions.InsertNextPoint(point);
            point_index++;
        }
        mapping[(table->GetValue(i, 0)).ToInt() - 1] = point_index;
    }
}

void loadEdges(vtkMutableDirectedGraph& g, std::vector<uint16_t> map, int timestep) {
    timestep = timestep / 100 * 100;
    if (timestep % 100 != 0) {
        return;
    }
    auto path = (dataFolder / "network-bin/rank_0_step_").string() + std::to_string(timestep * 100) + "_in_network";
    BinaryReader<Edge> reader(path);

    for (unsigned i = 0; i < reader.count(); i++) {
        Edge edge = reader.read();
        if (edge.weight <= 3) {
            break;
        }
        g.AddEdge(edge.from, edge.to);
    }
}

vtkNew<vtkUnsignedCharArray> loadColors(int timestep, int colorAttribute, const std::vector<uint16_t>& map, 
    double mini, double maxi, Range pointFilter) {
    const int pointCount = 50000;
    auto path = (dataFolder / "monitors-bin/timestep").string() + std::to_string(timestep);
    
    BinaryReader<NeuronProperties> reader(path);
    if (reader.count() < pointCount) {
        std::cout << "File:\"" << path << "\" is too small.\n";
        std::cout << sizeof(NeuronProperties) * pointCount << "bytes expected.\n";
    }

    vtkNew<vtkUnsignedCharArray> colors;
    colors->SetName("colors");
    colors->SetNumberOfComponents(4);
    
    ColorMixer colorMixer(QColor::fromRgbF(0, 0, 1), QColor::fromRgbF(0.7, 0.7, 0.7), QColor::fromRgbF(1, 0, 0), 0.5);

    for (int i = 0; i < pointCount; i++) {
        NeuronProperties neuron = reader.read();

        if (i == 0 || map[i] == map[i - 1]) continue;

        auto val = (neuron.projection(colorAttribute) - mini) / (maxi - mini);
        QColor color = colorMixer.getColor(val);

        unsigned char alpha = pointFilter.inRange(val) ? 255 : 0;

        std::array<unsigned char, 4> colorBytes = { (unsigned char)color.red(), (unsigned char)color.green(), (unsigned char)color.blue(), alpha };

        colors->InsertNextTypedTuple(colorBytes.data());
    }

    return colors;
}

vtkNew<vtkUnsignedCharArray> loadAggregatedColors(int timestepCount, int pointCount) {
    std::vector<float> values(pointCount);

    auto projection = [](NeuronProperties& prop) {
        return (float)prop.fired;
    };
    
    for (int j = 0; j < timestepCount; j++) {
        auto path = (dataFolder / "monitors-bin/timestep").string() + std::to_string(j);
        BinaryReader<NeuronProperties> reader(path);

        if (reader.count() < pointCount) {
            std::cout << "File:\"" << path << "\" is too small.\n";
            std::cout << sizeof(NeuronProperties) * pointCount << "bytes expected.\n";
        }

        for (int i = 0; i < pointCount; i++) {
            NeuronProperties neuron = reader.read();
            values[i] += projection(neuron);
        }
    }

    vtkNew<vtkUnsignedCharArray> colors;
    colors->SetName("colors");
    colors->SetNumberOfComponents(3);

    float mini = INFINITY, maxi = -INFINITY;
    for (int i = 0; i < pointCount; i++) {
        values[i] /= pointCount;
        mini = std::min(values[i], mini);
        maxi = std::max(values[i], maxi);
    }

    for (int i = 0; i < pointCount; i++) {
        std::array<unsigned char, 3> color = { 255, 255, 255 };

        auto val = (values[i] - mini) / (maxi - mini);
        val = val * 2 - 1;

        if (val > 0) {
            color[1] = 255 - 255 * val;
            color[2] = color[1];
        }
        else {
            color[1] = 255 - 255 * val;
            color[0] = color[1];
        }

        colors->InsertNextTypedTuple(color.data());
    }

    return colors;
}

vtkNew<vtkUnsignedCharArray> colorsFromPositions(vtkPoints& points) {
    vtkNew<vtkUnsignedCharArray> colors;
    colors->SetName("colors");
    colors->SetNumberOfComponents(3);

    std::array<double, 3> prev_point{};
    auto color = generateNiceColor();

    for (int i = 0; i < points.GetNumberOfPoints(); i++) {
        if (manhattanDist(points.GetPoint(i), prev_point.data()) > 0.5) {
            auto point = points.GetPoint(i);
            prev_point = { point[0], point[1], point[2] };
            color = generateNiceColor();
        }

        colors->InsertNextTypedTuple(color.data());
    }

    return colors;
}

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

template<typename StoredType, char deliminer>
std::vector<std::vector<StoredType>> parseCSV(std::string path) {

    std::vector<std::vector<StoredType>> result;
    std::ifstream file(path);
    checkFile(file);
        
    std::string line;
    while (file.good()) {
        if (line.size() != 0 && line[0] != '#') {
            result.emplace_back();
            for(auto token: std::ranges::views::split(line, deliminer)){
                StoredType item;
                auto [ptr, ec] = std::from_chars(token.data(), token.data() + token.size(), item);
                if (ec != std::errc() || ptr != token.data() + token.size()) {
                    assert(false);
                }
                result.back().emplace_back(item);
            }
        }
        std::getline(file, line);
    }
    return result;
}



void HistogramDataLoader::ensureLoaded(int colorAttribute) {
    if (dataState[colorAttribute] == Loaded) {
        return;
    }

    auto unloaded = Unloaded;
    if (dataState[colorAttribute].compare_exchange_strong(unloaded, Loading)) {
        histogramData[colorAttribute] = parseCSV<int, ' '>((dataFolder / "monitors-hist-real/").string() + attributeToString(colorAttribute));
        summaryData[colorAttribute] = parseCSV<double, ' '>((dataFolder / "monitors-histogram/").string() + attributeToString(colorAttribute));
        dataState[colorAttribute] = Loaded;
        dataState[colorAttribute].notify_all();
    }
    else {
        dataState[colorAttribute].wait(Loaded);
    }
    }

std::span<std::vector<double>> HistogramDataLoader::getSummaryData(int colorAttribute) {
    ensureLoaded(colorAttribute);
    return summaryData[colorAttribute];
}

std::span<std::vector<int>> HistogramDataLoader::getHistogramData(int colorAttribute) {
    ensureLoaded(colorAttribute);
    return histogramData[colorAttribute];
}

HistogramDataLoader::HistogramDataLoader() {
    backgroundThread = std::thread([this]() {
        for (int i = 0; i < 12; i++) {
            if (!continueBackground) {
                return;
            }
            ensureLoaded(i);
        }
    });
}

HistogramDataLoader::~HistogramDataLoader() {
    continueBackground = false;
    if (backgroundThread.joinable()) {
        backgroundThread.join();
    }
};
