#include <QApplication>
#include <QMainWindow>
#include <QVTKOpenGLNativeWidget.h>

#include "visUtility.hpp"
#include "visualisation.hpp"
#include "popupWidget.hpp"

#include "ui_mainWindow.h"


namespace { //anonymous namespace

    class MainWindow : public QMainWindow {
        std::vector<PopUpWidget*> popUpWidgets;
    public:
        void addPopUp(PopUpWidget* widget) {
            popUpWidgets.push_back(widget);
        }

        void resizeEvent(QResizeEvent* event) override {
            QWidget::resizeEvent(event);
            
            for (auto* popUp : popUpWidgets) {
                popUp->adjustPosition();
            }
        }

        void moveEvent(QMoveEvent *event) override {
            QWidget::moveEvent(event);
            
            for (auto* popUp : popUpWidgets) {
                popUp->adjustPosition();
            }
        }
    };

    class Application {
    public:
        Application(int argc, char** argv) {

            QSurfaceFormat::setDefaultFormat(QVTKOpenGLNativeWidget::defaultFormat());
            application.init(argc, argv);
            std::cout << QApplication::applicationDirPath().toStdString() << std::endl;

            mainWindow.init();
            mainUI.init();
            mainUI->setupUi(mainWindow.ptr());

            visualisation.init(Widgets{ mainUI->bottomPanel, mainUI->histogramSlider, mainUI->histogramSliderLabel,
                mainUI->rangeSlider,  mainUI->minValLabel, mainUI->maxValLabel, 
                mainUI->neuronGlobalPropertiesLabel, mainUI->neuronCurrentTimestepPropertiesLabel });
            visualisation->loadData();

            visualisationWidget.init();
            visualisationWidget->setRenderWindow(visualisation->context.renderWindow);
            mainUI->mainVisDock->addWidget(visualisationWidget.ptr());
            
            mainUI->bottomPanel->setFocusPolicy(Qt::ClickFocus);
            mainUI->bottomPanel->setAttribute(Qt::WA_OpaquePaintEvent);
            mainUI->histogramSlider->setFocusPolicy(Qt::ClickFocus);
            mainUI->histogramSlider->setAttribute(Qt::WA_OpaquePaintEvent);

            auto* popup = new PropertiesPopUp(mainWindow.ptr(), mainUI->attributesHelp);
            mainWindow->addPopUp(popup);

            auto attributeNames = std::to_array<const char*>({ "fired", "fired fraction", "activity", "dampening", "current calcium",
                "target calcium", "synaptic input", "background input", "grown axons", "connected axons", "grown dendrites", "connected dendrites" });
            for (auto name : attributeNames) {
                mainUI->comboBox->addItem(name);
            }

            // Remove title bars
            mainUI->leftDockWidget->setTitleBarWidget(new QWidget());
            mainUI->bottomDockWidget->setTitleBarWidget(new QWidget());
            
            mainUI->bottomDockWidget->setFixedHeight(200);

            QObject::connect(mainUI->comboBox, &QComboBox::currentIndexChanged, visualisation.ptr(), &Visualisation::changeColorAttribute);
            QObject::connect(mainUI->comboBox_2, &QComboBox::currentIndexChanged, visualisation.ptr(), &Visualisation::changeDrawMode);
            QObject::connect(mainUI->showEdgesCheckBox, &QCheckBox::stateChanged, visualisation.ptr(), &Visualisation::showEdges);
            QObject::connect(mainUI->bottomPanel, &HistogramWidget::histogramCursorMoved, visualisation.ptr(), &Visualisation::changeTimestep);
            QObject::connect(mainUI->histogramSlider, &HistogramSliderWidget::histogramCursorMoved, visualisation.ptr(), &Visualisation::changeTimestepRange);
            QObject::connect(mainUI->logScaleCheckbox, &QCheckBox::stateChanged, visualisation.ptr(), &Visualisation::logCheckboxChange);
            QObject::connect(mainUI->rangeSlider, &RangeSliderWidget::valueChange, visualisation.ptr(), &Visualisation::setPointFilter);
            QObject::connect(mainUI->pointSizeSlider, &QSlider::valueChanged, visualisation.ptr(), &Visualisation::changePointSize);
            QObject::connect(mainUI->scatterPointsCheckBox, &QCheckBox::stateChanged, visualisation.ptr(), &Visualisation::setPointScattering);
            QObject::connect(mainUI->showDerivativesCheckBox, &QCheckBox::stateChanged, visualisation.ptr(), &Visualisation::showDerivatives);
        }

        int run() {
            mainWindow->show();

            visualisation->firstRender();
            return application->exec();
        }

        DeferredInit<QApplication> application;
        DeferredInit<MainWindow> mainWindow;
        DeferredInit<Ui::MainWindow> mainUI;

        DeferredInit<Visualisation> visualisation;
        DeferredInit<QVTKOpenGLNativeWidget> visualisationWidget;
    };

}//namespace


int main(int argc, char** argv) {
    setCurrentDirectory();
    Application app(argc, argv);
    return app.run();
}