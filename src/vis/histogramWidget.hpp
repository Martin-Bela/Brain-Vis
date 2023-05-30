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

#include <span>



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

    void setTableData(std::span<std::vector<int>> histogramData, std::span<std::vector<double>> summaryData) {
        this->histogramTable = histogramData;
        this->summaryTable = summaryData;

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
    bool antialiased = true;
    bool fastRepaint = false;
    bool loaded = false;
    
    double max = 0; 
    double min = 0;
    double sMax = 0;
    double sMin = 0;
    
    std::span<std::vector<int>> histogramTable{};
    std::span<std::vector<double>> summaryTable{};
    HistogramDrawMode drawMode = Histogram;
    int tick = 1;
    std::vector<int> previousTicks;
    int firstVisibleTick = 0;
    int lastVisibleTick = 500;
    
    int getYPos(double value);

    void paintEvent(QPaintEvent *event) override;

    void paintHistogram(QPainter& painter);
    void paintHistogramTick(QPainter& painter, int tick, float binSize, float tickSize);
    void paintSummary(QPainter& painter, bool redraw);
    void paintSummaryTick(QPainter& painter, int tick, float tickSize, QPen& black, QPen& blue, QPen& red);
    void paintMinMaxLabels(QPainter &painter, QColor color);

    int getVisibleTicks() {
        return lastVisibleTick - firstVisibleTick;
    }

    size_t getTimesteps() {
        if (loaded){
            return histogramTable.size();
        }
        return 0;
    }
    size_t getBinCount() {
        if (loaded) {
            return histogramTable[0].size();
        }
        return 0;
    }

private:

    void mousePressEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void resizeEvent(QResizeEvent*) override;
    void keyPressEvent(QKeyEvent* event) override;
    void recomputeSummaryMinMax();
    void recomputeMinMax();

};
//! [0]

#endif // HISTOGRAM_WIDGET_H