#include<Qwidget>
#include<QObject>
#include<QPushButton>
#include <Qstring>

#include "popupWidget.hpp"

#include "ui_histogramTooltip.h"


PopUpWidget::PopUpWidget(QWidget* parent, QPushButton* button) :
    QWidget(parent), parent(parent), button(button)
{
    setWindowFlags(Qt::Tool);
    adjustPosition();

    QObject::connect(button, &QPushButton::clicked, [=](){
        setVisible(!isVisible());
        adjustPosition();
    });
}

void PopUpWidget::adjustPosition() {
    QRect rect = button->geometry();
    QPoint position = parent->mapToGlobal(rect.topRight());
    position.setX(position.x() + 10);
    setGeometry(QRect(position, geometry().size()));
}

QSize PopUpWidget::sizeHint() const
{
    return QSize(0,0);
}

HistogramPopUp::HistogramPopUp(QWidget* parent, QPushButton* button) :
    PopUpWidget(parent, button)
{
    QImage image(10, magmaColorMap.size(), QImage::Format_RGB888);
    for (int i = 0; const auto & pixel : magmaColorMap) {
        for (int x = 0; x < image.width(); x++) {
            image.setPixelColor(x, image.height() - 1 - i, pixel.rgb());
        }
        i++;
    }
    pixmap.convertFromImage(image);

    Ui::HistogramTooltip HistogramTooltipUI{};
    HistogramTooltipUI.setupUi(this);

    setWindowTitle("Histogram tooltip");

    HistogramTooltipUI.image_label->setPixmap(pixmap);
}

void HistogramPopUp::adjustPosition() {
    QRect rect = button->geometry();
    QPoint position = parent->mapToGlobal(rect.topRight());
    position.setX(position.x() + 10);
    position.setY(position.y() - geometry().size().height());
    setGeometry(QRect(position, geometry().size()));
}

namespace {
    struct Attribute {
        const char* name;
        const char* describtion;
    };

    auto attributes = std::to_array<Attribute>({
        { "Fired", "Boolean: Did the neuron fire within the last sample step" },
        { "Fired Fraction", "In Percent: Number of firings since the last sampling" },
        { "x", "Electric Activity" },
        { "Secondary Variable", "Inhibition variable used for the firing model of Izhikevich" },
        { "Calcium", "Current calcium level" },
        { "Target Calcium", "Target calcium level" },
        { "Synaptic Input", "Input electrical activity" },
        { "Background Activity", "Background noise electric activity input" },
        { "Grown Axons", "Number of currently grown axonal boutons" },
        { "Connected Axons", "Number of current outgoing connections" },
        { "Grown Excitatory Dendrites", "Number of currently grown dendrite spines for excitatory connections" },
        { "Connected Excitatory Dendrites", "Number of incoming excitatory connections" }
    });
}

PropertiesPopUp::PropertiesPopUp(QWidget* parent, QPushButton* button) :
    PopUpWidget(parent, button)
{
    setWindowTitle("Neuron Properties");

    auto* layout = new QGridLayout(this);

    QFont bold{};
    bold.setBold(true);

    for (int i = 0; i < attributes.size(); i++) {
        auto* nameLabel = new QLabel();
        nameLabel->setText(attributes[i].name);
        nameLabel->setFont(bold);
        //nameLabel->setAlignment(Qt::AlignRight);
       
        layout->addWidget(nameLabel, i, 0);

        auto* descLabel = new QLabel();
        descLabel->setText(attributes[i].describtion);
        layout->addWidget(descLabel, i, 1);
    }

    setLayout(layout);
}
