#include <vtkActor.h>
#include <vtkDenseArray.h>
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
#include <vtkPassThroughEdgeStrategy.h>
#include <vtkPassThroughLayoutStrategy.h>
#include <vtkEdgeLayout.h>
#include <vtkSmartPointer.h>
#include <vtkVariantArray.h>
#include <vtkPointGaussianMapper.h>

#include <QVTKOpenGLNativeWidget.h>

#include <QApplication>
#include <QDockWidget>
#include <QGridLayout>
#include <QLabel>
#include <QMainWindow>
#include <QPointer>
#include <QPushButton>
#include <QSlider>
#include <QComboBox>
#include <QString>
#include <QVBoxLayout>
#include <qstring.h>

#include "context.hpp"
#include "visUtility.hpp"
#include "neuronProperties.hpp"
#include "edge.hpp"
#include "binaryReader.hpp"
#include "histogramWidget.hpp"
#include "loaders.hpp"

#include "ui_mainWindow.h"

#include <unordered_map>
#include <optional>
#include <limits>
#include <ranges>


namespace { //anonymous namespace

    std::string readWholeFile(std::ifstream& file) {
        checkFile(file);
        return std::string(std::istreambuf_iterator<char>{file}, {});
    }

    struct Widgets {
        HistogramWidget* histogram = nullptr;
        HistogramSliderWidget* sliderWidget = nullptr;
        QRangeSlider* rangeSlider = nullptr;
        QLabel* minimumValLabel = nullptr;
        QLabel* maximumValLabel = nullptr;
        QLabel* timestepLabel = nullptr;
    };
 

    class Visualisation : public QObject {
    public:
        Context context;
        vtkNew<vtkSphereSource> sphere;
        vtkNew<vtkPoints> points;
        vtkNew<vtkPoints> aggregatedPoints;
        vtkNew<vtkPolyData> polyData;
        vtkNew<vtkPointGaussianMapper> pointGaussianMapper;

        vtkNew<vtkActor> actor;
        vtkNew<vtkActor> arrowActor;

        vtkNew<vtkGraphToPolyData> graphToPolyData;
        vtkNew<vtkArrowSource> arrowSource;
        vtkNew<vtkGlyph3D> arrowGlyph;
        vtkNew<vtkPolyDataMapper> arrowMapper;


        std::vector<uint16_t> point_map;

        Range pointFilter = Range::Whole();

        int currentTimestep = 0;
        int currentColorAttribute = 0;
        bool edgesVisible = false;

        HistogramDataLoader histogramDataLoader;

        enum: int { edgesHidden = -1 };
        int edgeTimestep = edgesHidden;

        Widgets widgets;

        Visualisation(Widgets widgets) :
            widgets(widgets) { }

        void loadData() {
            sphere->SetPhiResolution(10);
            sphere->SetThetaResolution(10);
            sphere->SetRadius(0.08);

            loadPositions(*points, *aggregatedPoints, point_map);

            // Convert the graph to a polydata
            graphToPolyData->EdgeGlyphOutputOn();
            graphToPolyData->SetEdgeGlyphPosition(0.0);

            // Make a simple edge arrow for glyphing.
            arrowSource->SetShaftRadius(0.01);
            arrowSource->SetTipRadius(0.02);
            arrowSource->Update();

            // Use Glyph3D to repeat the glyph on all edges.
            arrowGlyph->SetInputConnection(0, graphToPolyData->GetOutputPort(1));
            arrowGlyph->SetSourceConnection(arrowSource->GetOutputPort());
            arrowGlyph->SetScaleModeToScaleByVector();
            
            arrowActor->SetMapper(arrowMapper);
            arrowActor->GetProperty()->SetOpacity(0.75);
            arrowActor->GetProperty()->SetColor(namedColors->GetColor3d("DarkGray").GetData());

            // Points
            polyData->SetPoints(points);

            //double* range = polyData->GetPointData()->GetScalars()->GetRange();

            pointGaussianMapper->SetInputData(polyData);
            //pointGaussianMapper->SetScalarRange(range);
            pointGaussianMapper->SetScaleFactor(0.9);
            pointGaussianMapper->EmissiveOff();

            if (false) {
                std::ifstream shaderFile("src/vis/shaders/bilboard.frag");
                cout << "Loading fog shader!\n";

                std::string shaderCode = readWholeFile(shaderFile);
                pointGaussianMapper->SetSplatShaderCode(shaderCode.c_str());
            } else {
                pointGaussianMapper->SetSplatShaderCode(
                    #include "vis/shaders/_bilboard_fallback.frag"
                );
	        };
            
            actor->SetMapper(pointGaussianMapper);
            //actor->GetProperty()->SetOpacity(0.5);
            actor->GetProperty()->SetPointSize(30);
            actor->GetProperty()->SetColor(namedColors->GetColor3d("Tomato").GetData());

            context.init({ actor, arrowActor });
        }

        void firstRender() {
            // loadTableData();
            loadHistogramData(0);
            reloadColors(0, 0);
            reloadEdges();
            context.render();
        }

    private:

        void reloadColors(int timestep, int colorAttribute) {
            currentColorAttribute = colorAttribute;
            currentTimestep = timestep;

            widgets.timestepLabel->setText(QString::fromStdString(std::to_string(timestep * 100)));
            std::cout << std::format("Reloading colors - timestep: {}, attribute: {}\n", timestep * 100, colorAttribute);

            // todo: change to global data!!!
            auto summaryData = histogramDataLoader.getSummaryData(currentColorAttribute);
            double propMin = summaryData[timestep][3];
            double propMax = summaryData[timestep][2];

            widgets.minimumValLabel->setText(QString::fromStdString(std::format("{:.2}", std::lerp(propMin, propMax, pointFilter.lower_bound))));
            widgets.maximumValLabel->setText(QString::fromStdString(std::format("{:.2}", std::lerp(propMin, propMax, pointFilter.upper_bound))));

            auto colors = loadColors(timestep, colorAttribute, propMin, propMax, pointFilter);

            //auto colors = colorsFromPositions(*points);
            polyData->GetPointData()->SetScalars(colors);
        }

        void reloadEdges() {
            int newEdgeTimestep = edgesVisible ? currentTimestep / 100 * 100 : edgesHidden;
            if (edgeTimestep == newEdgeTimestep) {
                return;
            }
            edgeTimestep = newEdgeTimestep;
            std::cout << std::format("Edges reloaded with timestep {}\n", newEdgeTimestep);

            vtkNew<vtkMutableDirectedGraph> g;
            g->SetNumberOfVertices(aggregatedPoints->GetNumberOfPoints());
            g->SetPoints(aggregatedPoints);
            
            if (edgesVisible) {
                loadEdges(*g, point_map, currentTimestep);
            }

            vtkNew<vtkGraphLayout> layout;
            vtkNew<vtkPassThroughLayoutStrategy> strategy;
            layout->SetInputData(g);
            layout->SetLayoutStrategy(strategy);

            vtkNew<vtkPassThroughEdgeStrategy> edgeStrategy;
            vtkNew<vtkEdgeLayout> edgeLayout;
            edgeLayout->SetLayoutStrategy(edgeStrategy);
            edgeLayout->SetInputConnection(layout->GetOutputPort());

            graphToPolyData->SetInputConnection(edgeLayout->GetOutputPort());
            graphToPolyData->Update();

            arrowMapper->SetInputConnection(arrowGlyph->GetOutputPort());
            arrowMapper->Update();            
        }

        void loadHistogramData(int colorAttribute) {
            auto t1 = std::chrono::high_resolution_clock::now();

            auto histData = histogramDataLoader.getHistogramData(colorAttribute);
            auto summData = histogramDataLoader.getSummaryData(colorAttribute);

            widgets.histogram->setTableData(histData, summData);
            widgets.sliderWidget->setTableData(histData, summData);

            auto t2 = std::chrono::high_resolution_clock::now();
            std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1) << "\n";
            std::cout << "Histogram data for " << attributeToString(colorAttribute) << " loaded.\n";

            widgets.rangeSlider->setLowValue(0);
            widgets.rangeSlider->setHighValue(100);
        }

        void reloadHistogram(int timestep, int colorAttribute) {
            if (!widgets.histogram->isLoaded()) {
                std::cout << "histogramWidget in Visualization is not loaded!\n";
                return;
            }
            widgets.histogram->setTick(timestep);
            widgets.histogram->update();
        }

    public slots:
        void setPointFilter(unsigned low, unsigned hight) {
            pointFilter = Range{ low / 100.0, hight / 100.0 };
            reloadColors(currentTimestep, currentColorAttribute);
            context.render();
        }

        void changeTimestep(int timestep) {
            reloadColors(timestep, currentColorAttribute);
            reloadHistogram(currentTimestep, currentColorAttribute);
            reloadEdges();
            context.render();
        }

        void changePointSize(int size) {
            pointGaussianMapper->SetScaleFactor(size / 100.0);
            //sphere->SetRadius(size / 100.0);
            context.render();
        }

        // Used for changing the sliding window of the histogramWidget
        void changeTimestepRange(int sliderValue) {
            const int minVal = 0;
            const int maxVal = 9999;

            int lowerBoundary = std::max(sliderValue - 250, minVal);
            int upperBoundary = std::min(sliderValue + 250, maxVal);

            if (lowerBoundary == minVal) upperBoundary = 500;
            if (upperBoundary == maxVal) lowerBoundary = maxVal - 500;
            widgets.histogram->setVisibleRange(lowerBoundary, upperBoundary);

            std::cout << "Slider value:" << std::to_string(sliderValue) << std::endl;
            changeTimestep(sliderValue);
        }

        void changeDrawMode(int modeType) {
            widgets.histogram->changeDrawMode((HistogramDrawMode)modeType);
            widgets.sliderWidget->changeDrawMode((HistogramDrawMode)modeType);
            reloadHistogram(currentTimestep, currentColorAttribute);
            widgets.sliderWidget->update();
        }

        void changeColorAttribute(int colorAttribute) {
            pointFilter = Range::Whole();
            loadHistogramData(colorAttribute);
            reloadColors(currentTimestep, colorAttribute);
            reloadHistogram(currentTimestep, colorAttribute);
            context.render();
            widgets.sliderWidget->update();
        }

        void showEdges(int state) {
            if (state == Qt::Checked) {
                edgesVisible = true;
            }
            else {
                edgesVisible = false;
            }
            reloadEdges();
            context.render();
        }

        void logCheckboxChange(int state) {
            bool logEnabled = state == Qt::Checked;

            widgets.histogram->logarithmicScaleEnabled = logEnabled;
            widgets.sliderWidget->logarithmicScaleEnabled = logEnabled;
            widgets.sliderWidget->update();
            widgets.histogram->update();
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

            visualisation.init(Widgets{ mainUI->bottomPanel, mainUI->sliderWidget, mainUI->rangeSlider, 
                mainUI->minValLabel, mainUI->maxValLabel, mainUI->timestepLabel });
            visualisation->loadData();

            visualisationWidget.init();
            visualisationWidget->setRenderWindow(visualisation->context.renderWindow);
            mainUI->mainVisDock->addWidget(visualisationWidget.ptr());
            
            mainUI->bottomPanel->setFocusPolicy(Qt::ClickFocus);
            mainUI->bottomPanel->setAttribute(Qt::WA_OpaquePaintEvent);
            mainUI->sliderWidget->setFocusPolicy(Qt::ClickFocus);
            mainUI->sliderWidget->setAttribute(Qt::WA_OpaquePaintEvent);

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
            QObject::connect(mainUI->sliderWidget, &HistogramSliderWidget::histogramCursorMoved, visualisation.ptr(), &Visualisation::changeTimestepRange);
            QObject::connect(mainUI->logScaleCheckbox, &QCheckBox::stateChanged, visualisation.ptr(), &Visualisation::logCheckboxChange);
            QObject::connect(mainUI->rangeSlider, &QRangeSlider::valueChange, visualisation.ptr(), &Visualisation::setPointFilter);
            QObject::connect(mainUI->pointSizeSlider, &QSlider::valueChanged, visualisation.ptr(), &Visualisation::changePointSize);
        }

        int run() {
            mainWindow->show();

            visualisation->firstRender();
            return application->exec();
        }

        DeferredInit<QApplication> application;
        DeferredInit<QMainWindow> mainWindow;
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