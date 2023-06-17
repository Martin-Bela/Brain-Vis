#pragma once 

#include "histogramWidget.hpp"

#include <QWidget>
#include <QObject>
#include <QPainter>

#include <QLabel>
#include <QSpacerItem>
#include <QVBoxLayout>
#include <QResizeEvent>

class HistogramSliderLabelWidget : public QWidget {
    Q_OBJECT

    int timestep = 0;
    QLabel* label;
    QSpacerItem* spacer;

    void adjustSpacerWidth(int newWidth) {
        auto spacerWidth = (newWidth - label->width()) * timestep / 10000;
        spacer->changeSize(spacerWidth, 0, QSizePolicy::Fixed);
    }
public:
    HistogramSliderLabelWidget(QWidget* parent) :
        QWidget(parent)
    {
        auto* layout = new QHBoxLayout();

        spacer = new QSpacerItem(0, 0, QSizePolicy::Fixed);
        layout->addItem(spacer);
        layout->setContentsMargins(0, 0, 0, 0);

        label = new QLabel(parent);
        label->setText("0");
        layout->addWidget(label);

        layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding));

        setLayout(layout);
    }

    void setTimestep(int timestep) {
        this->timestep = timestep;
        adjustSpacerWidth(width());
        label->setText(QString::fromStdString(std::to_string(timestep * 100)));
    }

    void resizeEvent(QResizeEvent* event) override {
        int newWidth = event->size().width();
        adjustSpacerWidth(newWidth);
    }
};

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