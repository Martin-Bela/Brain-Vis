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

#include <optional>
#include <QComboBox>


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
        std::ifstream file(path, std::ios::binary);
        checkFile(file);

        int edge_n = 0;
        std::map<int, int> edge_count;
        std::vector<int> edges;
        for (unsigned i = 0; i < std::filesystem::file_size(path) / sizeof(Edge); i++) {
            Edge edge;
            file.read(reinterpret_cast<char*>(&edge), sizeof(edge));

            if (edge_count.contains(10000 * map[edge.from] + map[edge.to])) {
                edge_count[10000 * map[edge.from] + map[edge.to]]++;
            }
            else {
                edges.push_back(10000 * map[edge.from] + map[edge.to]);
                edge_count[10000 * map[edge.from] + map[edge.to]] = 1;
            }
            //checkFile(file);
        }

        for (int val : edges) {
            if (edge_count[val] >= 5) {
                g.AddEdge(val / 10000, val % 10000);
                edge_n++;
            }
        }

        cout << edge_n << endl;
        checkFile(file);
    }

    vtkNew<vtkUnsignedCharArray> loadColorsInefficient(int timestep, int attribute) {
        const int pointCount = 5000;
        std::string path;
        if (timestep == 0) {
            path = (dataFolder / "monitors2/monitors_0.csv").string();
        }
        else {
            path = (dataFolder / "monitors2/monitors_").string() + std::to_string(timestep) + "0000.csv";
        }

        vtkNew<vtkDelimitedTextReader> reader;
        reader->SetFileName(path.data());
        reader->DetectNumericColumnsOn();
        reader->SetFieldDelimiterCharacters(";");
        reader->Update();

        vtkNew<vtkUnsignedCharArray> colors;
        colors->SetName("colors");
        colors->SetNumberOfComponents(3);
        
        std::vector<double> values;

        vtkTable* table = reader->GetOutput();

        values.reserve(table->GetNumberOfRows());
        for (vtkIdType i = 0; i < table->GetNumberOfRows(); i++) {
            values.push_back( table->GetValue(i, attribute).ToDouble() );
        }

        double mini = 0, maxi = 0.0;
        
        double avg = 0;
        for (int i = 0; i < pointCount; i++) {
            //mini = std::min(values[i], mini);
            maxi = std::max(values[i], maxi);
            avg += values[i];
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
                color[1] = 255 + 255 * val;
                color[0] = color[1];
            }
            

            /*if (values[i] > 0.5) {
                color[1] = 0;
                color[2] = 0;
            }
            else {
                color[1] = 0;
                color[0] = 0;
            }*/
            
            colors->InsertNextTypedTuple(color.data());
        }
        return colors;
    }

    vtkNew<vtkUnsignedCharArray> loadColors(int timestep, std::map<int, int> map) {
        const int pointCount = 50000;
        auto path = (dataFolder / "monitors-bin/timestep").string() + std::to_string(100 * timestep);
        
        auto size = std::filesystem::file_size(path);
        if (size < sizeof(NeuronProperties) * pointCount) {
            std::cout << "File:\"" << path << "\" is too small.\n";
            std::cout << sizeof(NeuronProperties) * pointCount << "bytes expected.\n";
        }

        std::ifstream in(path, std::ios::binary);
        checkFile(in);

        vtkNew<vtkUnsignedCharArray> colors;
        colors->SetName("colors");
        colors->SetNumberOfComponents(3);

        auto projection = [](NeuronProperties& prop) {
            return (float) prop.calcium;
        };

        float mini = INFINITY, maxi = -INFINITY;
        for (int i = 0; i < pointCount; i++) {
            NeuronProperties neuron{};
            in.read(reinterpret_cast<char*>(&neuron), sizeof(NeuronProperties));
            checkFile(in);
            mini = std::min(projection(neuron), mini);
            maxi = std::max(projection(neuron), maxi);
        }
        in.seekg(0);

        for (int i = 0; i < pointCount; i++) {
            NeuronProperties neuron{};
            in.read(reinterpret_cast<char*>(&neuron), sizeof(NeuronProperties));
            checkFile(in);

            if (i == 0 || map[i] == map[i - 1]) continue;
            std::array<unsigned char, 3> color = { 255, 255, 255 };

            auto val = (projection(neuron) - mini) / (maxi - mini);
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
            auto size = std::filesystem::file_size(path);
            if (size < sizeof(NeuronProperties) * pointCount) {
                std::cout << "File:\"" << path << "\" is too small.\n";
                std::cout << sizeof(NeuronProperties) * pointCount << "bytes expected.\n";
            }

            std::ifstream in(path, std::ios::binary);
            checkFile(in);

            for (int i = 0; i < pointCount; i++) {
                NeuronProperties neuron{};
                in.read(reinterpret_cast<char*>(&neuron), sizeof(NeuronProperties));
                checkFile(in);

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

    class Visualisation : public QObject {
    public:
        Context context;
        vtkNew<vtkSphereSource> sphere;
        vtkNew<vtkPoints> points;
        vtkNew<vtkPolyData> polyData;
        vtkNew<vtkGlyph3DMapper> glyph3D;
        vtkNew<vtkActor> actor;

        int lastTimestep;
        int lastcolorAttribute;


        void loadData() {
            sphere->SetPhiResolution(10);
            sphere->SetThetaResolution(10);
            sphere->SetRadius(0.6);

            std::map<int, int> point_map = loadPositions(*points);
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

            vtkNew<vtkActor> arrowActor;
#if 1
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

    public slots:
        void changeTimestep(int timestep) {
            reloadColors(timestep, lastcolorAttribute);
        }

        void changeColorAttribute(int colorAttribute) {
            reloadColors(lastTimestep, colorAttribute);
        }

    private:
        void reloadColors(int timestep, int colorAttribute) {
            lastcolorAttribute = colorAttribute;
            lastTimestep = timestep;

            std::cout << std::format("Reloading colors - timestep: {}, attribute: {}\n", timestep, colorAttribute);

            auto colors = loadColorsInefficient(timestep, colorAttribute);
            //auto colors = colorsFromPositions(*points);
            polyData->GetPointData()->SetScalars(colors);
            glyph3D->Update();
            context.renderWindow->Render();
        }
    };

    class WidgetPanel {
    public:
        WidgetPanel(QMainWindow& window, Visualisation& vis) {
            // control area
            window.addDockWidget(Qt::LeftDockWidgetArea, &controlDock);
            controlDock.setTitleBarWidget(&controlDockTitle);
            controlDock.setWidget(&layoutContainer);

            controlDockTitle.setMargin(5);

            auto dockLayout = new QVBoxLayout{};
            layoutContainer.setLayout(dockLayout);
            
            auto attributeNames = std::to_array<const char*>({ "step", "fired", "fired fraction", "activity", "dampening", "current calcium",
                "target calcium", "synaptic input", "background input", "grown axons", "connected axons", "grown dendrites", "connected dendrites" });
            for (auto name : attributeNames) {
                combobox.addItem(name);
            }

            dockLayout->addWidget(&combobox);
            QObject::connect(&combobox, &QComboBox::currentIndexChanged, &vis, &Visualisation::changeColorAttribute);

            dockLayout->addWidget(&slider);
            QObject::connect(&slider, &QSlider::valueChanged, &vis, &Visualisation::changeTimestep);
        }

        QDockWidget controlDock;
        QLabel controlDockTitle{ "Control Dock" };
        QWidget layoutContainer;
        QSlider slider;
        QComboBox combobox;
    };

    class Application {
    public:
        Application(int argc, char** argv) {

            QSurfaceFormat::setDefaultFormat(QVTKOpenGLNativeWidget::defaultFormat());
            application.init(argc, argv);
            std::cout << QApplication::applicationDirPath().toStdString() << std::endl;

            mainWindow.init();
            mainWindow->resize(1200, 600);

            visualisation.init();
            visualisation->loadData();
            vtkWidget.init();
            vtkWidget->setRenderWindow(visualisation->context.renderWindow);
            mainWindow->setCentralWidget(vtkWidget.ptr());
            
            widgetPanel.init(mainWindow, visualisation);
        }

        int run() {
            mainWindow->show();

            visualisation->firstRender();

            return application->exec();
        }

        DeferredInit<QApplication> application;
        DeferredInit<QMainWindow> mainWindow;
        DeferredInit<QVTKOpenGLNativeWidget> vtkWidget;
        DeferredInit<WidgetPanel> widgetPanel;
        DeferredInit<Visualisation> visualisation;
    };

}//namepsace


int main(int argc, char** argv) {
    setCurrentDirectory();
    Application app(argc, argv);
    return app.run();
}