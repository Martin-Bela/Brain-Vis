#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>    
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkCamera.h>

#include "utility.hpp"

class Context{
public:
    vtkNew<vtkRenderer> renderer;
    vtkNew<vtkRenderWindowInteractor> renderWindowInteractor;
    vtkNew<vtkInteractorStyleTrackballCamera> trackballCamera;
    vtkNew<vtkRenderWindow> renderWindow;

    Context(){

    }

    void addActor(vtkActor* actor) {
        renderer->AddActor(actor);
    }

    void startRendering() {
        renderer->SetBackground(namedColors->GetColor3d("LightYellow").GetData());
        // Zoom in a little by accessing the camera and invoking its "Zoom" method.
        renderer->ResetCamera();
        renderer->GetActiveCamera()->Zoom(1.5);

        // The render window is the actual GUI window
        // that appears on the computer screen
        renderWindow->SetSize(800, 800);
        renderWindow->AddRenderer(renderer);
        renderWindow->SetWindowName("Brain Visualisation");

        // The render window interactor captures mouse events
        // and will perform appropriate camera or actor manipulation
        // depending on the nature of the events.

        renderWindowInteractor->SetInteractorStyle(trackballCamera);
        renderWindowInteractor->SetRenderWindow(renderWindow);

        // This starts the event loop and as a side effect causes an initial render.
        renderWindow->Render();
        renderWindowInteractor->Start();
    }
};