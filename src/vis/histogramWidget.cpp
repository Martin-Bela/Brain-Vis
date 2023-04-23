// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "histogramWidget.hpp"
#include <iostream>

#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>

//! [0]
HistogramWidget::HistogramWidget(QWidget *parent)
    : QWidget(parent)
{
    antialiased = false;

    setBackgroundRole(QPalette::Base);
    setAutoFillBackground(true);
}

int HistogramWidget::getYPos(double value) {
    auto val = geometry().height() - ((value - sMin) / (sMax - sMin)) * geometry().height();
    return val;
}

void HistogramWidget::paintSummary(QPainter& painter) {
    float tickSize = (float)geometry().width() / (float)getVisibleTicks();
    painter.fillRect(0, 0, geometry().width(), geometry().height(), QBrush({ 255, 255, 255 }));

    QPen black = QPen({ 0,   0, 0   });
    QPen blue  = QPen({ 0,   0, 255 });
    QPen red   = QPen({ 255, 0, 0   });
    black.setWidth(2);
    blue.setWidth(2);
    red.setWidth(2);
    // TODO Use PixMap?
    for (int i = firstVisibleTick; i <= lastVisibleTick - 1; i++) {
        int x0 = (i - firstVisibleTick) * tickSize;
        int x1 = (i + 1 - firstVisibleTick) * tickSize;

        /*
        painter.setPen(red);
        painter.drawLine(x0, getYPos(summaryData->GetValue(i + 1, 2).ToDouble()),
                         x1, getYPos(summaryData->GetValue(i + 2, 2).ToDouble()));

        painter.setPen(blue);
        painter.drawLine(x0, getYPos(summaryData->GetValue(i + 1, 3).ToDouble()),
                         x1, getYPos(summaryData->GetValue(i + 2, 3).ToDouble()));
        */

        painter.setPen(black);
        painter.drawLine(x0, getYPos(summaryData->GetValue(i + 1, 0).ToDouble()),
                         x1, getYPos(summaryData->GetValue(i + 2, 0).ToDouble()));
    }
}

void HistogramWidget::paintHistogram(QPainter &painter) {  
    float tickSize = (float) geometry().width() / (float)getVisibleTicks();
    float binSize = (float) geometry().height() / (float)getBinCount();
    
    std::cout << "Bins:" << getBinCount() << ", Steps: " << getVisibleTicks() << "\n";
    std::cout << "BinSize:" << binSize << ", TimestepSize: " << tickSize << "\n";
    
    // TODO Use PixMap!
    for (int x = firstVisibleTick; x <= lastVisibleTick; x++) {
        for (int y = 0; y < getBinCount(); y++) {
            double v = histogramData->GetValue(x, y).ToDouble() / (max + 1);
            QColor color;
            float h = (1.f - v) * 0.85f + 0.15f;
            float s = 1.f;
            float l = v * 0.5f + 0.25f;
            color.setHsl(h * 255, s * 255, l * 255);
            painter.setBrush(QBrush(color));
            painter.fillRect((x - firstVisibleTick) * tickSize, y * binSize, tickSize + 1, binSize + 1, painter.brush());
        }
    }
}

void HistogramWidget::paintEvent(QPaintEvent * /* event */)
{
    if(!isLoaded())
        return;

    QPainter painter(this);

    if (antialiased) painter.setRenderHint(QPainter::Antialiasing, true);

    if (summaryDrawMode) {
        paintSummary(painter);
        painter.setPen(QPen({ 0, 255, 0 }));
    }
    else {
        paintHistogram(painter);
        painter.setPen(QPen({ 255, 255, 255 }));
    }

    float tickSize =  (float) geometry().width() / (float) getVisibleTicks();
    int x = (tick - firstVisibleTick) * tickSize;
    painter.drawLine(x , 0, x, geometry().height());
}

void HistogramWidget::mousePressEvent(QMouseEvent* e) {
    if (e->buttons() & Qt::LeftButton) {
        float tickSize = (float)geometry().width() / (float)getVisibleTicks();
        tick = round(e->position().x() / tickSize) + firstVisibleTick;
        std::cout << "First tick: " << firstVisibleTick << std::endl;
        std::cout << "Timestep: " << tick << std::endl;
        histogramCursorMoved(tick);
    }
}


void HistogramWidget::mouseReleaseEvent(QMouseEvent*) {

}

void HistogramWidget::mouseMoveEvent(QMouseEvent* e) {
    if (e->buttons() & Qt::LeftButton) {
        float tickSize = (float)geometry().width() / (float)getVisibleTicks();
        tick = round(e->position().x() / tickSize) + firstVisibleTick;
        std::cout << "First tick: " << firstVisibleTick << std::endl;
        std::cout << "Timestep: " << tick << std::endl;
        histogramCursorMoved(tick);
    }
}


void HistogramWidget::setTick(int newTick) {
    tick = newTick;
}

void HistogramWidget::recomputeSummaryMinMax() {
    sMax = -INFINITY;
    sMin = INFINITY;
    for (int i = 1; i < summaryData->GetNumberOfRows(); i++) {
        sMax = std::fmax(sMax, summaryData->GetValue(i, 0).ToDouble());
        sMin = std::fmin(sMin, summaryData->GetValue(i, 0).ToDouble());
    }
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