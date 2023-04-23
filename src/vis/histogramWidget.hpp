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

//! [0]
class HistogramWidget : public QWidget
{
    Q_OBJECT


public:  

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
    
    void onLoadHistogram() {
        loaded = true;
        recomputeMinMax();
    }
    bool isLoaded() {
        return loaded;
    }
   

signals:
    void histogramClicked(int newValue);


protected:
    void paintEvent(QPaintEvent *event) override;

    void mousePressEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;

private:
    bool antialiased = true;
    bool loaded = false;

    double max = 0; 
    double min = 0;

    vtkNew<vtkTable> histogramData;
    vtkNew<vtkTable> summaryData;


    int tick = 1;
    int firstVisibleTick = 0;
    int lastVisibleTick = 500; // TODO Automatically set by Slider


    void recomputeMinMax();

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