#include "histogramSliderWidget.hpp"
#include "magmaColormap.hpp"

#include <QPainter>
#include <QMouseEvent>



HistogramSliderWidget::HistogramSliderWidget(QWidget* parent) : HistogramWidget(parent)
{
	setVisibleRange(0, 9999);
	//setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
}

void HistogramSliderWidget::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);

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

    int tickSize = 2;
    int x = (tick / (double)getVisibleTicks()) * geometry().width();
    painter.drawLine(x, 0, x, geometry().height());
}

void HistogramSliderWidget::mousePressEvent(QMouseEvent* e)
{
    if (e->buttons() & Qt::LeftButton) {
        //float tickSize = (float)geometry().width() / (float)getVisibleTicks();
        double pos = std::clamp(e->position().x(), 0.0, (double)geometry().width());
        setTick(round((pos / (double)geometry().width()) * getVisibleTicks()) + firstVisibleTick);
        histogramCursorMoved(tick);
    }
    update();
}

void HistogramSliderWidget::mouseMoveEvent(QMouseEvent* e)
{
    if (e->buttons() & Qt::LeftButton) {
        //float tickSize = (float)geometry().width() / (float)getVisibleTicks();
        double pos = std::clamp(e->position().x(), 0.0, (double)geometry().width());
        setTick(round((pos / (double)geometry().width()) * getVisibleTicks()) + firstVisibleTick);
        histogramCursorMoved(tick);
    }
    update();
}

void HistogramSliderWidget::paintHistogram(QPainter& painter)
{
    int tickSize = 1;
    int dataBinSize = tickSize * getVisibleTicks() / geometry().width();
    
    float binSize = (float)geometry().height() / (float) getBinCount();

    // TODO Use PixMap!
    for (int x = firstVisibleTick; x <= lastVisibleTick; x += dataBinSize) {
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
            QColor color = getMagmaColor(val);
            painter.setBrush(QBrush(color));
            painter.fillRect(x / dataBinSize - firstVisibleTick, y_pos * binSize, tickSize, binSize + 1, painter.brush());    
        }
        paintHistogramTick(painter, x, binSize, tickSize);
    }

}

void HistogramSliderWidget::paintHistogramTick(QPainter& painter, int tick, float binSize, float tickSize)
{
    
}

void HistogramSliderWidget::paintSummary(QPainter& painter, bool redraw)
{
    std::cout << "Summary paint" << std::endl;
    
    int tickSize = 1;
    int dataBinSize = tickSize * getVisibleTicks() / geometry().width();

    QPen black = QPen({ 0,   0, 0 });
    QPen blue = QPen({ 0,   0, 255 });
    QPen red = QPen({ 255, 0, 0 });
    black.setWidth(2);
    blue.setWidth(2);
    red.setWidth(2);
    // TODO Use PixMap?
    if (redraw) {
        painter.fillRect(0, 0, geometry().width(), geometry().height(), QBrush({ 255, 255, 255 }));
    }

    for (int i = firstVisibleTick; i <= lastVisibleTick - dataBinSize; i += dataBinSize) {
        int x0 = i / dataBinSize - firstVisibleTick;
        int x1 = i / dataBinSize + tickSize - firstVisibleTick;

        double max1 = 0.0, val1 = 0.0, min1 = 0.0, max2 = 0.0, val2 = 0.0, min2 = 0.0;

        for (int b = 0; b < dataBinSize; b++) {
            max1 += summaryTable[i + b][2];
            min1 += summaryTable[i + b][3];
            val1 += summaryTable[i + b][0];
            max2 += summaryTable[i + b + dataBinSize][2];
            min2 += summaryTable[i + b + dataBinSize][3];
            val2 += summaryTable[i + b + dataBinSize][0];
        }
        max1 /= dataBinSize;
        max2 /= dataBinSize;
        val1 /= dataBinSize;
        val2 /= dataBinSize;
        min1 /= dataBinSize;
        min2 /= dataBinSize;


        painter.setPen(red);
        painter.drawLine(x0, getYPos(max1), x1, getYPos(max2));

        painter.setPen(blue);
        painter.drawLine(x0, getYPos(min1), x1, getYPos(min2));

        painter.setPen(black);
        painter.drawLine(x0, getYPos(val1), x1, getYPos(val2));

    }
    
}

void HistogramSliderWidget::paintSummaryTick(QPainter& painter, int tick, float tickSize, QPen& black, QPen& blue, QPen& red)
{
    
}
