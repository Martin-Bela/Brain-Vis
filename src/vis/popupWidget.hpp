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
    //void resizeEvent(QResizeEvent *event) override;
    //void moveEvent(QMoveEvent *event) override;
protected:
    QWidget* parent;
    QPushButton* button;
    QLayout* content = nullptr;
};


inline std::string_view propertiesTipText = R"(
<b>Fired</b> Boolean: Did the neuron fire within the last sample step<br/>
<b>Fired Fraction</b>	In Percent: Number of firings since the last sampling<br/>
<b>x</b>	Electric Activity<br/>
<b>Secondary Variable</b>	Inhibition variable used for the firing model of Izhikevich<br/>
<b>Calcium</b>	Current calcium level<br/>
<b>Target Calcium</b>	Target calcium level<br/>
<b>Synaptic Input</b>	Input electrical activity<br/>
<b>Background Activity</b>	Background noise electric activity input<br/>
<b>Grown Axons</b>	Number of currently grown axonal boutons<br/>
<b>Connected Axons</b>	Number of current outgoing connections<br/>
<b>Grown Excitatory Dendrites</b>	Number of currently grown dendrite spines for excitatory connections<br/>
<b>Connected Excitatory Dendrites</b>	Number of incoming excitatory connections
)";

class PropertiesPopUp : public PopUpWidget {
public:
    PropertiesPopUp(QWidget* parent, QPushButton* button) :
        PopUpWidget(parent, button)
    {
        setWindowTitle("Neuron Properties");

        auto* layout = new QVBoxLayout(this);
        
        auto* label = new QLabel();
        layout->addWidget(label);

        
        label->setText(propertiesTipText.data());

        setLayout(layout);
    }
};


class HistogramPopUp : public PopUpWidget {
    QPixmap pixmap;
    
public:
    HistogramPopUp(QWidget* parent, QPushButton* button);

    void adjustPosition() override;
};