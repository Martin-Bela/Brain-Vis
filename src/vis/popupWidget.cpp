#include<Qwidget>
#include<QObject>
#include<QPushButton>

#include "popupWidget.hpp"

PopUpWidget::PopUpWidget(QWidget* parent, QPushButton* button) :
    QWidget(parent), parent(parent), button(button)
{
    setWindowFlags(Qt::Tool);
    //setLayout(content);
    adjustPosition();

    QObject::connect(button, &QPushButton::clicked, [=](){
        setVisible(!isVisible());
        adjustPosition();
    });
}

void PopUpWidget::adjustPosition() {
    QRect rect = button->geometry();
    QPoint position = parent->mapToGlobal(rect.topRight());
    setGeometry(QRect(position, QSize(rect.width(),200)));
}

QSize PopUpWidget::sizeHint() const
{
    return QSize(0,0);
}