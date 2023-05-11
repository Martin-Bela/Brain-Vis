// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "histogramWidget.hpp"
#include <iostream>

#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>

#include "magmaColormap.hpp"

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

void HistogramWidget::paintSummary(QPainter& painter, bool redraw) {
    float tickSize = (float)geometry().width() / (float)getVisibleTicks();
    
    if (redraw) {
        painter.fillRect(0, 0, geometry().width(), geometry().height(), QBrush({ 255, 255, 255 }));
    }

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

        painter.setPen(red);
        painter.drawLine(x0, getYPos(summaryData->GetValue(i + 1, 2).ToDouble()),
                         x1, getYPos(summaryData->GetValue(i + 2, 2).ToDouble()));

        painter.setPen(blue);
        painter.drawLine(x0, getYPos(summaryData->GetValue(i + 1, 3).ToDouble()),
                         x1, getYPos(summaryData->GetValue(i + 2, 3).ToDouble()));

        painter.setPen(black);
        painter.drawLine(x0, getYPos(summaryData->GetValue(i + 1, 0).ToDouble()),
                         x1, getYPos(summaryData->GetValue(i + 2, 0).ToDouble()));
    }
}

void HistogramWidget::paintHistogram(QPainter& painter) {  
    float tickSize = (float) geometry().width() / (float)getVisibleTicks();
    float binSize = (float) geometry().height() / (float)getBinCount();
    
    std::cout << "Bins:" << getBinCount() << ", Steps: " << getVisibleTicks() << "\n";
    std::cout << "BinSize:" << binSize << ", TimestepSize: " << tickSize << "\n";
    
    // TODO Use PixMap!
    for (int x = firstVisibleTick; x <= lastVisibleTick; x++) {
        for (int y = 0; y < getBinCount(); y++) {
            int y_pos = getBinCount() - y - 1;
            double v = histogramData->GetValue(x, y).ToDouble() / (max + 1);

            if (logarithmicScaleEnabled) {
                v = std::fmax(0, log(histogramData->GetValue(x, y).ToInt()) + 1) / (log(max) + 1);
            }

            QColor color = getMagmaColor(v);
            //float h = (1.f - v) * 0.85f + 0.15f;
            //float s = 1.f;
            //float l = v * 0.5f + 0.25f;
            //color.setHsl(h * 255, s * 255, l * 255);
            painter.setBrush(QBrush(color));
            painter.fillRect((x - firstVisibleTick) * tickSize, y_pos * binSize, tickSize + 1, binSize + 1, painter.brush());
        }
    }

}

void HistogramWidget::paintMinMaxLabels(QPainter &painter, QColor color) {  
        // draw Text
    int xOffset = 5;
    int yOffset = 10;

    painter.setPen(color);
    painter.drawText(xOffset, yOffset, std::format("{}", sMax).c_str());
    painter.drawText(xOffset, geometry().height() - 2, std::format("{}", sMin).c_str());

}

void HistogramWidget::paintEvent(QPaintEvent * /* event */)
{
    if(!isLoaded())
        return;

    QPainter painter(this);

    if (antialiased) painter.setRenderHint(QPainter::Antialiasing, true);
    
    //std::cout << "DrawMode: " << drawMode <<" \n";
    switch (drawMode) {
    case Summary:
        paintSummary(painter, true);
        painter.setPen(QPen({ 0, 255, 0 }));
        paintMinMaxLabels(painter, Qt::black);
        break;
    case Histogram:
        paintHistogram(painter);
        painter.setPen(QPen({ 255, 255, 255 }));
        paintMinMaxLabels(painter, Qt::black);
        break;
    case Both:
        paintHistogram(painter);
        painter.setPen(QPen({ 255, 255, 255 }));
        paintSummary(painter, false);
        painter.setPen(QPen({ 0, 255, 0 }));
        paintMinMaxLabels(painter, Qt::black);
        break;
    }

    float tickSize =  (float) geometry().width() / (float) getVisibleTicks();
    int x = (tick - firstVisibleTick) * tickSize;
    painter.drawLine(x , 0, x, geometry().height());
}

void HistogramWidget::mousePressEvent(QMouseEvent* e) {
    if (e->buttons() & Qt::LeftButton) {
        float tickSize = (float)geometry().width() / (float)getVisibleTicks();
        previousTick = tick;
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
        previousTick = tick;
        tick = round(e->position().x() / tickSize) + firstVisibleTick;
        std::cout << "First tick: " << firstVisibleTick << std::endl;
        std::cout << "Timestep: " << tick << std::endl;
        histogramCursorMoved(tick);
    }
}

void HistogramWidget::resizeEvent(QResizeEvent*) {
    dirty = true;
}

void HistogramWidget::keyPressEvent(QKeyEvent* e) {
    if (e->key() == Qt::Key_Left || e->key() == Qt::Key_A)  {
        previousTick = tick;
        tick = std::max(firstVisibleTick, tick - 1);
        histogramCursorMoved(tick);
    }
    else if (e->key() == Qt::Key_Right || e->key() == Qt::Key_D) {
        previousTick = tick;
        tick = std::min(lastVisibleTick, tick + 1);
        histogramCursorMoved(tick);
    }
    update();
}


void HistogramWidget::setTick(int newTick) {
    previousTick = tick;
    tick = newTick;
}

void HistogramWidget::recomputeSummaryMinMax() {
    sMax = -INFINITY;
    sMin = INFINITY;
    for (int i = 1; i < summaryData->GetNumberOfRows(); i++) {
        sMax = std::fmax(sMax, summaryData->GetValue(i, 2).ToDouble());
        sMin = std::fmin(sMin, summaryData->GetValue(i, 3).ToDouble());
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