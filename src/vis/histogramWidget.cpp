// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "histogramWidget.hpp"
#include <iostream>

#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>

#define SUMMARY_MEAN 0
#define SUMMARY_SUM 1
#define SUMMARY_MAX 2
#define SUMMARY_MIN 3

//! [0]
HistogramWidget::HistogramWidget(QWidget *parent)
    : QWidget(parent)
{
    antialiased = false;

    setBackgroundRole(QPalette::Base);
    setAutoFillBackground(true);
}

void HistogramWidget::paintEvent(QPaintEvent * /* event */)
{
    if(!isLoaded())
        return;

    QPainter painter(this);

    if (antialiased)
        painter.setRenderHint(QPainter::Antialiasing, true);

    int height = geometry().height();
    int width = geometry().width();
    float tickSize =  (float)width / (float) getVisibleTicks();

    // TODO Use PixMap!
    std::cout <<"Bins:" << getBinCount() << ", Steps: " << getVisibleTicks() << "\n";
    float binSize = (float)height / (float)getBinCount();
    std::cout <<"BinSize:" << binSize << ", TimestepSize: " << tickSize << "\n";
    for ( int x = firstVisibleTick; x <= lastVisibleTick; x++) {
        for ( int y = 0; y < getBinCount(); y++) {
            
            
            double v = histogramData->GetValue(x, y).ToDouble() / (max+1);

            // sqrt
            //v = sqrt(histogramData->GetValue(x, y)) / sqrt(max);

            //log 
            //v = std::fmax(0, log(histogramData->GetValue(x, y).ToInt()) + 1) / (log(max) + 1);

            //getHeatMapColor(v, &r, &g , &b);
            //painter.setBrush(QColor(r, g, b, 127));
            QColor color;
            float h = (1.f - v) * 0.85f + 0.15f;
            float s = 1.f;
            float l = v*0.5f + 0.25f;
            color.setHsl(h * 255 ,s * 255, l * 255);
            painter.setBrush(QBrush(color));
            painter.fillRect((x - firstVisibleTick) * tickSize , y * binSize, tickSize + 1, binSize + 1, painter.brush());
        }    
    }
    int x = (tick - firstVisibleTick) * tickSize;

    QColor WHITE_COLOR{255, 255, 255};
    painter.setPen(QPen(WHITE_COLOR));
    //painter.setBrush(QBrush(QColor(255,255,255)));
    painter.drawLine(x , 0, x, height);

    
    
}

void HistogramWidget::mousePressEvent(QMouseEvent* e) {
    if (e->button() == Qt::LeftButton) {
        float tickSize = (float)geometry().width() / (float)getVisibleTicks();
        tick = round(e->localPos().x() / tickSize) + firstVisibleTick;
        std::cout << "First tick: " << firstVisibleTick << std::endl;
        std::cout << "Timestep: " << tick << std::endl;
        histogramClicked(tick);
    }
}


void HistogramWidget::mouseReleaseEvent(QMouseEvent*) {

}


void HistogramWidget::setTick(int newTick) {
    tick = newTick;
}

void HistogramWidget::recomputeMinMax() {
    max = 0;
    min = INFINITY;
    for (int i = 0; i < histogramData->GetNumberOfRows(); i++) {
        for (int j = 0; j < histogramData->GetNumberOfColumns(); j++) {
            max = std::fmax(max, histogramData->GetValue( i,j).ToDouble());
            min = std::fmin(min, histogramData->GetValue( i, j).ToDouble());
        }
    }

    std::cout << "max: " << max << ", min: " << min<< "\n";

}