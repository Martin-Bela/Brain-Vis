#include <vtkActor.h>
#include <vtkCamera.h>
#include <vtkNamedColors.h>
#include <vtkNew.h>
#include <vtkPolyDataMapper.h>
#include <vtkPointData.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkTable.h>

#include <vtkDelimitedTextReader.h>
#include <vtkVertexGlyphFilter.h>
#include <vtkGlyph3D.h>
#include <vtkSphereSource.h>
#include <vtkGlyph3DMapper.h>
#include <vtkSliderWidget.h>
#include <vtkWidgetEvent.h>
#include <vtkInteractorStyleTrackballCamera.h>

#include <array>
#include <filesystem>
#include <random>

#include "neuronProperties.hpp"

std::mt19937 rand_gen(time(0));

namespace { //anonymous namespace

    std::filesystem::path dataFolder = "./data/viz-calcium";

    vtkNew<vtkNamedColors> namedColors;

    vtkNew<vtkPoints> loadPositions() {
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
            positions->InsertNextPoint(
                (table->GetValue(i, 1)).ToDouble(),
                (table->GetValue(i, 2)).ToDouble(),
                (table->GetValue(i, 3)).ToDouble());
        }
        return positions;
    }

    // from https://stackoverflow.com/questions/2353211/hsl-to-rgb-color-conversion
    std::array<float,3> hslToRgb(std::array<float, 3> hsl) {

        auto hueToRgb = [](float p, float q, float t) {
            if (t < 0.f)
                t += 1.f;
            if (t > 1.f)
                t -= 1.f;
            if (t < 1.f / 6.f)
                return p + (q - p) * 6.f * t;
            if (t < 1.f / 2.f)
                return q;
            if (t < 2.f / 3.f)
                return p + (q - p) * (2.f / 3.f - t) * 6.f;
            return p;
        };

        auto [h, s, l] = hsl;
        float r, g, b;

        if (s == 0.f) {
            r = g = b = l; // achromatic
        }
        else {
            float q = l < 0.5f ? l * (1 + s) : l + s - l * s;
            float p = 2 * l - q;
            r = hueToRgb(p, q, h + 1.f / 3.f);
            g = hueToRgb(p, q, h);
            b = hueToRgb(p, q, h - 1.f / 3.f);
        }
        return { r, g, b };
    }

    std::array<unsigned char, 3> generateNiceColor() {
        std::uniform_real_distribution<float> dist(0, 1);
        std::array<float, 3> hsl = { dist(rand_gen), 1.0, dist(rand_gen) / 2.f + 0.25f };
        auto rgb = hslToRgb(hsl);
        return { static_cast<unsigned char>(rgb[0] * 255),
            static_cast<unsigned char>(rgb[1] * 255),
            static_cast<unsigned char>(rgb[2] * 255) };
    }

    vtkNew<vtkUnsignedCharArray> loadColors(int timestep, int pointCount) {
        auto path = (dataFolder / "monitors-bin/timestep").string() + std::to_string(timestep);
        std::ifstream in(path);

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

    void render(vtkActor* actor) {
        vtkNew<vtkRenderer> renderer;
        renderer->AddActor(actor);

        renderer->SetBackground(namedColors->GetColor3d("LightYellow").GetData());
        // Zoom in a little by accessing the camera and invoking its "Zoom" method.
        renderer->ResetCamera();
        renderer->GetActiveCamera()->Zoom(1.5);

        // The render window is the actual GUI window
        // that appears on the computer screen
        vtkNew<vtkRenderWindow> renderWindow;
        renderWindow->SetSize(800, 800);
        renderWindow->AddRenderer(renderer);
        renderWindow->SetWindowName("Brain Visualisation");

        // The render window interactor captures mouse events
        // and will perform appropriate camera or actor manipulation
        // depending on the nature of the events.
        vtkNew<vtkRenderWindowInteractor> renderWindowInteractor;
        vtkNew<vtkInteractorStyleTrackballCamera> trackballCamera;
        renderWindowInteractor->SetInteractorStyle(trackballCamera);
        renderWindowInteractor->SetRenderWindow(renderWindow);

        // This starts the event loop and as a side effect causes an initial render.
        renderWindow->Render();
        renderWindowInteractor->Start();
    }

}//namepsace

int main() {
    if (std::filesystem::current_path().filename().string().starts_with("build")) {
        // cd ..
        auto newPath = std::filesystem::current_path().parent_path();
        std::filesystem::current_path(newPath);
    }
    std::cout << "Current working directory: " << std::filesystem::current_path() << std::endl;

    vtkNew<vtkSphereSource> sphere;
    sphere->SetPhiResolution(10);
    sphere->SetThetaResolution(10);
    sphere->SetRadius(.08);

    auto points = loadPositions();

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

    render(actor);

    return EXIT_SUCCESS;
}