#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>

#include "magmaColormap.hpp"

class QPushButton;

class PopUpWidget : public QWidget {
public:
    explicit PopUpWidget(QWidget* parent, QPushButton* button);

    QSize sizeHint() const;

    virtual void adjustPosition();
 
protected:
    QWidget* parent;
    QPushButton* button;
    QLayout* content = nullptr;
};

class PropertiesPopUp : public PopUpWidget {
public:
    PropertiesPopUp(QWidget* parent, QPushButton* button);
};


class HistogramPopUp : public PopUpWidget {
    QPixmap pixmap;
    
public:
    HistogramPopUp(QWidget* parent, QPushButton* button);

    void adjustPosition() override;
};