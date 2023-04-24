// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef HISTOGRAM_WIDGET_H
#define HISTOGRAM_WIDGET_H

#include <QBrush>
#include <QPen>
#include <QPixmap>
#include <QWidget>
#include <QObject>


#include <vtkTable.h>
#include <vtkDenseArray.h>
#include <vtkNew.h>
#include <vtkSmartPointer.h>



enum HistogramDrawMode {
    Histogram = 0,
    Summary = 1,
    Both = 2,
};

//! [0]
class HistogramWidget : public QWidget
{
    Q_OBJECT


public:  

    bool logarithmicScaleEnabled = false;

    explicit HistogramWidget(QWidget *parent = nullptr);

    void setTick(int tick);

    void setVisibleRange(int firstTick, int lastTick) {
        firstVisibleTick = firstTick;
        lastVisibleTick = lastTick;
    }

    vtkTable &getHistogramDataRef() {
        return *histogramData.Get();
    }

    vtkTable &getSummaryDataRef() {
        return *summaryData.Get();
    }

    void setData(vtkSmartPointer<vtkTable> histogramData, vtkSmartPointer<vtkTable> summaryData) {
        this->histogramData = std::move(histogramData);
        this->summaryData = std::move(summaryData);

        loaded = true;
        recomputeMinMax();
        recomputeSummaryMinMax();
    }
    
    bool isLoaded() {
        return loaded;
    }

    void changeDrawMode(HistogramDrawMode drawModeParam) {
        std::cout << "DrawMode: " << drawMode <<" \n";
        drawMode = drawModeParam;
    }
   

signals:
    void histogramCursorMoved(int newValue);


protected:
    void paintEvent(QPaintEvent *event) override;

    void mousePressEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;

private:
    bool antialiased = true;
    bool loaded = false;

    double max = 0; 
    double min = 0;
    double sMax = 0;
    double sMin = 0;

    vtkSmartPointer<vtkTable> histogramData;
    vtkSmartPointer<vtkTable> summaryData;
    HistogramDrawMode drawMode = Histogram;

    int tick = 1;
    int firstVisibleTick = 0;
    int lastVisibleTick = 500; // TODO Automatically set by Slider

    void paintHistogram(QPainter& painter);
    void paintSummary(QPainter& painter, bool redraw);
    void paintMinMaxLabels(QPainter &painter, QColor color);

    void recomputeSummaryMinMax();
    void recomputeMinMax();

    int getYPos(double value);

    int getVisibleTicks() {
        return lastVisibleTick - firstVisibleTick;
    }

    int getTimesteps() {
        if (loaded){
            return histogramData->GetNumberOfRows();
        }
        return 0;
    }
    int getBinCount() {
        if (loaded) {
            return histogramData->GetNumberOfColumns();
        }
        return 0;
    }

};
//! [0]

#endif // HISTOGRAM_WIDGET_H