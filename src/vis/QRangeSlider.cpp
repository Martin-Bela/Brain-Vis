#include "QRangeSlider.hpp"

#include "visUtility.hpp"

#include <QPainter>
#include <QMouseEvent>

QRangeSlider::QRangeSlider(QWidget* parent) : QWidget(parent) {
	_lowValue = _minimum;
	_highValue = _maximum;

	setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
}

void QRangeSlider::setMinimum(const unsigned int minimum) {
	if (_minimum != minimum) {
		_minimum = minimum;
		if (_minimum >= _maximum) {
			setMaximum(_minimum + 1);
			setHighValue(_maximum);
			setLowValue(_minimum);
		}
		else if (_minimum >= _highValue) {
			setHighValue(_minimum + 1);
			setLowValue(_minimum);
		}
		else if (_minimum > _lowValue) {
			setLowValue(_minimum);
		}

		update();
		emit(minimumChange(_minimum));
		emit(rangeChange(_minimum, _maximum));
	}
}

void QRangeSlider::setMaximum(const unsigned int maximum)
{
	if (maximum != _maximum) {
		_maximum = maximum;
		if (_maximum <= _minimum) {
			setMinimum(_maximum++);
			setHighValue(_maximum);
			setLowValue(_minimum);
		}
		else if (_maximum <= _lowValue) {
			setHighValue(_maximum);
			setLowValue(_highValue - 1);
		}
		else if (_maximum < _highValue) {
			setHighValue(_maximum);
		}

		update();
		emit(maximumChange(_maximum));
		emit(rangeChange(_minimum, _maximum));
	}
}

unsigned int QRangeSlider::lowValue() const {
	return _lowValue;
}

void QRangeSlider::setLowValue(const unsigned int lowValue) {
	if (_lowValue != lowValue) {
		_lowValue = lowValue;
		if (_lowValue >= _maximum) {
			_lowValue = _maximum - 1;
		}
		else if (_lowValue < _minimum) {
			_lowValue = _minimum;
		}
		
		if (_lowValue >= _highValue) {
			setHighValue(_lowValue + 1);
		}

		update();
		emit(lowValueChange(_lowValue));
		emit(valueChange(_lowValue, _highValue));
	}
}

unsigned int QRangeSlider::highValue() const
{
	return _highValue;
}

void QRangeSlider::setHighValue(const unsigned int highValue) {
	if (_highValue != highValue) {
		_highValue = highValue;
		
		if (_highValue > _maximum) {
			_highValue = _maximum;
		}
		else if (_highValue <= _minimum) {
			_highValue = _minimum + 1;
		}
		
		if (_highValue <= _lowValue) {
			setLowValue(_highValue - 1);
		}

		update();
		emit(highValueChange(_highValue));
		emit(valueChange(_lowValue, _highValue));
	}
}

unsigned int QRangeSlider::step() const {
	return _step;
}

void QRangeSlider::setStep(const unsigned int step) {
	_step = step;
}

void QRangeSlider::setRange(const unsigned int minimum, const unsigned int maximum) {
	setMinimum(minimum);
	setMaximum(maximum);
}

QSize QRangeSlider::sizeHint() const {
	return minimumSizeHint();
	//return QSize(100 * HANDLE_SIZE + 2 * PADDING, 2 * HANDLE_SIZE + 2 * PADDING);
}

QSize QRangeSlider::minimumSizeHint() const {
	return QSize(2 * PADDING, 2 * (PADDING + HANDLE_RADIUS));
}

void QRangeSlider::mousePressEvent(QMouseEvent* e) {
	float mouseY = e->position().y();

	auto dist_low = std::abs(valueToYCoord(_lowValue) - mouseY);
	auto dist_high = std::abs(valueToYCoord(_highValue) - mouseY);

	if (dist_low < dist_high) {
		if (dist_low <= HANDLE_RADIUS) {
			_movingHandle = lowHandle;
			return;
		}
	}
	else {
		if (dist_high <= HANDLE_RADIUS) {
			_movingHandle = highHandle;
			return;
		}
	}
	_movingHandle = noHandle;
}

void QRangeSlider::mouseReleaseEvent(QMouseEvent* e) {
	Q_UNUSED(e);
	_movingHandle = noHandle;
}

void QRangeSlider::mouseMoveEvent(QMouseEvent* e) {
	int slider_height = height() - 2 * PADDING;

	float mouseY = e->position().y() - PADDING;

	int mouseValue = std::lerp(_minimum, _maximum, 1 - mouseY / slider_height);
	mouseValue = std::clamp(mouseValue, (int)_minimum, (int)_maximum);

	if (_movingHandle == lowHandle) {
		setLowValue(mouseValue);
	}
	else if (_movingHandle == highHandle) {
		setHighValue(mouseValue);
	}
}

int QRangeSlider::valueToYCoord(int value) const {
	int slider_height = height() - 2 * PADDING;
	double pos_lower = 1 - map_to_unit_range(_minimum, _maximum, value);
	return round(slider_height * pos_lower) + PADDING;
}

void QRangeSlider::paintEvent(QPaintEvent* event) {
	Q_UNUSED(event);

	ColorMixer colorMixer(QColor::fromRgbF(0, 0, 1), QColor::fromRgbF(0.7, 0.7, 0.7), QColor::fromRgbF(1, 0, 0), 0.5);

	QLinearGradient linearGrad(0, 0, 0, height());
	linearGrad.setColorAt(0, colorMixer.getColor(1));
	linearGrad.setColorAt(0.5, colorMixer.getColor(0.5));
	linearGrad.setColorAt(1, colorMixer.getColor(0));

	int slider_height = height() - 2 * PADDING;

	QPainter painter(this);
	painter.setRenderHint(QPainter::RenderHint::Antialiasing);

	// Draw background
	//painter.setPen(QPen(Qt::GlobalColor::darkGray, 0.8));
	painter.setBrush(QBrush(linearGrad));
	painter.drawRoundedRect((width() - SLIDER_WIDTH) / 2,
		PADDING,
		SLIDER_WIDTH,
		slider_height,
		2,
		2);

	// Draw lower handle
	painter.setBrush(QBrush(QColor(Qt::GlobalColor::white)));

	painter.drawEllipse(QPoint(width() / 2, valueToYCoord(_lowValue)), HANDLE_RADIUS, HANDLE_RADIUS);
	painter.drawEllipse(QPoint(width() / 2, valueToYCoord(_highValue)), HANDLE_RADIUS, HANDLE_RADIUS);

	painter.end();
}
