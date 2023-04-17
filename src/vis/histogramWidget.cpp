// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "histogramWidget.hpp"
#include <iostream>

#include <QPainter>
#include <QPainterPath>

void getHeatMapColor(float value, float *red, float *green, float *blue)
{
  const int NUM_COLORS = 4;
  static float color[NUM_COLORS][3] = { {0,0,1}, {0,1,0}, {1,1,0}, {1,0,0} };
    // A static array of 4 colors:  (blue,   green,  yellow,  red) using {r,g,b} for each.
  
  int idx1;        // |-- Our desired color will be between these two indexes in "color".
  int idx2;        // |
  float fractBetween = 0;  // Fraction between "idx1" and "idx2" where our value is.
  
  if(value <= 0)      {  idx1 = idx2 = 0;            }    // accounts for an input <=0
  else if(value >= 1)  {  idx1 = idx2 = NUM_COLORS-1; }    // accounts for an input >=0
  else
  {
    value = value * (NUM_COLORS-1);        // Will multiply value by 3.
    idx1  = floor(value);                  // Our desired color will be after this index.
    idx2  = idx1+1;                        // ... and before this index (inclusive).
    fractBetween = value - float(idx1);    // Distance between the two indexes (0-1).
  }
    
  *red   = (color[idx2][0] - color[idx1][0])*fractBetween + color[idx1][0];
  *green = (color[idx2][1] - color[idx1][1])*fractBetween + color[idx1][1];
  *blue  = (color[idx2][2] - color[idx1][2])*fractBetween + color[idx1][2];
}

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
    float tickSize =  (float)width / (float)getVisibleTicks();

    // TODo Ue PixMap!
    std::cout <<"Bins:" << getBinCount() << ", Steps: " << getVisibleTicks() << "\n";
    float binSize = (float)height / (float)getBinCount();
    std::cout <<"BinSize:" << binSize << ", TimestepSize: " << tickSize << "\n";
    for ( int x = 0; x < getVisibleTicks(); x++) {
        for ( int y = 0; y < getBinCount(); y++) {
            
            
            double v = histogramData->GetValue(x, y) / max;

            // sqrt
            //v = sqrt(histogramData->GetValue(x, y)) / sqrt(max);

            //log 
            v = std::fmax(0, log(histogramData->GetValue(x, y)) + 1) / (log(max) + 1);

            //getHeatMapColor(v, &r, &g , &b);
            //painter.setBrush(QColor(r, g, b, 127));
            QColor color;
            float h = (1.f - v) * 0.85f + 0.15f;
            float s = 1.f;
            float l = v*0.5f + 0.25f;
            color.setHsl(h * 255 ,s * 255, l * 255);
            painter.setBrush(QBrush(color));
            painter.fillRect(x * tickSize , y * binSize, tickSize+1, binSize+1, painter.brush());
        }    
    }
    int x = tick * tickSize;

    QColor WHITE_COLOR{255, 255, 255};
    painter.setPen(QPen(WHITE_COLOR));
    //painter.setBrush(QBrush(QColor(255,255,255)));
    painter.drawLine(x , 0, x, height);

    
    
}



void HistogramWidget::mousePressEvent(QMouseEvent*) {

}

void HistogramWidget::mouseReleaseEvent(QMouseEvent*) {

}


void HistogramWidget::setTick(int newTick) {
    tick = newTick;
    //update();
}

