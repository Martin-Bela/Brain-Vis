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

#include <QVTKOpenGLNativeWidget.h>

#include <QApplication>
#include <QDockWidget>
#include <QGridLayout>
#include <QLabel>
#include <QMainWindow>
#include <QPointer>
#include <QPushButton>
#include <QSlider>
#include <QComboBox>
#include <QString>
#include <QVBoxLayout>

#include "context.hpp"
#include "visUtility.hpp"
#include "neuronProperties.hpp"
#include "edge.hpp"
#include "binaryReader.hpp"
#include "histogramWidget.hpp"

#include "ui_mainWindow.h"

#include <unordered_map>
#include <optional>
#include <limits>
#include <ranges>


namespace { //anonymous namespace

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

            std::array<unsigned char, 4> colorBytes = { color.red(), color.green(), color.blue(), alpha };

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

    class HistogramDataLoader {
        enum dataState { Unloaded, Loading, Loaded };

        std::array<std::atomic<dataState>, 12> dataState;
        std::array<std::vector<std::vector<int>>, 12> histogramData;
        std::array<std::vector<std::vector<double>>, 12> summaryData;

        std::atomic<bool> continueBackground = true;
        std::thread backgroundThread;

        void ensureLoaded(int colorAttribute) {
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

    public:
        std::span<std::vector<double>> getSummaryData(int colorAttribute) {
            ensureLoaded(colorAttribute);
            return summaryData[colorAttribute];
        }

        std::span<std::vector<int>> getHistogramData(int colorAttribute) {
            ensureLoaded(colorAttribute);
            return histogramData[colorAttribute];
        }

        HistogramDataLoader() {
            backgroundThread = std::thread([this]() {
                for (int i = 0; i < 12; i++) {
                    if (!continueBackground) {
                        return;
                    }
                    ensureLoaded(i);
                }
            });
        }
        
        ~HistogramDataLoader() {
            continueBackground = false;
            if (backgroundThread.joinable()) {
                backgroundThread.join();
            }
        };

        // Disable copying and moving
        HistogramDataLoader(const HistogramDataLoader& other) = delete;
        HistogramDataLoader& operator=(const HistogramDataLoader& other) = delete;
        HistogramDataLoader(HistogramDataLoader&& other) = delete;
        HistogramDataLoader& operator=(HistogramDataLoader&& other) = delete;
    };

    class Visualisation : public QObject {
    public:
        Context context;
        vtkNew<vtkSphereSource> sphere;
        vtkNew<vtkPoints> points;
        vtkNew<vtkPolyData> polyData;
        vtkNew<vtkGlyph3DMapper> glyph3D;

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
        bool edgesVisible = false;

        HistogramDataLoader histogramDataLoader;

        enum: int { edgesHidden = -1 };
        int edgeTimestep = edgesHidden;

        HistogramWidget* histogramW = nullptr;
        HistogramSliderWidget* sliderWidget = nullptr;

        void loadData() {
            sphere->SetPhiResolution(10);
            sphere->SetThetaResolution(10);
            sphere->SetRadius(0.6);

            loadPositions(*points, point_map);     

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
            polyData->SetPoints(points);
            
            glyph3D->SetInputData(polyData);
            glyph3D->SetSourceConnection(sphere->GetOutputPort());
            glyph3D->Update();
            
            actor->SetMapper(glyph3D);
            actor->GetProperty()->SetPointSize(30);
            actor->GetProperty()->SetColor(namedColors->GetColor3d("Tomato").GetData());

            context.init({ actor, arrowActor });
        }

        void firstRender() {
            // loadTableData();
            loadHistogramData(0);
            reloadColors(0, 0);
            reloadEdges();
            context.render();
        }

        void setHistogramWidgetPtr(HistogramWidget* histogramWidget, HistogramSliderWidget* slider) {
            histogramW = histogramWidget;
            sliderWidget = slider;
        }

    private:

        void reloadColors(int timestep, int colorAttribute) {
            currentColorAttribute = colorAttribute;
            currentTimestep = timestep;

            std::cout << std::format("Reloading colors - timestep: {}, attribute: {}\n", timestep, colorAttribute);

            auto summaryData = histogramDataLoader.getSummaryData(currentColorAttribute);
            double propMin = summaryData[timestep][3];
            double propMax = summaryData[timestep][2];
            auto colors = loadColors(timestep, colorAttribute, point_map, propMin, propMax, pointFilter);

            //auto colors = colorsFromPositions(*points);
            polyData->GetPointData()->SetScalars(colors);
            glyph3D->Update();
        }

        void reloadEdges() {
            int newEdgeTimestep = edgesVisible ? currentTimestep / 100 * 100 : edgesHidden;
            if (edgeTimestep == newEdgeTimestep) {
                return;
            }
            edgeTimestep = newEdgeTimestep;
            std::cout << std::format("Edges reloaded with timestep {}\n", newEdgeTimestep);

            vtkNew<vtkMutableDirectedGraph> g;
            g->SetNumberOfVertices(points->GetNumberOfPoints());
            g->SetPoints(points);
            
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

            auto histData = histogramDataLoader.getHistogramData(colorAttribute);
            auto summData = histogramDataLoader.getSummaryData(colorAttribute);

            histogramW->setTableData(histData, summData);
            sliderWidget->setTableData(histData, summData);

            auto t2 = std::chrono::high_resolution_clock::now();
            std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1) << "\n";
            std::cout << "Histogram data for " << attributeToString(colorAttribute) << " loaded.\n";
        }

        void reloadHistogram(int timestep, int colorAttribute) {
            if (!histogramW->isLoaded()) {
                std::cout << "histogramWidget in Visualization is not loaded!\n";
                return;
            }
            histogramW->setTick(timestep);
            histogramW->update();
        }

    public slots:
        void setPointFilter(unsigned low, unsigned hight) {
            pointFilter = Range{ low / 100.0, hight / 100.0 };
            reloadColors(currentTimestep, currentColorAttribute);
            context.render();
        }

        void changeTimestep(int timestep) {
            reloadColors(timestep, currentColorAttribute);
            reloadHistogram(currentTimestep, currentColorAttribute);
            reloadEdges();
            context.render();
        }

        // Used for changing the sliding window of the histogramWidget
        void changeTimestepRange(int sliderValue) {
            const int minVal = 0;
            const int maxVal = 9999;

            int lowerBoundary = std::max(sliderValue - 250, minVal);
            int upperBoundary = std::min(sliderValue + 250, maxVal);

            if (lowerBoundary == minVal) upperBoundary = 500;
            if (upperBoundary == maxVal) lowerBoundary = maxVal - 500;
            histogramW->setVisibleRange(lowerBoundary, upperBoundary);

            std::cout << "Slider value:" << std::to_string(sliderValue) << std::endl;
            changeTimestep(sliderValue);
        }

        void changeDrawMode(int modeType) {
            histogramW->changeDrawMode((HistogramDrawMode)modeType);
            sliderWidget->changeDrawMode((HistogramDrawMode)modeType);
            reloadHistogram(currentTimestep, currentColorAttribute);
            sliderWidget->update();
        }

        void changeColorAttribute(int colorAttribute) {
            pointFilter = Range::Whole();
            loadHistogramData(colorAttribute);
            reloadColors(currentTimestep, colorAttribute);
            reloadHistogram(currentTimestep, colorAttribute);
            context.render();
            sliderWidget->update();
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

            histogramW->logarithmicScaleEnabled = logEnabled;
            sliderWidget->logarithmicScaleEnabled = logEnabled;
            histogramW->setDirty();
            sliderWidget->setDirty();
            sliderWidget->update();
            histogramW->update();
        }
    };

    class Application {
    public:
        Application(int argc, char** argv) {

            QSurfaceFormat::setDefaultFormat(QVTKOpenGLNativeWidget::defaultFormat());
            application.init(argc, argv);
            std::cout << QApplication::applicationDirPath().toStdString() << std::endl;

            mainWindow.init();
            mainUI.init();
            mainUI->setupUi(mainWindow.ptr());

            visualisation.init();
            visualisation->loadData();

            visualisationWidget.init();
            visualisationWidget->setRenderWindow(visualisation->context.renderWindow);
            mainUI->mainVisDock->addWidget(visualisationWidget.ptr());

            auto foo = mainUI->sliderWidget;
            // Set Histogram Widget so Visualization Class knows about it!
            visualisation->setHistogramWidgetPtr(mainUI->bottomPanel, mainUI->sliderWidget);
            mainUI->bottomPanel->setFocusPolicy(Qt::ClickFocus);
            mainUI->bottomPanel->setAttribute(Qt::WA_OpaquePaintEvent);
            mainUI->sliderWidget->setFocusPolicy(Qt::ClickFocus);
            mainUI->sliderWidget->setAttribute(Qt::WA_OpaquePaintEvent);

            auto attributeNames = std::to_array<const char*>({ "fired", "fired fraction", "activity", "dampening", "current calcium",
                "target calcium", "synaptic input", "background input", "grown axons", "connected axons", "grown dendrites", "connected dendrites" });
            for (auto name : attributeNames) {
                mainUI->comboBox->addItem(name);
            }
            mainUI->comboBox_2->addItem("Histogram");
            mainUI->comboBox_2->addItem("Summary");
            mainUI->comboBox_2->addItem("Both");

            // Remove title bars
            mainUI->leftDockWidget->setTitleBarWidget(new QWidget());
            mainUI->bottomDockWidget->setTitleBarWidget(new QWidget());
            mainUI->bottomDockWidget->setFixedHeight(200);

            QObject::connect(mainUI->comboBox, &QComboBox::currentIndexChanged, visualisation.ptr(), &Visualisation::changeColorAttribute);
            QObject::connect(mainUI->comboBox_2, &QComboBox::currentIndexChanged, visualisation.ptr(), &Visualisation::changeDrawMode);
            QObject::connect(mainUI->showEdgesCheckBox, &QCheckBox::stateChanged, visualisation.ptr(), &Visualisation::showEdges);
            QObject::connect(mainUI->bottomPanel, &HistogramWidget::histogramCursorMoved, visualisation.ptr(), &Visualisation::changeTimestep);
            QObject::connect(mainUI->sliderWidget, &HistogramSliderWidget::histogramCursorMoved, visualisation.ptr(), &Visualisation::changeTimestepRange);
            QObject::connect(mainUI->logScaleCheckbox, &QCheckBox::stateChanged, visualisation.ptr(), &Visualisation::logCheckboxChange);
            QObject::connect(mainUI->rangeSlider, &QRangeSlider::valueChange, visualisation.ptr(), &Visualisation::setPointFilter);
        }

        int run() {
            mainWindow->show();

            visualisation->firstRender();
            return application->exec();
        }

        DeferredInit<QApplication> application;
        DeferredInit<QMainWindow> mainWindow;
        DeferredInit<Ui::MainWindow> mainUI;

        DeferredInit<Visualisation> visualisation;
        DeferredInit<QVTKOpenGLNativeWidget> visualisationWidget;

        //DeferredInit<HistogramWidget> histogramWidget;

    };

}//namespace


int main(int argc, char** argv) {
    setCurrentDirectory();
    Application app(argc, argv);
    return app.run();
}