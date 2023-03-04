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

#include <array>
#include <filesystem>

#include <vtkDelimitedTextReader.h>
#include <vtkVertexGlyphFilter.h>
#include <vtkGlyph3D.h>
#include <vtkSphereSource.h>
#include <vtkGlyph3DMapper.h>
#include <vtkSliderWidget.h>
#include <vtkWidgetEvent.h>

namespace { //anonymous namespace

    std::filesystem::path dataFolder = "../data/viz-calcium";

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

    vtkNew<vtkUnsignedCharArray> loadColors(uint64_t timestep, uint64_t pointCount) {
        /*vtkNew<vtkDelimitedTextReader> reader;
        auto path = (dataFolder / "monitors/0_").string();
        path += std::to_string(timestep) + ".csv";
        
        reader->SetFileName(path.data());
        reader->DetectNumericColumnsOn();
        reader->SetFieldDelimiterCharacters(" ");
        reader->Update();
        
        vtkTable* table = reader->GetOutput();
        */

        vtkNew<vtkUnsignedCharArray> colors;
        colors->SetName("colors");
        colors->SetNumberOfComponents(3);

        
        std::array<unsigned char, 3> color = { 255, 0, 0 };
        std::array<unsigned char, 3> color1 = { 0, 255, 0 };
        std::array<unsigned char, 3> color2 = { 0, 0, 255 };
        std::array<unsigned char, 3> color3 = { 255, 255, 0 };
        std::array<unsigned char, 3> color4 = { 255, 0, 255 };

        for (vtkIdType i = 0; i < pointCount / 5; i++)
        {
            colors->InsertNextTypedTuple(color.data());
            colors->InsertNextTypedTuple(color1.data());
            colors->InsertNextTypedTuple(color2.data());
            colors->InsertNextTypedTuple(color3.data());
            colors->InsertNextTypedTuple(color4.data());
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
        renderWindowInteractor->SetRenderWindow(renderWindow);

        // This starts the event loop and as a side effect causes an initial render.
        renderWindow->Render();
        renderWindowInteractor->Start();
    }

}//namepsace

int main() {
    vtkNew<vtkSphereSource> sphere;
    sphere->SetPhiResolution(10);
    sphere->SetThetaResolution(10);
    sphere->SetRadius(.08);

    auto points = loadPositions();

    vtkNew<vtkPolyData> polyData;
    polyData->SetPoints(points);

    auto colors = loadColors(0, points->GetNumberOfPoints());
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