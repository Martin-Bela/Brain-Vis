#pragma once

#include <vtkSliderRepresentation2D.h>
#include <vtkSliderWidget.h>
#include <vtkCommand.h>
#include <vtkProperty.h>
#include <vtkProperty2D.h>
#include <vtkTextProperty.h>


#include "context.hpp"

#include <functional>

using SliderCallback = std::function<void(vtkSliderWidget*, vtkSliderRepresentation2D*, unsigned long, void*)>;

class SliderCallbackWrapper : public vtkCommand {
    SliderCallback callback;

public:
    static SliderCallbackWrapper* New() {
        return new SliderCallbackWrapper{};
    }

    void setFunction(SliderCallback callback) {

        this->callback = std::move(callback);
    }

    void Execute(vtkObject* caller, unsigned long eventId, void* callData) override {
        auto* sliderWidget = static_cast<vtkSliderWidget*>(caller);
        auto* representation = static_cast<vtkSliderRepresentation2D*>(sliderWidget->GetRepresentation());
        auto value = std::round(representation->GetValue());
        representation->SetValue(value);
        callback(sliderWidget, representation, eventId, callData);
    }
};


class Slider {
    vtkNew<vtkSliderRepresentation2D> sliderRep;
    vtkNew<vtkSliderWidget> sliderWidget;
    vtkNew<SliderCallbackWrapper> callbackWrapper;

public:
    void init(Context& context, SliderCallback callback) {
        sliderRep->SetMinimumValue(0);
        sliderRep->SetMaximumValue(999);
        sliderRep->SetValue(0);
        sliderRep->SetTitleText("Timestep");

        // Set color properties:
        // Change the color of the knob that slides
        sliderRep->GetSliderProperty()->SetColor(namedColors->GetColor3d("Red").GetData());

        // Change the color of the text indicating what the slider controls
        sliderRep->GetTitleProperty()->SetColor(namedColors->GetColor3d("Red").GetData());

        // Change the color of the text displaying the value
        sliderRep->GetLabelProperty()->SetColor(namedColors->GetColor3d("Red").GetData());

        // Change the color of the knob when the mouse is held on it
        sliderRep->GetSelectedProperty()->SetColor(
            namedColors->GetColor3d("Lime").GetData());

        // Change the color of the bar
        sliderRep->GetTubeProperty()->SetColor(
            namedColors->GetColor3d("Black").GetData());

        // Change the color of the ends of the bar
        sliderRep->GetCapProperty()->SetColor(namedColors->GetColor3d("Black").GetData());

        sliderRep->GetPoint1Coordinate()->SetCoordinateSystemToDisplay();
        sliderRep->GetPoint1Coordinate()->SetValue(40, 80);
        sliderRep->GetPoint2Coordinate()->SetCoordinateSystemToDisplay();
        sliderRep->GetPoint2Coordinate()->SetValue(200, 80);

        sliderWidget->SetInteractor(context.renderWindowInteractor);
        sliderWidget->SetRepresentation(sliderRep);
        sliderWidget->SetAnimationModeToAnimate();
        sliderWidget->EnabledOn();

        //vtkCommand::InteractionEvent
        sliderWidget->AddObserver(vtkCommand::EndInteractionEvent, callbackWrapper);
        callback(sliderWidget, sliderRep, 0, nullptr);
        callbackWrapper->setFunction(std::move(callback));
    }
};