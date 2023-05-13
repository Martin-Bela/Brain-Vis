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

void HistogramWidget::paintSummaryTick(QPainter& painter, int tick, float tickSize, QPen& black, QPen& blue, QPen& red) {
    int x0 = (tick - firstVisibleTick) * tickSize;
    int x1 = (tick + 1 - firstVisibleTick) * tickSize;

    if (tick >= summaryTable.size() - 1) {
        return;
    }
    painter.setPen(red);
    painter.drawLine(x0, getYPos(summaryTable[tick][2]), x1, getYPos(summaryTable[tick + 1][2]));

    painter.setPen(blue);
    painter.drawLine(x0, getYPos(summaryTable[tick][3]), x1, getYPos(summaryTable[tick + 1][3]));

    painter.setPen(black);
    painter.drawLine(x0, getYPos(summaryTable[tick][0]), x1, getYPos(summaryTable[tick + 1][0]));
}

void HistogramWidget::paintSummary(QPainter& painter, bool redraw) {
    float tickSize = (float)geometry().width() / (float)getVisibleTicks();
    
    QPen black = QPen({ 0,   0, 0   });
    QPen blue  = QPen({ 0,   0, 255 });
    QPen red   = QPen({ 255, 0, 0   });
    black.setWidth(2);
    blue.setWidth(2);
    red.setWidth(2);
    // TODO Use PixMap?
    if (dirty) {
        if (redraw) {
            painter.fillRect(0, 0, geometry().width(), geometry().height(), QBrush({ 255, 255, 255 }));
        }
        
        for (int i = firstVisibleTick; i <= lastVisibleTick - 1; i++) {
            paintSummaryTick(painter, i, tickSize, black, blue, red);
        }
    }
    else {
        for (int previousTick : previousTicks) {
            int x0 = (previousTick - firstVisibleTick) * tickSize;
            int x1 = (previousTick + 1 - firstVisibleTick) * tickSize;

            if (redraw) {
                painter.fillRect(x0, 0, tickSize, geometry().height(), QBrush({ 255, 255, 255 }));
            }

            paintSummaryTick(painter, previousTick, tickSize, black, blue, red);
        }
        previousTicks.clear();
    }
}

void HistogramWidget::paintHistogramTick(QPainter& painter, int tick, float binSize, float tickSize) {
    for (int y = 0; y < getBinCount(); y++) {
        int y_pos = getBinCount() - y - 1;
        double v = histogramTable[tick][y] / (max + 1);

        if (logarithmicScaleEnabled) {
            v = std::fmax(0, log(histogramTable[tick][y]) + 1) / (log(max) + 1);
        }

        QColor color = getMagmaColor(v);
        painter.setBrush(QBrush(color));
        painter.fillRect((tick - firstVisibleTick) * tickSize, y_pos * binSize, tickSize + 1, binSize + 1, painter.brush());
    }
}


void HistogramWidget::paintHistogram(QPainter& painter) {  
    float tickSize = (float) geometry().width() / (float) getVisibleTicks();
    float binSize = (float) geometry().height() / (float) getBinCount();
    
    std::cout << "Bins:" << getBinCount() << ", Steps: " << getVisibleTicks() << "\n";
    std::cout << "BinSize:" << binSize << ", TimestepSize: " << tickSize << "\n";
    
    // TODO Use PixMap!
    if (dirty) {
        for (int x = firstVisibleTick; x <= lastVisibleTick; x++) {
            paintHistogramTick(painter, x, binSize, tickSize);
        }
    }
    else {
        for (int previousTick : previousTicks) {
            paintHistogramTick(painter, previousTick, binSize, tickSize);
        }

        if (drawMode != Both) {
            previousTicks.clear();
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
    dirty = false;

    float tickSize =  (float) geometry().width() / (float) getVisibleTicks();
    int x = (tick - firstVisibleTick) * tickSize;
    painter.fillRect(x , 0, tickSize, geometry().height(), QColor::fromRgb(0,0,0));
}

void HistogramWidget::mousePressEvent(QMouseEvent* e) {
    if (e->buttons() & Qt::LeftButton) {
        float tickSize = (float) geometry().width() / (float) getVisibleTicks();
        double pos = std::clamp(e->position().x(), 0.0, (double) geometry().width());
        setTick(round(pos / tickSize) + firstVisibleTick);
        histogramCursorMoved(tick);
    }
    update();
}


void HistogramWidget::mouseReleaseEvent(QMouseEvent*) {

}

void HistogramWidget::mouseMoveEvent(QMouseEvent* e) {
    if (e->buttons() & Qt::LeftButton) {
        float tickSize = (float)geometry().width() / (float)getVisibleTicks();
        double pos = std::clamp(e->position().x(), 0.0, (double)geometry().width());
        setTick(round(pos / tickSize) + firstVisibleTick);
        histogramCursorMoved(tick);
    }
    update();
}

void HistogramWidget::resizeEvent(QResizeEvent* e) {
    dirty = true;
    update();
}

void HistogramWidget::keyPressEvent(QKeyEvent* e) {
    if (e->key() == Qt::Key_Left || e->key() == Qt::Key_A)  {
        setTick(std::max(firstVisibleTick, tick - 1));
        histogramCursorMoved(tick);
    }
    else if (e->key() == Qt::Key_Right || e->key() == Qt::Key_D) {
        setTick(std::min(lastVisibleTick, tick + 1));
        histogramCursorMoved(tick);
    }
    update();
}


void HistogramWidget::setTick(int newTick) {
    previousTicks.emplace_back(tick);
    tick = newTick;
}

void HistogramWidget::recomputeSummaryMinMax() {
    sMax = -INFINITY;
    sMin = INFINITY;
    for (int i = 0; i < summaryTable.size(); i++) {
        sMax = std::fmax(sMax, summaryTable[i][2]);
        sMin = std::fmin(sMin, summaryTable[i][3]);
    }
}

void HistogramWidget::recomputeMinMax() {
    max = 0;
    min = INFINITY;
    for (int i = 0; i < histogramTable.size(); i++) {
        for (int j = 0; j < histogramTable[0].size(); j++) {
            max = std::fmax(max, histogramTable[i][j]);
            min = std::fmin(min, histogramTable[i][j]);
        }
    }

    std::cout << "max: " << max << ", min: " << min<< "\n";
}