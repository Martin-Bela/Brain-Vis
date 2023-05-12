#include "histogramWidget.hpp"

#include <QWidget>
#include <QObject>
#include <QPainter>

class HistogramSliderWidget : public HistogramWidget
{
    
    Q_OBJECT

public:
    HistogramSliderWidget(QWidget* parent = nullptr);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;

private:
    int dataBinSize = 0;
    int tickSize = 1;

    void paintHistogram(QPainter& painter);
    void paintHistogramTick(QPainter& painter, int tick, float binSize, float tickSize);
    void paintSummary(QPainter& painter, bool redraw);
    void paintSummaryTick(QPainter& painter, int tick, float tickSize, QPen& black, QPen& blue, QPen& red);

    int getXPos(double val);

};