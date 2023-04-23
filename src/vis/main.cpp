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
#include <QComboBox>
#include <QString>

#include "context.hpp"
#include "visUtility.hpp"
#include "neuronProperties.hpp"
#include "slider.hpp"
#include "edge.hpp"
#include "binaryReader.hpp"
#include "histogramWidget.hpp"

#include "ui_mainWindow.h"

#include <unordered_map>
#include <optional>


namespace { //anonymous namespace

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

    vtkNew<vtkUnsignedCharArray> loadColors(int timestep, int colorAttribute, const std::vector<uint16_t>& map) {
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

            if (i == 0 || map[i] == map[i - 1]) continue;
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

        vtkNew<vtkGraphToPolyData> graphToPolyData;
        vtkNew<vtkArrowSource> arrowSource;
        vtkNew<vtkGlyph3D> arrowGlyph;
        vtkNew<vtkPolyDataMapper> arrowMapper;


        std::vector<uint16_t> point_map;

        int currentTimestep;
        int currentColorAttribute;
        bool edgesVisible = false;

        enum: int { edgesHidden = -1 };
        int edgeTimestep = edgesHidden;

        void loadData() {
            sphere->SetPhiResolution(10);
            sphere->SetThetaResolution(10);
            sphere->SetRadius(0.6);

            loadPositions(*points, point_map);
            // Add the coordinates of the points to the graph
#if 0
            vtkNew<vtkMutableDirectedGraph> g;
            g->SetNumberOfVertices(points->GetNumberOfPoints());
            g->SetPoints(points);
            loadEdges(*g, point_map);

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
            reloadColors(0, 0);
            reloadEdges();
            context.render();
        }

        void setHistogramWidgetPtr(HistogramWidget* histogramWidget) {
            histogramW = histogramWidget;
        }

    private:

        HistogramWidget *histogramW = nullptr;

        void reloadColors(int timestep, int colorAttribute) {
            currentColorAttribute = colorAttribute;
            currentTimestep = timestep;

            std::cout << std::format("Reloading colors - timestep: {}, attribute: {}\n", timestep, colorAttribute);

            auto colors = loadColors(timestep, colorAttribute, point_map);

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

    public slots:
        void changeTimestep(int timestep) {
            reloadColors(timestep, currentColorAttribute);
            reloadHistogram(currentTimestep, currentColorAttribute);
            // reloadEdges();
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

            changeTimestep(sliderValue);
        }

        // This is when we are selecting new thing
        void changeColorAttribute(int colorAttribute) {
            reloadColors(currentTimestep, colorAttribute);
            loadHistogramData(colorAttribute);
            reloadHistogram(currentTimestep, colorAttribute);
            context.render();
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

           
            mainUI->gridLayout->removeWidget(mainUI->slider);
            mainUI->gridLayout->addWidget(mainUI->slider, 0, 0, 1, 3);     
            
            QLabel* label0 = new QLabel();
            label0->setText("0");
            label0->setFixedHeight(16);
            mainUI->gridLayout->addWidget(label0, 1, 0, 1, 1, Qt::AlignLeft);
            QLabel* label1 = new QLabel();
            label1->setText("5000");
            label1->setFixedHeight(16);
            mainUI->gridLayout->addWidget(label1, 1, 1, 1, 1, Qt::AlignHCenter);
            QLabel* label2 = new QLabel();
            label2->setText("10000");
            label2->setFixedHeight(16);
            mainUI->gridLayout->addWidget(label2, 1, 2, 1, 1, Qt::AlignRight);


            QObject::connect(mainUI->comboBox, &QComboBox::currentIndexChanged, visualisation.ptr(), &Visualisation::changeColorAttribute);
            QObject::connect(mainUI->slider, &QSlider::valueChanged, visualisation.ptr(), &Visualisation::changeTimestepRange);
            QObject::connect(mainUI->showEdgesCheckBox, &QCheckBox::stateChanged, visualisation.ptr(), &Visualisation::showEdges);
            QObject::connect(mainUI->bottomPanel, &HistogramWidget::histogramClicked, visualisation.ptr(), &Visualisation::changeTimestep);
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