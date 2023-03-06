#pragma once

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

    void init(std::vector<vtkActor*> actors) {
        for (auto& actor : actors) {
            renderer->AddActor(actor);
        }

        // This starts the event loop and as a side effect causes an initial render.
        renderer->SetBackground(namedColors->GetColor3d("LightYellow").GetData());
        // Zoom in a little by accessing the camera and invoking its "Zoom" method.
        renderer->ResetCamera();
        renderer->GetActiveCamera()->Zoom(1.5);

        renderWindow->AddRenderer(renderer);
        // The render window is the actual GUI window
        // that appears on the computer screen
        renderWindow->SetSize(800, 800);
        renderWindow->SetWindowName("Brain Visualisation");

        // The render window interactor captures mouse events
        // and will perform appropriate camera or actor manipulation
        // depending on the nature of the events.

        renderWindowInteractor->SetInteractorStyle(trackballCamera);
        renderWindowInteractor->SetRenderWindow(renderWindow);
    }

    void startRendering() {
        renderWindow->Render();
        renderWindowInteractor->Start();
    }
};