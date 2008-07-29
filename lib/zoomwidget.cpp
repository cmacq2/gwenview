// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview: an image viewer
Copyright 2008 Aurélien Gâteau <aurelien.gateau@free.fr>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Cambridge, MA 02110-1301, USA.

*/
// Self
#include "zoomwidget.moc"

// stdc++
#include <cmath>

// Qt
#include <QAction>
#include <QApplication>
#include <QLabel>
#include <QHBoxLayout>
#include <QSlider>

// KDE

// Local
#include "signalblocker.h"
#include "statusbartoolbutton.h"

namespace Gwenview {


static const qreal MAGIC_K = 1.04;
static const qreal MAGIC_OFFSET = 16.;
static const qreal PRECISION = 100.;
inline int sliderValueForZoom(qreal zoom) {
	return int( PRECISION * (log(zoom) / log(MAGIC_K) + MAGIC_OFFSET) );
}


inline qreal zoomForSliderValue(int sliderValue) {
	return pow(MAGIC_K, sliderValue / PRECISION - MAGIC_OFFSET);
}




struct ZoomWidgetPrivate {
	ZoomWidget* that;

	StatusBarToolButton* mZoomToFitButton;
	StatusBarToolButton* mActualSizeButton;
	QLabel* mZoomLabel;
	QSlider* mZoomSlider;
	QAction* mZoomToFitAction;
	QAction* mActualSizeAction;

	void emitZoomChanged() {
		// Use QSlider::sliderPosition(), not QSlider::value() because when we are
		// called from slotZoomSliderActionTriggered(), QSlider::value() has not
		// been updated yet.
		qreal zoom = zoomForSliderValue(mZoomSlider->sliderPosition());
		emit that->zoomChanged(zoom);
	}
};


ZoomWidget::ZoomWidget(QWidget* parent)
: QFrame(parent)
, d(new ZoomWidgetPrivate) {
	d->that = this;

	setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);

	d->mZoomToFitButton = new StatusBarToolButton;
	d->mActualSizeButton = new StatusBarToolButton;

	if (QApplication::isLeftToRight()) {
		d->mZoomToFitButton->setGroupPosition(StatusBarToolButton::GroupLeft);
		d->mActualSizeButton->setGroupPosition(StatusBarToolButton::GroupRight);
	} else {
		d->mActualSizeButton->setGroupPosition(StatusBarToolButton::GroupLeft);
		d->mZoomToFitButton->setGroupPosition(StatusBarToolButton::GroupRight);
	}

	d->mZoomLabel = new QLabel;
	d->mZoomLabel->setFixedWidth(d->mZoomLabel->fontMetrics().width(" 1000% "));
	d->mZoomLabel->setAlignment(Qt::AlignCenter);

	d->mZoomSlider = new QSlider;
	d->mZoomSlider->setOrientation(Qt::Horizontal);
	d->mZoomSlider->setMinimumWidth(150);
	d->mZoomSlider->setSingleStep(int(PRECISION));
	d->mZoomSlider->setPageStep(3 * d->mZoomSlider->singleStep());
	connect(d->mZoomSlider, SIGNAL(rangeChanged(int, int)), SLOT(slotZoomSliderRangeChanged()) );
	connect(d->mZoomSlider, SIGNAL(actionTriggered(int)), SLOT(slotZoomSliderActionTriggered()) );

	// Layout
	QHBoxLayout* layout = new QHBoxLayout(this);
	layout->setMargin(0);
	layout->setSpacing(0);
	layout->addWidget(d->mZoomToFitButton);
	layout->addWidget(d->mActualSizeButton);
	layout->addWidget(d->mZoomSlider);
	layout->addWidget(d->mZoomLabel);
}


ZoomWidget::~ZoomWidget() {
	delete d;
}


void ZoomWidget::setActions(QAction* zoomToFitAction, QAction* actualSizeAction) {
	d->mZoomToFitAction = zoomToFitAction;
	d->mActualSizeAction = actualSizeAction;

	d->mZoomToFitButton->setDefaultAction(zoomToFitAction);
	d->mActualSizeButton->setDefaultAction(actualSizeAction);

	// Adjust sizes
	int width = qMax(d->mZoomToFitButton->sizeHint().width(), d->mActualSizeButton->sizeHint().width());
	d->mZoomToFitButton->setFixedWidth(width);
	d->mActualSizeButton->setFixedWidth(width);
}


QSlider* ZoomWidget::slider() const {
	return d->mZoomSlider;
}


QLabel* ZoomWidget::label() const {
	return d->mZoomLabel;
}


void ZoomWidget::slotZoomSliderRangeChanged() {
	if (d->mZoomToFitAction->isChecked()) {
		SignalBlocker blocker(d->mZoomSlider);
		d->mZoomSlider->setValue(d->mZoomSlider->minimum());
	} else {
		d->emitZoomChanged();
	}
}


void ZoomWidget::slotZoomSliderActionTriggered() {
	// The slider value changed because of the user (not because of range
	// changes). In this case disable zoom and apply slider value.
	d->emitZoomChanged();
}


void ZoomWidget::setZoom(qreal zoom) {
	SignalBlocker blocker(d->mZoomSlider);
	int value = sliderValueForZoom(zoom);
	d->mZoomSlider->setValue(value);
}


void ZoomWidget::setZoomRange(qreal minZoom, qreal maxZoom) {
	d->mZoomSlider->setRange(
		sliderValueForZoom(minZoom),
		sliderValueForZoom(maxZoom));
}


} // namespace