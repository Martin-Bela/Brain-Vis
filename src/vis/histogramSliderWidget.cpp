#include "histogramSliderWidget.hpp"
#include "magmaColormap.hpp"

#include <QPainter>
#include <QMouseEvent>



HistogramSliderWidget::HistogramSliderWidget(QWidget* parent) : HistogramWidget(parent)
{
	setVisibleRange(0, 9999);
}

void HistogramSliderWidget::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    if (antialiased) painter.setRenderHint(QPainter::Antialiasing, true);

    dataBinSize = tickSize * getVisibleTicks() / geometry().width();

    switch (drawMode) {
    case Summary:
        paintSummary(painter, true);
        break;
    case Histogram:
        paintHistogram(painter);
        break;
    case Both:
        paintHistogram(painter);
        paintSummary(painter, false);
        break;
    }

    QColor black = { 0, 0, 0 };
    black.setAlphaF(0.3);
    QBrush brush = QBrush(black);
    
    int lowerBoundary = std::max(tick - 250, firstVisibleTick);
    int upperBoundary = std::min(tick + 250, lastVisibleTick);

    if (lowerBoundary == firstVisibleTick) upperBoundary = 500;
    if (upperBoundary == lastVisibleTick) lowerBoundary = lastVisibleTick - 500;
    painter.fillRect(getXPos(lowerBoundary), 0, getXPos(upperBoundary) - getXPos(lowerBoundary), geometry().height(), brush);

    int x = getXPos(tick);
    painter.drawLine(x, 0, x, geometry().height());
    dirty = false;
}

void HistogramSliderWidget::mousePressEvent(QMouseEvent* e)
{
    if (e->buttons() & Qt::LeftButton) {
        double pos = std::clamp(e->position().x(), 0.0, (double)geometry().width());
        setTick(round((pos / (double)geometry().width()) * getVisibleTicks()) + firstVisibleTick);
        histogramCursorMoved(tick);
    }
    update();
}

void HistogramSliderWidget::mouseMoveEvent(QMouseEvent* e)
{
    if (e->buttons() & Qt::LeftButton) {
        double pos = std::clamp(e->position().x(), 0.0, (double)geometry().width());
        setTick(round((pos / (double)geometry().width()) * getVisibleTicks()) + firstVisibleTick);
        histogramCursorMoved(tick);
    }
    update();
}

void HistogramSliderWidget::paintHistogram(QPainter& painter)
{ 
    float binSize = (float)geometry().height() / (float) getBinCount();

    // TODO Use PixMap!
    if (dirty) {
        std::cout << getBinCount() << std::endl;
        for (int x = firstVisibleTick; x <= lastVisibleTick - dataBinSize; x += dataBinSize) {
            paintHistogramTick(painter, x, binSize, tickSize);
        }
    }
    else {
        for (int previous : previousTicks) {
            int lowerBoundary = std::max(previous - 250, firstVisibleTick);
            int upperBoundary = std::min(previous + 250, lastVisibleTick);

            if (lowerBoundary == firstVisibleTick) upperBoundary = 500;
            if (upperBoundary == lastVisibleTick) lowerBoundary = lastVisibleTick - 500;
            
            for (int x = lowerBoundary; x <= upperBoundary; x += dataBinSize) {
                if (x + dataBinSize >= lastVisibleTick) continue;
                paintHistogramTick(painter, x, binSize, tickSize);
            }
        }
        if (drawMode != Both) {
            previousTicks.clear();
        }
    }
}

void HistogramSliderWidget::paintHistogramTick(QPainter& painter, int x, float binSize, float tickSize)
{
    for (int y = 0; y < getBinCount(); y++) {
        int y_pos = getBinCount() - y - 1;
        double val = 0;
        for (int i = 0; i < dataBinSize; i++) {
            double v = histogramTable[x + i][y] / (max + 1);

            if (logarithmicScaleEnabled) {
                v = std::fmax(0, log(histogramTable[x + i][y]) + 1) / (log(max) + 1);
            }

            val += v;
        }
        QColor color = getMagmaColor(val / dataBinSize);
        painter.setBrush(QBrush(color));
        painter.fillRect(getXPos(x), y_pos * binSize, tickSize, binSize + 1, painter.brush());
    }
}

void HistogramSliderWidget::paintSummary(QPainter& painter, bool redraw)
{ 
    QPen black = QPen({ 0,   0, 0 });
    QPen blue = QPen({ 0,   0, 255 });
    QPen red = QPen({ 255, 0, 0 });
    black.setWidth(2);
    blue.setWidth(2);
    red.setWidth(2);
    // TODO Use PixMap?
    if (dirty) {
        if (redraw) {
            painter.fillRect(0, 0, geometry().width(), geometry().height(), QBrush({ 255, 255, 255 }));
        }

        for (int i = firstVisibleTick; i < lastVisibleTick; i += dataBinSize) {
            paintSummaryTick(painter, i, tickSize, black, blue, red);
        }
    }
    else {
        for (int previous : previousTicks) {
            int lowerBoundary = std::max(previous - 250, firstVisibleTick);
            int upperBoundary = std::min(previous + 250, lastVisibleTick);
            if (lowerBoundary == firstVisibleTick) upperBoundary = 500;
            if (upperBoundary == lastVisibleTick) lowerBoundary = lastVisibleTick - 500;


            if (redraw) {
                painter.fillRect(getXPos(lowerBoundary), 0, getXPos(upperBoundary) - getXPos(lowerBoundary), geometry().height(), QBrush({255, 255, 255}));
            }

            for (int x = lowerBoundary; x <= upperBoundary; x += dataBinSize) {
                if (x + dataBinSize >= lastVisibleTick) continue;
                paintSummaryTick(painter, x, tickSize, black, blue, red);
            }
        }
        previousTicks.clear();
    }
    
}

void HistogramSliderWidget::paintSummaryTick(QPainter& painter, int tick, float tickSize, QPen& black, QPen& blue, QPen& red)
{
    int x0 = getXPos(tick);
    int x1 = getXPos(tick + dataBinSize);

    double max1 = 0.0, val1 = 0.0, min1 = 0.0, max2 = 0.0, val2 = 0.0, min2 = 0.0;

    int n = 0;
    for (int b = 0; b < dataBinSize; b++) {
        if (tick + b + dataBinSize >= summaryTable.size()) {
            break;
        }
        max1 += summaryTable[tick + b][2];
        min1 += summaryTable[tick + b][3];
        val1 += summaryTable[tick + b][0];
        max2 += summaryTable[tick + b + dataBinSize][2];
        min2 += summaryTable[tick + b + dataBinSize][3];
        val2 += summaryTable[tick + b + dataBinSize][0];
        n++;
    }
    max1 /= n;
    max2 /= n;
    val1 /= n;
    val2 /= n;
    min1 /= n;
    min2 /= n;

    painter.setPen(red);
    painter.drawLine(x0, getYPos(max1), x1, getYPos(max2));

    painter.setPen(blue);
    painter.drawLine(x0, getYPos(min1), x1, getYPos(min2));

    painter.setPen(black);
    painter.drawLine(x0, getYPos(val1), x1, getYPos(val2));
}

int HistogramSliderWidget::getXPos(double val)
{
    return (val / (double) getVisibleTicks()) * geometry().width();
}
