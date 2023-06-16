#pragma once

#include <vtkArrowSource.h>
#include <vtkGraphToPolyData.h>
#include <vtkGlyph3DMapper.h>
#include <vtkActor.h>
#include <vtkPointData.h>
#include <vtkProperty.h>
#include <vtkGraphLayout.h>
#include <vtkPassThroughEdgeStrategy.h>
#include <vtkPassThroughLayoutStrategy.h>
#include <vtkEdgeLayout.h>
#include <vtkSmartPointer.h>
#include <vtkPointGaussianMapper.h>

#include "visUtility.hpp"
#include "histogramWidget.hpp"
#include "histogramSliderWidget.hpp"
#include "rangeSliderWidget.hpp"
#include "loaders.hpp"

#include "context.hpp"

#include <QLabel>

struct Widgets {
    HistogramWidget* histogram = nullptr;
    HistogramSliderWidget* histogramSlider = nullptr;
    RangeSliderWidget* rangeSlider = nullptr;
    QLabel* minimumValLabel = nullptr;
    QLabel* maximumValLabel = nullptr;
    QLabel* timestepLabel = nullptr;
    QLabel* neuronGlobalPropertiesLabel = nullptr;
    QLabel* neuronCurrentTimestepPropertiesLabel = nullptr;
};


class Visualisation : public QObject {
public:
    Context context;
    vtkNew<vtkPoints> originalPositions;
    vtkNew<vtkPoints> scatteredPositions;

    vtkNew<vtkPoints> aggregatedPoints;
    vtkNew<vtkPolyData> polyData;
    vtkNew<vtkPointGaussianMapper> pointGaussianMapper;

    vtkNew<vtkActor> actor;
    vtkNew<vtkActor> arrowActor;

    vtkNew<vtkGraphToPolyData> graphToPolyData;
    vtkNew<vtkArrowSource> arrowSource;
    vtkNew<vtkGlyph3D> arrowGlyph;
    vtkNew<vtkPolyDataMapper> arrowMapper;


    std::vector<uint16_t> point_map;

    Range pointFilter = Range::Whole();

    int currentTimestep = 0;
    int currentColorAttribute = 0;
    bool derivatives = false;

    bool edgesVisible = false;

    HistogramDataLoader histogramDataLoader;

    enum : int { edgesHidden = -1 };
    int edgeTimestep = edgesHidden;

    Widgets widgets;

    Visualisation(Widgets widgets) :
        widgets(widgets) { }

    void loadData() {

        loadPositions(*originalPositions, *scatteredPositions, *aggregatedPoints, point_map);

        // Convert the graph to a polydata
        graphToPolyData->EdgeGlyphOutputOn();
        graphToPolyData->SetEdgeGlyphPosition(0.0);

        // Make a simple edge arrow for glyphing.
        arrowSource->SetShaftRadius(0.01);
        arrowSource->SetTipRadius(0.02);
        arrowSource->Update();

        // Use Glyph3D to repeat the glyph on all edges.
        arrowGlyph->SetInputConnection(0, graphToPolyData->GetOutputPort(1));
        arrowGlyph->SetSourceConnection(arrowSource->GetOutputPort());
        arrowGlyph->SetScaleModeToScaleByVector();

        arrowActor->SetMapper(arrowMapper);
        arrowActor->GetProperty()->SetOpacity(0.75);
        arrowActor->GetProperty()->SetColor(namedColors->GetColor3d("DarkGray").GetData());

        // Points
        polyData->SetPoints(originalPositions);

        pointGaussianMapper->SetInputData(polyData);
        //pointGaussianMapper->SetScalarRange(range);
        //pointGaussianMapper->SetScaleFactor(0.9);
        pointGaussianMapper->EmissiveOff();
        pointGaussianMapper->SetColorModeToDirectScalars();

        if (false) {
            std::ifstream shaderFile("src/vis/shaders/bilboard.frag");
            cout << "Loading fog shader!\n";

            std::string shaderCode = readWholeFile(shaderFile);
            pointGaussianMapper->SetSplatShaderCode(shaderCode.c_str());
        }
        else {
            pointGaussianMapper->SetSplatShaderCode(
#include "vis/shaders/_bilboard_fallback.frag"
            );
        };

        actor->SetMapper(pointGaussianMapper);
        //actor->GetProperty()->SetOpacity(0.5);
        actor->GetProperty()->SetPointSize(30);
        actor->GetProperty()->SetColor(namedColors->GetColor3d("Tomato").GetData());

        context.init({ actor, arrowActor });
    }

    void firstRender() {
        loadHistogramData(0);
        reloadColors(0, 0, false);
        reloadEdges();
        context.render();
    }

private:

    void reloadColors(int timestep, int colorAttribute, bool derivatives) {
        currentColorAttribute = colorAttribute;
        currentTimestep = timestep;

        widgets.timestepLabel->setText(QString::fromStdString(std::to_string(timestep * 100)));
        std::cout << std::format("Reloading colors - timestep: {}, attribute: {}\n", timestep * 100, colorAttribute);

        auto attributeData = histogramDataLoader.getAttributeData(currentColorAttribute);

        double labelMin = attributeData.globalStatistics.min;
        double labelMax = attributeData.globalStatistics.max;

        if (derivatives) {
            std::tie(labelMin, labelMax) = diffMinMax(timestep, colorAttribute);
        }

        widgets.minimumValLabel->setText(QString::fromStdString(std::format("{:.2}", std::lerp(labelMin, labelMax, pointFilter.lower_bound))));
        widgets.maximumValLabel->setText(QString::fromStdString(std::format("{:.2}", std::lerp(labelMin, labelMax, pointFilter.upper_bound))));

        auto& curStatistics = attributeData.summary[timestep];

        auto neuronCurrentPropertiesString = std::format("min: {}\nmax: {}\nmean: {:.5}", 
            curStatistics.min, curStatistics.max, curStatistics.mean);
        widgets.neuronCurrentTimestepPropertiesLabel->setText(QString::fromStdString(neuronCurrentPropertiesString));

        auto colors = loadColors(timestep, colorAttribute, labelMin, labelMax, pointFilter, derivatives);
        polyData->GetPointData()->SetScalars(colors);
    }

    void reloadEdges() {
        int newEdgeTimestep = edgesVisible ? currentTimestep / 100 * 100 : edgesHidden;
        if (edgeTimestep == newEdgeTimestep) {
            return;
        }
        edgeTimestep = newEdgeTimestep;
        std::cout << std::format("Edges reloaded with timestep {}\n", newEdgeTimestep);

        vtkNew<vtkMutableDirectedGraph> g;
        g->SetNumberOfVertices(aggregatedPoints->GetNumberOfPoints());
        g->SetPoints(aggregatedPoints);

        if (edgesVisible) {
            loadEdges(*g, point_map, currentTimestep);
        }

        vtkNew<vtkGraphLayout> layout;
        vtkNew<vtkPassThroughLayoutStrategy> strategy;
        layout->SetInputData(g);
        layout->SetLayoutStrategy(strategy);

        vtkNew<vtkPassThroughEdgeStrategy> edgeStrategy;
        vtkNew<vtkEdgeLayout> edgeLayout;
        edgeLayout->SetLayoutStrategy(edgeStrategy);
        edgeLayout->SetInputConnection(layout->GetOutputPort());

        graphToPolyData->SetInputConnection(edgeLayout->GetOutputPort());
        graphToPolyData->Update();

        arrowMapper->SetInputConnection(arrowGlyph->GetOutputPort());
        arrowMapper->Update();
    }

    void loadHistogramData(int colorAttribute) {
        auto t1 = std::chrono::high_resolution_clock::now();

        auto attributeData = histogramDataLoader.getAttributeData(colorAttribute);

        auto globalStats = attributeData.globalStatistics;
        widgets.histogram->setTableData(attributeData.histogram, attributeData.summary, 
            globalStats);

        widgets.histogramSlider->setTableData(attributeData.histogram, attributeData.summary,
            globalStats);

        auto t2 = std::chrono::high_resolution_clock::now();
        std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1) << "\n";
        std::cout << "Histogram data for " << attributeToString(colorAttribute) << " loaded.\n";

        widgets.rangeSlider->setLowValue(0);
        widgets.rangeSlider->setHighValue(100);

        auto neuronPropertiesString = std::format("min: {}\nmax: {}\nmean: {:.5}",
            attributeData.globalStatistics.min, attributeData.globalStatistics.max, attributeData.globalStatistics.mean);
        widgets.neuronGlobalPropertiesLabel->setText(QString::fromStdString(neuronPropertiesString));
    }

    void reloadHistogram(int timestep, int colorAttribute) {
        if (!widgets.histogram->isLoaded()) {
            std::cout << "histogramWidget in Visualization is not loaded!\n";
            return;
        }
        widgets.histogram->setTick(timestep);
        widgets.histogram->repaint();
    }

public slots:
    void setPointScattering(int state) {
        bool scatter = state == Qt::Checked;
        if (scatter) {
            polyData->SetPoints(scatteredPositions);
        }
        else {
            polyData->SetPoints(originalPositions);
        }
        context.render();
    }

    void showDerivatives(int state) {
        derivatives = state == Qt::Checked;
        reloadColors(currentTimestep, currentColorAttribute, derivatives);
        context.render();
    }

    void setPointFilter(unsigned low, unsigned hight) {
        pointFilter = Range{ low / 100.0, hight / 100.0 };
        reloadColors(currentTimestep, currentColorAttribute, derivatives);
        context.render();
    }

    void changeTimestep(int timestep) {
        reloadColors(timestep, currentColorAttribute, derivatives);
        reloadHistogram(currentTimestep, currentColorAttribute);
        reloadEdges();
        context.render();
    }

    void changePointSize(int size) {
        pointGaussianMapper->SetScaleFactor(size / 100.0);
        context.render();
    }

    void changeTimestepRange(int sliderValue) {
        const int minVal = 0;
        const int maxVal = 9999;

        int lowerBoundary = std::max(sliderValue - 250, minVal);
        int upperBoundary = std::min(sliderValue + 250, maxVal);

        if (lowerBoundary == minVal) upperBoundary = 500;
        if (upperBoundary == maxVal) lowerBoundary = maxVal - 500;
        widgets.histogram->setVisibleRange(lowerBoundary, upperBoundary);

        std::cout << "Slider value:" << std::to_string(sliderValue) << std::endl;
        changeTimestep(sliderValue);
    }

    void changeDrawMode(int modeType) {
        widgets.histogram->changeDrawMode((HistogramDrawMode)modeType);
        widgets.histogramSlider->changeDrawMode((HistogramDrawMode)modeType);
        reloadHistogram(currentTimestep, currentColorAttribute);
        widgets.histogramSlider->update();
    }

    void changeColorAttribute(int colorAttribute) {
        pointFilter = Range::Whole();
        loadHistogramData(colorAttribute);
        reloadColors(currentTimestep, colorAttribute, derivatives);
        reloadHistogram(currentTimestep, colorAttribute);
        context.render();
        widgets.histogramSlider->update();
    }

    void showEdges(int state) {
        if (state == Qt::Checked) {
            edgesVisible = true;
        }
        else {
            edgesVisible = false;
        }
        reloadEdges();
        context.render();
    }

    void logCheckboxChange(int state) {
        bool logEnabled = state == Qt::Checked;

        widgets.histogram->logarithmicScaleEnabled = logEnabled;
        widgets.histogramSlider->logarithmicScaleEnabled = logEnabled;
        widgets.histogramSlider->update();
        widgets.histogram->update();
    }
};