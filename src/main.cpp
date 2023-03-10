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


#include "context.hpp"
#include "utility.hpp"
#include "neuronProperties.hpp"
#include "slider.hpp"

namespace { //anonymous namespace

    vtkNew<vtkPoints> loadPositions(vtkNew<vtkMutableDirectedGraph> *g) {
        vtkNew<vtkDelimitedTextReader> reader;
        auto path = (dataFolder / "positions/rank_0_positions.txt").string();
        reader->SetFileName(path.data());
        reader->DetectNumericColumnsOn();
        reader->SetFieldDelimiterCharacters(" ");
        reader->Update();

        vtkNew<vtkPoints> positions;
        vtkTable* table = reader->GetOutput();
        for (vtkIdType i = 0; i < table->GetNumberOfRows(); i++)
        {
            if (table->GetValue(i, 0).ToString() == "#") continue;

            positions->InsertNextPoint(
                (table->GetValue(i, 1)).ToDouble(),
                (table->GetValue(i, 2)).ToDouble(),
                (table->GetValue(i, 3)).ToDouble());
            g->GetPointer()->AddVertex();
        }
        return positions;
    }


    void loadEdges(vtkNew<vtkMutableDirectedGraph>* g) {
        vtkNew<vtkDelimitedTextReader> reader;
        auto path = (dataFolder / "network/rank_0_step_0_in_network.txt").string();
        reader->SetFileName(path.data());
        reader->DetectNumericColumnsOn();
        reader->SetFieldDelimiterCharacters(" \t");
        reader->Update();


        vtkTable* table = reader->GetOutput();
        for (vtkIdType i = 0; i < table->GetNumberOfRows(); i++)
        {
            if (table->GetValue(i, 0).ToString() == "#") continue;
            g->GetPointer()->AddEdge(static_cast<vtkIdType>(table->GetValue(i, 3).ToInt()) - 1, static_cast<vtkIdType>(table->GetValue(i, 1).ToInt()) - 1);
        }
    }

    vtkNew<vtkUnsignedCharArray> loadColors(int timestep, int pointCount) {
        auto path = (dataFolder / "monitors-bin/timestep").string() + std::to_string(timestep);
        std::ifstream in(path, std::ios::binary);
        if (!in.good()) {
            std::cout << "Timestep file couldn't be opened!\n" << std::endl;
        }

        vtkNew<vtkUnsignedCharArray> colors;
        colors->SetName("colors");
        colors->SetNumberOfComponents(3);

        for (int i = 0; i < pointCount; i++) {
            NeuronProperties neuron{};
            in.read(reinterpret_cast<char*>(&neuron), sizeof(NeuronProperties));

            std::array<unsigned char, 3> color = { 255, 0, 0};
            std::array<unsigned char, 3> color2 = { 0, 255, 0 };

            colors->InsertNextTypedTuple(neuron.electricActivity > -62 ? color.data() : color2.data());
        }

        if (!in.good()) {
            std::cout << "Error reading colors!\n" << std::endl;
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
            sphere->SetRadius(0.5);

            vtkNew<vtkMutableDirectedGraph> g;
            auto points = loadPositions(&g);
            // Add the coordinates of the points to the graph
            g->SetPoints(points);
            loadEdges(&g);

            // Convert the graph to a polydata
            vtkNew<vtkGraphToPolyData> graphToPolyData;
            graphToPolyData->SetInputData(g);
            graphToPolyData->Update();

            // Create a mapper and actor
            vtkNew<vtkPolyDataMapper> mapper;
            mapper->SetInputConnection(graphToPolyData->GetOutputPort());

            vtkNew<vtkActor> graphActor;
            graphActor->SetMapper(mapper);


            vtkNew<vtkPolyData> polyData;
            polyData->SetPoints(points);

            //auto colors = loadColors(80, points->GetNumberOfPoints());
            auto colors = colorsFromPositions(*points);
            polyData->GetPointData()->SetScalars(colors);

            vtkNew<vtkGlyph3DMapper> glyph3D;
            glyph3D->SetInputData(polyData);
            glyph3D->SetSourceConnection(sphere->GetOutputPort());
            glyph3D->Update();

            vtkNew<vtkActor> actor;
            actor->SetMapper(glyph3D);
            actor->GetProperty()->SetPointSize(30);
            actor->GetProperty()->SetColor(namedColors->GetColor3d("Tomato").GetData());

            context.init({ actor, graphActor });

            slider.init(context, [](vtkSliderWidget* widget, vtkSliderRepresentation2D* representation, unsigned long, void*) {
                //todo add slider functionality
                std::cout << representation->GetValue() << std::endl;
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