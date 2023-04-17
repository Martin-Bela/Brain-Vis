// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef HISTOGRAM_WIDGET_H
#define HISTOGRAM_WIDGET_H

#include <QBrush>
#include <QPen>
#include <QPixmap>
#include <QWidget>

#include <vtkTable.h>
#include <vtkDenseArray.h>
#include <vtkNew.h>

//! [0]
class HistogramWidget : public QWidget
{
    Q_OBJECT

public:
    double max = 0; 


    explicit HistogramWidget(QWidget *parent = nullptr);

    void setTick(int tick);
    void setFirstVisibleTick(int tick) {
        firstVisibleTick = tick;
        update();
    }
    void setLastVisibleTick(int tick) {
        lastVisibleTick = tick;
        update();
    }

    vtkDenseArray<double> &getHistogramDataRef() {
        return *histogramData.Get();
    }
    
    void onLoadHistogram() {
        loaded = true;
    }
    bool isLoaded() {
        return loaded;
    }

protected:
    void paintEvent(QPaintEvent *event) override;

    void mousePressEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;

private:
    bool antialiased = true;
    bool loaded = false;

    vtkNew<vtkDenseArray<double>> histogramData;


    int tick = 1;
    int firstVisibleTick = 0;
    int lastVisibleTick = 500; // TODO Automatically set by Slider


    int getVisibleTicks() {
        return lastVisibleTick - firstVisibleTick;
    }

    int getTimesteps() {
        if (loaded){
            return histogramData->GetExtent(0).GetEnd();
        }
        return 0;
    }
    int getBinCount() {
        if (loaded) {
            return histogramData->GetExtent(1).GetEnd();
        }
        return 0;
    }

};
//! [0]

#endif // HISTOGRAM_WIDGET_H