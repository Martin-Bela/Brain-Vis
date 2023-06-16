#pragma once

#include <vtkPoints.h>
#include <vtkNew.h>
#include <vtkUnsignedCharArray.h>
#include <vtkMutableDirectedGraph.h>

#include <numeric>
#include <atomic>
#include <vector>
#include <array>
#include <thread>
#include <span>

#include "visUtility.hpp"

struct Range {
    double lower_bound;
    double upper_bound;

    static Range Whole() {
        return { 0, 1 };
    }

    bool inRange(double x) {
        return lower_bound <= x && x <= upper_bound;
    }
};

struct AttributeData {
    std::span<std::vector<int>> histogram;
    std::span<std::vector<double>> summary;
    double propertyMin;
    double propertyMax;
};


std::pair<float, float> diffMinMax(int timestep, int colorAttribute);

void loadPositions(vtkPoints& originalPositions, vtkPoints& scatteredPositions, vtkPoints& aggregatedPositions, std::vector<uint16_t>& mapping);

vtkNew<vtkUnsignedCharArray> loadColors(int timestep, int colorAttribute, double mini, double maxi, Range pointFilter, bool derivatives);

void loadEdges(vtkMutableDirectedGraph& g, std::vector<uint16_t> map, int timestep);

std::string attributeToString(int attribute);


class HistogramDataLoader {
        enum dataState { Unloaded, Loading, Loaded };

        std::array<std::atomic<dataState>, 12> dataState;
        std::array<std::vector<std::vector<int>>, 12> histogramData;
        std::array<std::vector<std::vector<double>>, 12> summaryData;
        std::array<double, 12> minimums;
        std::array<double, 12> maximums;

        std::atomic<bool> continueBackground = true;
        std::thread backgroundThread;

        void ensureLoaded(int colorAttribute);

    public:
        AttributeData getAttributeData(int colorAttribute);

        HistogramDataLoader();
        
        ~HistogramDataLoader();
        // Disable copying and moving
        HistogramDataLoader(const HistogramDataLoader& other) = delete;
        HistogramDataLoader& operator=(const HistogramDataLoader& other) = delete;
        HistogramDataLoader(HistogramDataLoader&& other) = delete;
        HistogramDataLoader& operator=(HistogramDataLoader&& other) = delete;
    };
