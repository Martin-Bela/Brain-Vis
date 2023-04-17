#pragma once
#include <QWidget>
#include <QLabel>


class TheWatch : public QWidget
{
    Q_OBJECT
    Q_ENUMS(Priority)

public:
    TheWatch(QWidget *parent = nullptr);

private :
    QLabel *lblWatch;

private slots:
    void writeUpdatedTime();

signals:
    void timeUpdated(QString timetext);

};