#pragma once

#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkRenderer.h>    

#include <vtkCamera.h>

#include "visUtility.hpp"

class Context{
public:
    vtkNew<vtkRenderer> renderer;
    vtkNew<vtkGenericOpenGLRenderWindow> renderWindow;

    void init(std::vector<vtkActor*> actors) {
        for (auto& actor : actors) {
            renderer->AddActor(actor);
        }

        // This starts the event loop and as a side effect causes an initial render.
        renderer->SetBackground(namedColors->GetColor3d("White").GetData());
        // Create camera, center it at center of points and setup to be similar to what is used in school
        vtkCamera* camera = renderer->GetActiveCamera();
        camera->Elevation(-90);
        camera->SetViewUp(0, 0, 1);
        renderer->ResetCamera();
        std::cout << camera->GetNearPlaneScale();

        // The render window is the actual GUI window
        // that appears on the computer screen
        renderWindow->AddRenderer(renderer);
            
    }

    void render() {
        renderWindow->Render();
    }
};