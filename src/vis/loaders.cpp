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
#include <random>

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



struct Point3 {
    double x;
    double y;
    double z;

    Point3(double x, double y, double z) {
        this->x = x;
        this->y = y;
        this->z = z;
    }

    void add(Point3 b) {
        x += b.x;
        y += b.y;
        z += b.z;
    }

    double size() {
        return sqrt(x * x + y * y + z * z);
    }

    void scale(double scale) {
        x *= scale;
        y *= scale;
        z *= scale;
    }

    void normalize() {
        double size = this->size();
        x /= size;
        y /= size;
        z /= size;
    }

    static double dist(Point3 a, Point3 b) {
        return sqrt((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y) + (a.z - b.z) * (a.z - b.z));
    }
    
    static Point3 cross(Point3 a, Point3 b) {
        return Point3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
    }

    static Point3 vector(Point3 a, Point3 b) {
        Point3 p = Point3(a.x - b.x, a.y - b.y, a.z - b.z);
        p.normalize();
        return p;
    }

};

std::vector<Point3> findClosestPoints(int index, std::vector<Point3>& points) {
    std::vector<Point3> closePoints{ Point3(0.0, 0.0, 0.0), Point3(0.0, 0.0, 0.0), Point3(0.0, 0.0, 0.0), Point3(0.0, 0.0, 0.0) };
    std::vector<double> distances{ 500.0, 500.0, 500.0, 500.0 };

    for (int i = 0; i < points.size(); i++) {
        if (i == index) continue;

        double dist = Point3::dist(points[index], points[i]);
        int pointsId = -1;
        int distId = -1;
        double bestDist = 0;
        for (int j = 0; j < 4; j++) {
            if (dist < distances[j] && distances[j] > bestDist) {
                bestDist = distances[j];
                pointsId = i;
                distId = j;
            }
        }

        if (pointsId != -1) {
            closePoints[distId] = points[pointsId];
            distances[distId] = Point3::dist(points[pointsId], points[index]);
        }
    }
    return closePoints;
}


void loadPositions(vtkPoints& originalPositions, vtkPoints& scatteredPositions, vtkPoints& aggregatedPositions, std::vector<uint16_t>& mapping) {
    vtkNew<vtkDelimitedTextReader> reader;
    std::string path = (dataFolder / "positions/rank_0_positions.txt").string();
    reader->SetFileName(path.data());
    reader->DetectNumericColumnsOn();
    reader->SetFieldDelimiterCharacters(" ");
    reader->Update();

    int point_index = 0;
    vtkTable* table = reader->GetOutput();

    // First find index of first row that is not a header/comment
    int begin = 0;
    for (vtkIdType i = 0; i < table->GetNumberOfRows(); i++) {
        if (table->GetValue(i, 0).ToString() != "#") {
            begin = i;
            break;
        }
    }

    mapping.resize(table->GetNumberOfRows());
    std::vector<Point3> averagePoints;
    Point3 current = Point3((table->GetValue(begin, 1)).ToDouble(), (table->GetValue(begin, 2)).ToDouble(), (table->GetValue(begin, 3)).ToDouble());
    Point3 average = Point3(0.0, 0.0, 0.0);
    int nPoints = 0;

    std::vector<Point3> points;
    std::vector<std::vector<Point3>> clusters;

    for (vtkIdType i = begin; i < table->GetNumberOfRows(); i++) {
        Point3 vPoint = Point3(
            (table->GetValue(i, 1)).ToDouble(),
            (table->GetValue(i, 2)).ToDouble(),
            (table->GetValue(i, 3)).ToDouble()
        );
        
        if (Point3::dist(current, vPoint) > 0.75) {
            clusters.push_back(points);
            points.clear();

            average.scale(1.0 / nPoints);
            averagePoints.push_back(average);
            double avgPoint[] = { average.x, average.y, average.z };
            aggregatedPositions.InsertNextPoint(avgPoint);
            point_index++;

            current = Point3(vPoint.x, vPoint.y, vPoint.z);
            average = Point3(0.0, 0.0, 0.0);
            nPoints = 0; 
        }

        average.add(vPoint);
        nPoints++;
        points.push_back(vPoint);
        mapping[(table->GetValue(i, 0)).ToInt() - 1] = point_index;
    }

    clusters.push_back(points);
    average.scale(1.0 / nPoints);
    averagePoints.push_back(average);

    double avgPoint[] = { average.x, average.y, average.z };
    aggregatedPositions.InsertNextPoint(avgPoint);

    // This is the original code

    for (std::vector<Point3> cluster : clusters) {
        for (Point3 cPoint : cluster) {
            double mPoint[] = { cPoint.x, cPoint.y, cPoint.z };
            originalPositions.InsertNextPoint(mPoint);
        }
    }

    


    // This generates random poisson disk patterns (equally spaced points)
    // That are used to place the point clusters
    std::vector<std::vector<double>> random_s;
    std::vector<std::vector<double>> random_t;
    const double collisionDistance = 0.6;
    const int n_patterns = 8;
    for (int p = 0; p < n_patterns; p++) {
        std::vector<double> ss = { 0.0 };
        std::vector<double> tt = { 0.0 };

        while (ss.size() < 10) {
            double s = 2 * ((double)rand() / RAND_MAX) - 1;
            double t = 2 * ((double)rand() / RAND_MAX) - 1;

            bool hit = false;
            for (int i = 0; i < ss.size(); i++) {
                if (sqrt((s - ss[i]) * (s - ss[i]) + (t - tt[i]) * (t - tt[i])) < collisionDistance) {
                    hit = true;
                    break;
                }
            }
            if (hit == false) {
                ss.push_back(s);
                tt.push_back(t);
            }
        }

        random_s.push_back(ss);
        random_t.push_back(tt);
    }


    // This calculates the point positions
    for (int i = 0; i < averagePoints.size(); i++) {
        // First find 4 closest non-cluster points to given points
        std::vector<Point3> closePoints = findClosestPoints(i, averagePoints);
        double avgDistance = 0.0;
        for (Point3 &point : closePoints) {
            avgDistance += Point3::dist(point, averagePoints[i]);
        }
        avgDistance /= closePoints.size();

        // Approximate derivatives using the 4 points by finding tuples whose vectors
        // have the biggest angle between them
        int possibilities[4][4] = { {0, 1, 2, 3}, {0, 2, 1, 3}, {0, 3, 1, 2}, {1, 2, 0, 3} };
        int* best = possibilities[0];
        double minDot = 1.0;
        for (int* v : possibilities) {
            Point3 first = Point3::vector(closePoints[v[0]], closePoints[v[1]]);
            Point3 second = Point3::vector(closePoints[v[2]], closePoints[v[3]]);
            double dot = first.x * second.x + first.y * second.y + first.z * second.z;
            if (abs(dot) < minDot) {
                best = v;
                minDot = abs(dot);
            }
        }

        Point3 first = Point3::vector(closePoints[best[0]], closePoints[best[1]]);
        Point3 second = Point3::vector(closePoints[best[2]], closePoints[best[3]]);
        Point3 normal = Point3::cross(first, second);
        normal.normalize();

        Point3 pa = Point3::cross(normal, first);
        Point3 pb = Point3::cross(normal, pa);
        pa.normalize();
        pb.normalize();

        // Generate the points using the calculated normal vector at that point + random patterns
        // Make it adaptive because why shouldnt I
        double scale = std::clamp(avgDistance / 2.0, 1.0, 3.0);
        int pattern = rand() % n_patterns;
        for (int j = 0; j < random_s[pattern].size(); j++) {
            double s = random_s[pattern][j];
            double t = random_t[pattern][j];
            double mPoint[] = { averagePoints[i].x + scale * (s * pa.x + t * pb.x), averagePoints[i].y + scale * (s * pa.y + t * pb.y), averagePoints[i].z + scale * (s * pa.z + t * pb.z) };
            scatteredPositions.InsertNextPoint(mPoint);
        }
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

vtkNew<vtkUnsignedCharArray> loadColors(int timestep, int colorAttribute, double mini, double maxi, Range pointFilter, bool derivatives) {
    if (derivatives && timestep == 0) {
        timestep = 1;
    }

    const int pointCount = 50000;
    auto path = (dataFolder / "monitors-bin/timestep").string() + std::to_string(timestep);
    
    BinaryReader<NeuronProperties> reader(path);
    if (reader.count() < pointCount) {
        std::cout << "File:\"" << path << "\" is too small.\n";
        std::cout << sizeof(NeuronProperties) * pointCount << "bytes expected.\n";
    }

    vtkNew<vtkUnsignedCharArray> colors;
    colors->SetNumberOfComponents(4);
    
    ColorMixer colorMixer(QColor::fromRgbF(0, 0, 1), QColor::fromRgbF(0.7, 0.7, 0.7), QColor::fromRgbF(1, 0, 0), 0.5);

    if (!derivatives) {
        for (int i = 0; i < pointCount; i++) {
            NeuronProperties neuron = reader.read();

            //if (i == 0 || map[i] == map[i - 1]) continue;

            auto val = (neuron.projection(colorAttribute) - mini) / (maxi - mini);
            QColor color = colorMixer.getColor(val);

            unsigned char alpha = pointFilter.inRange(val) ? 255 : 0;

            std::array<unsigned char, 4> colorBytes = { (unsigned char)color.red(), (unsigned char)color.green(), (unsigned char)color.blue(), alpha };

            colors->InsertNextTypedTuple(colorBytes.data());
        }
    }
    else {
        auto previousTimestepPath = (dataFolder / "monitors-bin/timestep").string() + std::to_string(timestep - 1);
        BinaryReader<NeuronProperties> previousReader(previousTimestepPath);

        for (int i = 0; i < pointCount; i++) {
            NeuronProperties properties = reader.read();
            NeuronProperties prevProperties = previousReader.read();

            auto difference = properties.projection(colorAttribute) - prevProperties.projection(colorAttribute);
            auto max_diff = maxi - mini;
            auto val = difference / max_diff / 2 + 0.5;
            QColor color = colorMixer.getColor(val);

            unsigned char alpha = pointFilter.inRange(val) ? 255 : 0;

            std::array<unsigned char, 4> colorBytes = { (unsigned char)color.red(), (unsigned char)color.green(), (unsigned char)color.blue(), alpha };

            colors->InsertNextTypedTuple(colorBytes.data());
        }
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
        dataState[colorAttribute].wait(Loading);
    }
}

std::span<std::vector<double>> HistogramDataLoader::getSummaryData(int colorAttribute) {
    ensureLoaded(colorAttribute);
    assert(summaryData[colorAttribute].size() > 0);
    return summaryData[colorAttribute];
}

std::span<std::vector<int>> HistogramDataLoader::getHistogramData(int colorAttribute) {
    ensureLoaded(colorAttribute);
    assert(histogramData[colorAttribute].size() > 0);
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
