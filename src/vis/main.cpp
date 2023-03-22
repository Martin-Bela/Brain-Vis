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
#include <vtkBoostDividedEdgeBundling.h>

#include "context.hpp"
#include "visUtility.hpp"
#include "neuronProperties.hpp"
#include "slider.hpp"
#include "edge.hpp"

namespace { //anonymous namespace

    void loadPositions(vtkPoints& positions) {
        vtkNew<vtkDelimitedTextReader> reader;
        auto path = (dataFolder / "positions/rank_0_positions.txt").string();
        reader->SetFileName(path.data());
        reader->DetectNumericColumnsOn();
        reader->SetFieldDelimiterCharacters(" ");
        reader->Update();

        vtkTable* table = reader->GetOutput();
        //the first row is header
        for (vtkIdType i = 1; i < table->GetNumberOfRows(); i++) {
            if (table->GetValue(i, 0).ToString() == "#") continue;

            positions.InsertNextPoint(
                (table->GetValue(i, 1)).ToDouble(),
                (table->GetValue(i, 2)).ToDouble(),
                (table->GetValue(i, 3)).ToDouble());
        }
    }


    void loadEdges(vtkMutableDirectedGraph& g) {
        auto path = (dataFolder / "network-bin/rank_0_step_0_in_network").string();
        std::ifstream file(path, std::ios::binary);
        checkFile(file);

        for (unsigned i = 0; i < std::filesystem::file_size(path) / sizeof(Edge); i++) {
            Edge edge;
            file.read(reinterpret_cast<char*>(&edge), sizeof(edge));
            
            g.AddEdge(edge.from, edge.to);
            //checkFile(file);
        }
        checkFile(file);
    }

    vtkNew<vtkUnsignedCharArray> loadColors(int timestep, int pointCount) {
        auto path = (dataFolder / "monitors-bin/timestep").string() + std::to_string(timestep);
        
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

    vtkNew<vtkUnsignedCharArray> colorsFromPositions(vtkPoints& points) {
        vtkNew<vtkUnsignedCharArray> colors;
        colors->SetName("colors");
        colors->SetNumberOfComponents(3);

        auto manhattan_dist = [](double* x, float* y) {
            auto dx = x[0] - y[0];
            auto dy = x[1] - y[1];
            auto dz = x[2] - y[2];

            return abs(dx) + abs(dy) + abs(dz);
        };
        
        std::array<float, 3> prev_point{};
        auto color = generateNiceColor();

        for (int i = 0; i < points.GetNumberOfPoints(); i++) {
            if (manhattan_dist(points.GetPoint(i), prev_point.data()) > 0.5) {
                auto point = points.GetPoint(i);
                prev_point = { float(point[0]), float(point[1]), float(point[2]) };
                color = generateNiceColor();
            }

            colors->InsertNextTypedTuple(color.data());
        }

        return colors;
    }


    class Visualisation {
    public:
        Context context;
        Slider slider;

        void run() {
            vtkNew<vtkSphereSource> sphere;
            sphere->SetPhiResolution(10);
            sphere->SetThetaResolution(10);
            sphere->SetRadius(0.3);

            vtkNew<vtkPoints> points;
            loadPositions(*points);
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

            vtkNew<vtkPolyData> polyData;
            polyData->SetPoints(points);
            
            vtkNew<vtkGlyph3DMapper> glyph3D;
            glyph3D->SetInputData(polyData);
            glyph3D->SetSourceConnection(sphere->GetOutputPort());
            glyph3D->Update();
            
            vtkNew<vtkActor> actor;
            actor->SetMapper(glyph3D);
            actor->GetProperty()->SetPointSize(30);
            actor->GetProperty()->SetColor(namedColors->GetColor3d("Tomato").GetData());

            context.init({ actor });

            slider.init(context, [&points, &polyData, &glyph3D](vtkSliderWidget* widget, vtkSliderRepresentation2D* representation, unsigned long, void*) {
                //todo add slider functionality
                auto colors = loadColors(representation->GetValue(), points->GetNumberOfPoints());
                //auto colors = colorsFromPositions(*points);
                polyData->GetPointData()->SetScalars(colors);
                glyph3D->Update();
                });

            context.startRendering();
        }
    };

}//namepsace


int main() {
    set_current_directory();

    Visualisation visualisation;
    visualisation.run();
    
    return EXIT_SUCCESS;
}