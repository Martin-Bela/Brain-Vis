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
#include "utility.hpp"
#include "neuronProperties.hpp"
#include "slider.hpp"

namespace { //anonymous namespace

    void loadPositions(vtkMutableDirectedGraph& g, vtkPoints& positions) {
        vtkNew<vtkDelimitedTextReader> reader;
        auto path = (dataFolder / "positions/rank_0_positions.txt").string();
        reader->SetFileName(path.data());
        reader->DetectNumericColumnsOn();
        reader->SetFieldDelimiterCharacters(" ");
        reader->Update();

        vtkTable* table = reader->GetOutput();
        //the first row is header
        for (vtkIdType i = 1; i < table->GetNumberOfRows(); i++)
        {
            if (table->GetValue(i, 0).ToString() == "#") continue;

            positions.InsertNextPoint(
                (table->GetValue(i, 1)).ToDouble(),
                (table->GetValue(i, 2)).ToDouble(),
                (table->GetValue(i, 3)).ToDouble());
            g.AddVertex();
        }
    }


    void loadEdges(vtkMutableDirectedGraph& g) {
        vtkNew<vtkDelimitedTextReader> reader;
        auto path = (dataFolder / "network/rank_0_step_0_in_network.txt").string();
        reader->SetFileName(path.data());
        reader->DetectNumericColumnsOn();
        reader->SetFieldDelimiterCharacters(" \t");
        reader->Update();


        vtkTable* table = reader->GetOutput();
        for (vtkIdType i = 0; i < 1000; i++)
        {
            if (table->GetValue(i, 0).ToString() == "#") continue;
            g.AddEdge(static_cast<vtkIdType>(table->GetValue(i, 3).ToInt()) - 1, static_cast<vtkIdType>(table->GetValue(i, 1).ToInt()) - 1);
        }
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

            vtkNew<vtkMutableDirectedGraph> g;
            vtkNew<vtkPoints> points;
            loadPositions(*g, *points);
            // Add the coordinates of the points to the graph
#if 0
            g->SetPoints(points);
            loadEdges(*g);

            vtkNew<vtkBoostDividedEdgeBundling> edgeBundler;
            edgeBundler->SetInputDataObject(g);

            // Convert the graph to a polydata
            vtkNew<vtkGraphToPolyData> graphToPolyData;
            graphToPolyData->SetInputConnection(edgeBundler->GetOutputPort());
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
    std::filesystem::path path = std::filesystem::current_path();
    while (!path.filename().string().starts_with("brain-visualisation")) {
        auto parent = path.parent_path();
        if (parent == path) {
            std::cout << "brain-visualisation directory wasn't found!" << std::endl;
            return EXIT_FAILURE;
        }
        path = std::move(parent);
    }
    std::filesystem::current_path(path);
    std::cout << "Current working directory: " << std::filesystem::current_path() << std::endl;

    Visualisation visualisation;
    visualisation.run();
    
    return EXIT_SUCCESS;
}