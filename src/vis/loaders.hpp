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



#include "../utility.hpp"

struct Range {
        double lower_bound;
        double upper_bound;

        static Range Whole() {
            return { std::numeric_limits<double>::lowest(), std::numeric_limits<double>::max() };
        }

        bool inRange(double x) {
            return lower_bound <= x && x <= upper_bound;
        }
    };


extern void loadPositions(vtkPoints& positions, std::vector<uint16_t>& mapping);
extern vtkNew<vtkUnsignedCharArray> loadColors(int timestep, int colorAttribute, const std::vector<uint16_t>& map, 
    double mini, double maxi, Range pointFilter);
extern void loadEdges(vtkMutableDirectedGraph& g, std::vector<uint16_t> map, int timestep);
extern std::string attributeToString(int attribute);



class HistogramDataLoader {
        enum dataState { Unloaded, Loading, Loaded };

        std::array<std::atomic<dataState>, 12> dataState;
        std::array<std::vector<std::vector<int>>, 12> histogramData;
        std::array<std::vector<std::vector<double>>, 12> summaryData;

        std::atomic<bool> continueBackground = true;
        std::thread backgroundThread;

        void ensureLoaded(int colorAttribute);

    public:
        std::span<std::vector<double>> getSummaryData(int colorAttribute);

        std::span<std::vector<int>> getHistogramData(int colorAttribute);

        HistogramDataLoader();
        
        ~HistogramDataLoader();
        // Disable copying and moving
        HistogramDataLoader(const HistogramDataLoader& other) = delete;
        HistogramDataLoader& operator=(const HistogramDataLoader& other) = delete;
        HistogramDataLoader(HistogramDataLoader&& other) = delete;
        HistogramDataLoader& operator=(HistogramDataLoader&& other) = delete;
    };
