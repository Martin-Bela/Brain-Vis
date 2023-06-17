#include<Qwidget>
#include<QObject>
#include<QPushButton>

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

    Ui::Form form{};
    form.setupUi(this);

    setWindowTitle("Histogram tooltip");

    form.image_label->setPixmap(pixmap);
}

void HistogramPopUp::adjustPosition() {
    QRect rect = button->geometry();
    QPoint position = parent->mapToGlobal(rect.topRight());
    position.setX(position.x() + 10);
    position.setY(position.y() - geometry().size().height());
    setGeometry(QRect(position, geometry().size()));
}
