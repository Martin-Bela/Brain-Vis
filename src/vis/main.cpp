#include <vtkActor.h>
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
#include <QVBoxLayout>
#include <QVTKOpenGLNativeWidget.h>

#include <vtkDenseArray.h>

#include <QApplication>
#include <QDockWidget>
#include <QGridLayout>
#include <QLabel>
#include <QMainWindow>
#include <QPointer>
#include <QPushButton>
#include <QSlider>

#include "context.hpp"
#include "visUtility.hpp"
#include "neuronProperties.hpp"
#include "slider.hpp"
#include "edge.hpp"
#include "binaryReader.hpp"

#include "ui_mainWindow.h"

#include <optional>
#include <QComboBox>

#include "histogramWidget.hpp"


namespace { //anonymous namespace

    double manhattanDist(double* x, double* y) {
        auto dx = x[0] - y[0];
        auto dy = x[1] - y[1];
        auto dz = x[2] - y[2];
        return abs(dx) + abs(dy) + abs(dz);
    };

    std::map<int, int> loadPositions(vtkPoints& positions) {
        vtkNew<vtkDelimitedTextReader> reader;
        std::string path = (dataFolder / "positions/rank_0_positions.txt").string();
        reader->SetFileName(path.data());
        reader->DetectNumericColumnsOn();
        reader->SetFieldDelimiterCharacters(" ");
        reader->Update();

        int point_index = -1;
        std::map<int, int> map;
        vtkTable* table = reader->GetOutput();
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
            map[(table->GetValue(i, 0)).ToInt() - 1] = point_index;
        }
        return map;
    }

    void loadEdgesInefficient(vtkMutableDirectedGraph& g, const std::map<int, int>& map, int step) {
        auto path = (dataFolder.string() + "/network/rank_0_step_" + std::to_string(step) + "_in_network.txt");
        vtkNew<vtkDelimitedTextReader> reader;
        reader->SetFileName(path.data());
        reader->DetectNumericColumnsOn();
        reader->SetFieldDelimiterCharacters(" \t");
        reader->Update();

        int edge_n = 0;
        std::map<int, int> edge_count;

        vtkTable* table = reader->GetOutput();
        for (vtkIdType i = 1; i < table->GetNumberOfRows(); i++) {
            if (table->GetValue(i, 0).ToString() == "#") continue;

            int from = static_cast<uint16_t>(table->GetValue(i, 1).ToInt() - 1);
            int to = static_cast<uint16_t>(table->GetValue(i, 3).ToInt() - 1);

            edge_count[10000 * map.at(from) + map.at(to)]++;
        }

        for (auto&[val, count] : edge_count) {
            if (count >= 5) {
                g.AddEdge(val / 10000, val % 10000);
                edge_n++;
            }
        }

        cout << edge_n << endl;
    }

    void loadEdges(vtkMutableDirectedGraph& g, std::map<int, int> map) {
        auto path = (dataFolder / "network-bin/rank_0_step_0_in_network").string();
        BinaryReader<Edge> reader(path);

        int edge_n = 0;
        std::map<int, int> edge_count;
        std::vector<int> edges;
        for (unsigned i = 0; i < reader.count(); i++) {
            Edge edge = reader.read();

            if (edge_count.contains(10000 * map[edge.from] + map[edge.to])) {
                edge_count[10000 * map[edge.from] + map[edge.to]]++;
            }
            else {
                edges.push_back(10000 * map[edge.from] + map[edge.to]);
                edge_count[10000 * map[edge.from] + map[edge.to]] = 1;
            }
        }

        for (int val : edges) {
            if (edge_count[val] >= 5) {
                g.AddEdge(val / 10000, val % 10000);
                edge_n++;
            }
        }

        cout << edge_n << endl;
    }

    vtkNew<vtkUnsignedCharArray> loadColors(int timestep, int colorAttribute, const std::map<int, int>& map) {
        const int pointCount = 50000;
        auto path = (dataFolder / "monitors-bin/timestep").string() + std::to_string(timestep);
        
        BinaryReader<NeuronProperties> reader(path);
        if (reader.count() < pointCount) {
            std::cout << "File:\"" << path << "\" is too small.\n";
            std::cout << sizeof(NeuronProperties) * pointCount << "bytes expected.\n";
        }

        vtkNew<vtkUnsignedCharArray> colors;
        colors->SetName("colors");
        colors->SetNumberOfComponents(3);

        float mini = INFINITY, maxi = -INFINITY;
        for (int i = 0; i < pointCount; i++) {
            NeuronProperties neuron = reader.read();
            mini = std::min(neuron.projection(colorAttribute), mini);
            maxi = std::max(neuron.projection(colorAttribute), maxi);
        }
        reader.setPos(0);

        for (int i = 0; i < pointCount; i++) {
            NeuronProperties neuron = reader.read();

            if (i == 0 || map.at(i) == map.at(i - 1)) continue;
            std::array<unsigned char, 3> color = { 255, 255, 255 };

            auto val = (neuron.projection(colorAttribute) - mini) / (maxi - mini);
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

    void loadHistogramsFromFile(int attributeId, vtkTable &widgetTable, vtkTable &summaryData) {
        auto path = (dataFolder / "monitors-hist-real/").string() + attributeToString(attributeId);
        vtkNew<vtkDelimitedTextReader> reader;
        reader->SetFileName(path.data());
        reader->DetectNumericColumnsOn();
        reader->SetFieldDelimiterCharacters(" ");
        reader->SetHaveHeaders(false);
        reader->Update();

        vtkTable* table = reader->GetOutput();

        int timestepCount = table->GetNumberOfRows();
        int histogramSize = table->GetNumberOfColumns();
        //std::cout << "cols: " << table->GetNumberOfColumns() << ", rows:" << table->GetNumberOfRows() << "\n";
        widgetTable.DeepCopy(table);
        
        
        // Now we can load 'histogram' plot data

        auto path2 = (dataFolder / "monitors-histogram/").string() + attributeToString(attributeId);
        vtkNew<vtkDelimitedTextReader> reader2;
        reader2->SetFileName(path.data());
        reader2->DetectNumericColumnsOn();
        reader2->SetFieldDelimiterCharacters(" ");
        reader2->SetHaveHeaders(false);
        reader2->Update();

        vtkTable* table2 = reader->GetOutput();
        summaryData.DeepCopy(table2);

        // for (int i = 0; i < table->GetNumberOfRows(); i++) {
        //     for (int j = 0; j < table->GetNumberOfColumns(); j++)  {
        //         double val = 0;
        //         if (table->GetValue(i, j).IsNumeric()) {
        //             val = table->GetValue(i, j).ToInt();
        //         }
                
        //         max = std::fmax(max, val);
        //         arr.SetValue(i, j, val);
        //     }
        // }
    }

    class Visualisation : public QObject {
    public:
        Context context;
        vtkNew<vtkSphereSource> sphere;
        vtkNew<vtkPoints> points;
        vtkNew<vtkPolyData> polyData;
        vtkNew<vtkGlyph3DMapper> glyph3D;

        vtkNew<vtkActor> actor;
        vtkNew<vtkActor> arrowActor;

        std::map<int, int> point_map;

        int lastTimestep;
        int lastcolorAttribute;


        void loadData() {
            sphere->SetPhiResolution(10);
            sphere->SetThetaResolution(10);
            sphere->SetRadius(0.6);

            point_map = loadPositions(*points);
            // Add the coordinates of the points to the graph
#if 0
            vtkNew<vtkMutableDirectedGraph> g;
            g->SetNumberOfVertices(points->GetNumberOfPoints());
            g->SetPoints(points);
            loadEdges(*g);

            vtkNew<vtkBoostDividedEdgeBundling> edgeBundler;
            edgeBundler->SetInputDataObject(g);

            // Convert the graph to a polydata
            vtkNew<vtkGraphToPolyData> graphToPolyData;
            graphToPolyData->SetInputDataObject(g);
            //graphToPolyData->SetInputConnection(edgeBundler->GetOutputPort());
            graphToPolyData->Update();

            // Create a mapper and actor
            vtkNew<vtkPolyDataMapper> mapper;
            mapper->SetInputConnection(graphToPolyData->GetOutputPort());

            vtkNew<vtkActor> graphActor;
            graphActor->SetMapper(mapper);
            graphActor->GetProperty()->SetColor(namedColors->GetColor3d("Blue").GetData());
#endif       

#if 0
            vtkNew<vtkMutableDirectedGraph> g;
            // Add the coordinates of the points to the graph
            g->SetPoints(points);
            // TODO: Fix! This is terrible
            for (int i = 0; i < points->GetNumberOfPoints(); i++) {
                g->AddVertex();
            }
            loadEdgesInefficient(*g, point_map, 0);
            
            vtkNew<vtkGraphLayout> layout;
            vtkNew<vtkPassThroughLayoutStrategy> strategy;
            layout->SetInputData(g);
            layout->SetLayoutStrategy(strategy);

            vtkNew<vtkPassThroughEdgeStrategy> edgeStrategy;
            vtkNew<vtkEdgeLayout> edgeLayout;
            edgeLayout->SetLayoutStrategy(edgeStrategy);
            edgeLayout->SetInputConnection(layout->GetOutputPort());

            // Convert the graph to a polydata
            vtkNew<vtkGraphToPolyData> graphToPolyData;
            graphToPolyData->SetInputConnection(edgeLayout->GetOutputPort());
            graphToPolyData->EdgeGlyphOutputOn();
            graphToPolyData->SetEdgeGlyphPosition(0.0);
            graphToPolyData->Update();

            // Make a simple edge arrow for glyphing.
            vtkNew<vtkArrowSource> arrowSource;
            arrowSource->SetShaftRadius(0.01);
            arrowSource->SetTipRadius(0.02);
            arrowSource->Update();

            // Use Glyph3D to repeat the glyph on all edges.
            vtkNew<vtkGlyph3D> arrowGlyph;
            arrowGlyph->SetSourceConnection(arrowSource->GetOutputPort());
            arrowGlyph->SetInputConnection(0, graphToPolyData->GetOutputPort(1));
            arrowGlyph->SetScaleModeToScaleByVector();

            // Add the edge arrow actor to the view.
            vtkNew<vtkPolyDataMapper> arrowMapper;
            arrowMapper->SetInputConnection(arrowGlyph->GetOutputPort());
            
            arrowActor->SetMapper(arrowMapper);
            arrowActor->GetProperty()->SetOpacity(0.75);
            arrowActor->GetProperty()->SetColor(namedColors->GetColor3d("DarkGray").GetData());
#endif

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
            reloadColors(0, 0);
        }

        void setHistogramWidgetPtr(HistogramWidget* histogramWidget) {
            histogramW = histogramWidget;
        }

    public slots:
        void changeTimestep(int timestep) {
            reloadColors(timestep, lastcolorAttribute);
            reloadHistogram(lastTimestep, lastcolorAttribute);
        }

        // This is when we are selecting new thing
        void changeColorAttribute(int colorAttribute) {
            reloadColors(lastTimestep, colorAttribute);
            loadHistogramData(colorAttribute);
            reloadHistogram(lastTimestep, colorAttribute);
        }

        void showEdges(int state) {
            if (state == Qt::Checked) {
                context.renderer->AddActor(arrowActor);
            }
            else {
                context.renderer->RemoveActor(arrowActor);
            }
            context.render();
        }

    private:

        HistogramWidget *histogramW = nullptr;

        void reloadColors(int timestep, int colorAttribute) {
            lastcolorAttribute = colorAttribute;
            lastTimestep = timestep;

            std::cout << std::format("Reloading colors - timestep: {}, attribute: {}\n", timestep, colorAttribute);

            auto colors = loadColors(timestep, colorAttribute, point_map);
            //auto colors = colorsFromPositions(*points);
            polyData->GetPointData()->SetScalars(colors);
            glyph3D->Update();
            context.render();
        }

        void loadHistogramData(int colorAttribute) {
            auto t1 = std::chrono::high_resolution_clock::now();
            loadHistogramsFromFile(colorAttribute, histogramW->getHistogramDataRef(), histogramW->getSummaryDataRef());
            auto t2 = std::chrono::high_resolution_clock::now();
            auto ms_int = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1);
            std::cout << ms_int.count() << "ms\n";
            histogramW->onLoadHistogram();
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
    };

    class Application {
    public:
        Application(int argc, char** argv) {

            QSurfaceFormat::setDefaultFormat(QVTKOpenGLNativeWidget::defaultFormat());
            application.init(argc, argv);
            std::cout << QApplication::applicationDirPath().toStdString() << std::endl;

            //renderArea = new RenderArea;

            mainWindow.init();
            mainUI.init();
            mainUI->setupUi(mainWindow.ptr());

            visualisation.init();
            visualisation->loadData();

            visualisationWidget.init();
            visualisationWidget->setRenderWindow(visualisation->context.renderWindow);
            mainUI->mainVisDock->addWidget(visualisationWidget.ptr());

            // Set Histogram Widget so Visualization Class knwo about it!
            visualisation->setHistogramWidgetPtr(mainUI->bottomPanel);

            auto attributeNames = std::to_array<const char*>({ "fired", "fired fraction", "activity", "dampening", "current calcium",
                "target calcium", "synaptic input", "background input", "grown axons", "connected axons", "grown dendrites", "connected dendrites" });
            for (auto name : attributeNames) {
                mainUI->comboBox->addItem(name);
            }


            QObject::connect(mainUI->comboBox, &QComboBox::currentIndexChanged, visualisation.ptr(), &Visualisation::changeColorAttribute);
            QObject::connect(mainUI->slider, &QSlider::valueChanged, visualisation.ptr(), &Visualisation::changeTimestep);
            QObject::connect(mainUI->showEdgesCheckBox, &QCheckBox::stateChanged, visualisation.ptr(), &Visualisation::showEdges);

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