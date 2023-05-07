#include <QWidget>

class QRangeSlider : public QWidget
{

    Q_OBJECT

public:
    QRangeSlider(QWidget *parent = nullptr);
    unsigned int minimum() const { return _minimum; };
    unsigned int maximum() const { return _maximum; };
    unsigned int lowValue() const;
    unsigned int highValue() const;
    unsigned int step() const;
    void setStep(const unsigned int step);
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

public slots:
    void setMinimum(const unsigned int minimum);
    void setMaximum(const unsigned int maximum);
    void setRange(const unsigned int minimum, const unsigned int maximum);

    void setLowValue(const unsigned int lowValue);
    void setHighValue(const unsigned int highValue);


signals:
    void minimumChange(unsigned int minimum);
    void maximumChange(unsigned int maximum);
    void rangeChange(unsigned int minimum, unsigned int maximum);

    void lowValueChange(unsigned int lowValue);
    void highValueChange(unsigned int highValue);
    void valueChange(unsigned int lowValue, unsigned int highValue);


private:
    void mousePressEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);
    void mouseMoveEvent(QMouseEvent *e);
    void paintEvent(QPaintEvent *event);

    int valueToYCoord(int value) const;

    /* Minimum range value */
    unsigned int _minimum = 0;

    /* Maximum range value */
    unsigned int _maximum = 100;

    /* Step value */
    unsigned int _step = 1;

    /* Value of low handle */
    unsigned int _lowValue;

    /* Value of high handle */
    unsigned int _highValue;


    enum Handle{ noHandle, lowHandle, highHandle};
    Handle _movingHandle = noHandle;

    static constexpr unsigned int SLIDER_WIDTH = 5;
    static constexpr unsigned int HANDLE_RADIUS = 6;
    static constexpr unsigned int PADDING = HANDLE_RADIUS + 2;
};
