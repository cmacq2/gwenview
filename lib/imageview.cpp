/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau <aurelien.gateau@free.fr>

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/
#include "imageview.moc"

// Qt
#include <QPainter>
#include <QPaintEvent>
#include <QPointer>
#include <QScrollBar>

// KDE
#include <kdebug.h>

// Local
#include "abstractimageviewtool.h"
#include "imagescaler.h"


namespace Gwenview {


struct ImageViewPrivate {
	const QImage mEmptyImage;

	ImageView* mView;
	QPixmap mBackgroundTexture;
	QWidget* mViewport;
	ImageView::AlphaBackgroundMode mAlphaBackgroundMode;
	QColor mAlphaBackgroundColor;
	const QImage* mImage;
	qreal mZoom;
	bool mZoomToFit;
	QPixmap mCurrentBuffer;
	QPixmap mAlternateBuffer;
	ImageScaler* mScaler;
	QPointer<AbstractImageViewTool> mTool;
	bool mInsideSetZoom;


	void createBackgroundTexture() {
		mBackgroundTexture = QPixmap(32, 32);
		QPainter painter(&mBackgroundTexture);
		painter.fillRect(mBackgroundTexture.rect(), QColor(128, 128, 128));
		QColor light = QColor(192, 192, 192);
		painter.fillRect(0, 0, 16, 16, light);
		painter.fillRect(16, 16, 16, 16, light);
	}


	qreal computeZoomToFit() const {
		int width = mViewport->width();
		int height = mViewport->height();
		qreal zoom = qreal(width) / mImage->width();
		if ( int(mImage->height() * zoom) > height) {
			zoom = qreal(height) / mImage->height();
		}
		return qMin(zoom, 1.0);
	}


	QSize requiredBufferSize() const {
		QSize size;
		qreal zoom;
		if (mZoomToFit) {
			zoom = computeZoomToFit();
		} else {
			zoom = mZoom;
		}

		size = mImage->size() * zoom;
		size = size.boundedTo(mViewport->size());

		return size;
	}


	void createBuffer() {
		QSize size = requiredBufferSize();
		if (!size.isValid()) {
			return;
		}
		mCurrentBuffer = QPixmap(size);
		mAlternateBuffer = QPixmap();
		QColor bgColor = mView->palette().color(mView->backgroundRole());
		mCurrentBuffer.fill(bgColor.rgba());
	}


	void resizeBuffer() {
		QSize size = requiredBufferSize();
		if (size == mCurrentBuffer.size()) {
			return;
		}
		QPixmap oldBuffer = mCurrentBuffer;
		createBuffer();
		{
			QPainter painter(&mCurrentBuffer);
			painter.drawPixmap(0, 0, oldBuffer);
		}
	}


	int hScroll() const {
		if (mZoomToFit) {
			return 0;
		} else {
			return mView->horizontalScrollBar()->value();
		}
	}

	int vScroll() const {
		if (mZoomToFit) {
			return 0;
		} else {
			return mView->verticalScrollBar()->value();
		}
	}

	QRect mapViewportToZoomedImage(const QRect& viewportRect) {
		QPoint offset = mView->imageOffset();
		QRect rect = QRect(
			viewportRect.x() + hScroll() - offset.x(),
			viewportRect.y() + vScroll() - offset.y(),
			viewportRect.width(),
			viewportRect.height()
		);

		return rect;
	}


	void setScalerRegionToVisibleRect() {
		QRect rect = mapViewportToZoomedImage(mViewport->rect());
		mScaler->setDestinationRegion(QRegion(rect));
	}

	// At least gcc 3.4.6 on FreeBSD requires a default constructor.
	ImageViewPrivate() { }
};


ImageView::ImageView(QWidget* parent)
: QAbstractScrollArea(parent)
, d(new ImageViewPrivate)
{
	d->mAlphaBackgroundMode = AlphaBackgroundCheckBoard;
	d->mAlphaBackgroundColor = Qt::black;

	d->mImage = &d->mEmptyImage;
	d->mView = this;
	d->mZoom = 1.;
	d->mZoomToFit = true;
	d->createBackgroundTexture();
	setFrameShape(QFrame::NoFrame);
	setBackgroundRole(QPalette::Base);
	d->mViewport = new QWidget();
	setViewport(d->mViewport);
	d->mViewport->setMouseTracking(true);
	d->mViewport->setAttribute(Qt::WA_OpaquePaintEvent, true);
	horizontalScrollBar()->setSingleStep(16);
	verticalScrollBar()->setSingleStep(16);
	d->mScaler = new ImageScaler(this);
	d->mInsideSetZoom = false;
	connect(d->mScaler, SIGNAL(scaledRect(int, int, const QImage&)), 
		SLOT(updateFromScaler(int, int, const QImage&)) );
}

ImageView::~ImageView() {
	delete d;
}


void ImageView::setAlphaBackgroundMode(AlphaBackgroundMode mode) {
	d->mAlphaBackgroundMode = mode;
	d->mViewport->update();
}


void ImageView::setAlphaBackgroundColor(const QColor& color) {
	d->mAlphaBackgroundColor = color;
	d->mViewport->update();
}


void ImageView::setImage(const QImage* image) {
	if (image) {
		d->mImage = image;
	} else {
		// This little trick makes sure d->mImage always point to a valid
		// image, even if we were given an NULL pointer.
		d->mImage = &d->mEmptyImage;
	}
	d->createBuffer();
	d->mScaler->setImage(d->mImage);
	if (d->mZoomToFit) {
		setZoom(d->computeZoomToFit());
	} else {
		updateScrollBars();
	}
	d->mViewport->update();
}


const QImage* ImageView::image() const {
	if (d->mImage == &d->mEmptyImage) {
		return 0;
	} else {
		return d->mImage;
	}
}


void ImageView::updateImageRect(const QRect& imageRect) {
	QRect viewportRect = mapToViewport(imageRect);
	viewportRect = viewportRect.intersected(d->mViewport->rect());
	if (viewportRect.isEmpty()) {
		return;
	}

	QSize bufferSize = d->requiredBufferSize();
	if (bufferSize != d->mCurrentBuffer.size()) {
		// Since the required buffer size is not the same as our current buffer
		// size, the image must have been resized. Call setImage(), it will
		// take care of resizing the buffer and repainting the whole image.
		setImage(d->mImage);
		return;
	}

	d->setScalerRegionToVisibleRect();
	d->mViewport->update();
}


void ImageView::paintEvent(QPaintEvent* event) {
	QPainter painter(d->mViewport);
	painter.setClipRect(event->rect());
	QPoint offset = imageOffset();

	// Erase pixels around the image
	QRect imageRect(offset, d->mCurrentBuffer.size());
	QRegion emptyRegion = QRegion(event->rect()) - QRegion(imageRect);
	QColor bgColor = palette().color(backgroundRole());
	Q_FOREACH(const QRect& rect, emptyRegion.rects()) {
		painter.fillRect(rect, bgColor);
	}

	if (d->mImage->hasAlphaChannel()) {
		if (d->mAlphaBackgroundMode == AlphaBackgroundCheckBoard) {
			painter.drawTiledPixmap(imageRect, d->mBackgroundTexture
				// This option makes the background scroll with the image, like GIMP
				// and others do. I think having a fixed background makes it easier to
				// distinguish transparent parts, so I comment it out for now.

				//, QPoint(d->hScroll() % d->mBackgroundTexture.width(), d->vScroll() % d->mBackgroundTexture.height())
				);
		} else {
			painter.fillRect(imageRect, d->mAlphaBackgroundColor);
		}
	}
	painter.drawPixmap(offset, d->mCurrentBuffer);

	if (d->mTool) {
		d->mTool->paint(&painter);
	}
}

void ImageView::resizeEvent(QResizeEvent*) {
	if (d->mZoomToFit) {
		setZoom(d->computeZoomToFit());
		// Make sure one can't use mousewheel in zoom-to-fit mode
		horizontalScrollBar()->setRange(0, 0);
		verticalScrollBar()->setRange(0, 0);
	} else {
		d->resizeBuffer();
		updateScrollBars();
		d->setScalerRegionToVisibleRect();
	}
}

QPoint ImageView::imageOffset() const {
	int left = qMax( (d->mViewport->width() - d->mCurrentBuffer.width()) / 2, 0);
	int top = qMax( (d->mViewport->height() - d->mCurrentBuffer.height()) / 2, 0);

	return QPoint(left, top);
}

void ImageView::setZoom(qreal zoom) {
	QPoint center = QPoint(d->mViewport->width() / 2, d->mViewport->height() / 2);
	setZoom(zoom, center);
}


void ImageView::setZoom(qreal zoom, const QPoint& center) {
	qreal oldZoom = d->mZoom;
	if (qAbs(zoom - oldZoom) < 0.001) {
		return;
	}

	d->mZoom = zoom;
	if (d->mImage->isNull()) {
		return;
	}

	// If we zoom more than twice, then assume the user wants to see the real
	// pixels, for example to fine tune a crop operation
	if (d->mZoom < 2.) {
		d->mScaler->setTransformationMode(Qt::SmoothTransformation);
	} else {
		d->mScaler->setTransformationMode(Qt::FastTransformation);
	}

	// Get offset *before* resizing the buffer, otherwise we get the new offset
	QPoint oldOffset = imageOffset();
	d->createBuffer();
	if (d->mZoom < oldZoom && (d->mCurrentBuffer.width() < d->mViewport->width() || d->mCurrentBuffer.height() < d->mViewport->height())) {
		// Trigger an update to erase borders
		d->mViewport->update();
	}

	d->mInsideSetZoom = true;

	/*
	We want to keep the point at viewport coordinates "center" at the same
	position after zooming. The coordinates of this point in image coordinates
	can be expressed like this:

	                      oldScroll + center
	imagePointAtOldZoom = ------------------
	                           oldZoom

	                   scroll + center
	imagePointAtZoom = ---------------
	                        zoom

	So we want:

	    imagePointAtOldZoom = imagePointAtZoom

	    oldScroll + center   scroll + center
	<=> ------------------ = ---------------
	          oldZoom             zoom

	              zoom
	<=> scroll = ------- (oldScroll + center) - center
	             oldZoom
	*/

	/*
	Compute oldScroll
	It's useless to take the new offset in consideration because if a direction
	of the new offset is not 0, we won't be able to center on a specific point
	in that direction.
	*/
	QPointF oldScroll = QPointF(d->hScroll(), d->vScroll()) - oldOffset;

	QPointF scroll = (zoom / oldZoom) * (oldScroll + center) - center;

	updateScrollBars();
	horizontalScrollBar()->setValue(int(scroll.x()));
	verticalScrollBar()->setValue(int(scroll.y()));
	d->mInsideSetZoom = false;

	d->mScaler->setZoom(d->mZoom);
	d->setScalerRegionToVisibleRect();
	emit zoomChanged();
}

qreal ImageView::zoom() const {
	return d->mZoom;
}

bool ImageView::zoomToFit() const {
	return d->mZoomToFit;
}

void ImageView::setZoomToFit(bool on) {
	d->mZoomToFit = on;
	if (d->mZoomToFit) {
		setZoom(d->computeZoomToFit());
	}
}

void ImageView::updateScrollBars() {
	if (d->mZoomToFit) {
		setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
		setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
		return;
	}
	setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

	int max;
	int width = d->mViewport->width();
	int height = d->mViewport->height();

	max = qMax(0, int(d->mImage->width() * d->mZoom) - width);
	horizontalScrollBar()->setRange(0, max);
	horizontalScrollBar()->setPageStep(width);

	max = qMax(0, int(d->mImage->height() * d->mZoom) - height);
	verticalScrollBar()->setRange(0, max);
	verticalScrollBar()->setPageStep(height);
}


void ImageView::scrollContentsBy(int dx, int dy) {
	if (d->mInsideSetZoom) {
		// Do not scroll anything: since we are zooming the whole viewport will
		// eventually be repainted
		return;
	}
	// Scroll existing
	{
		if (d->mAlternateBuffer.isNull()) {
			d->mAlternateBuffer = QPixmap(d->mCurrentBuffer.size());
		}
		QPainter painter(&d->mAlternateBuffer);
		painter.fillRect(d->mAlternateBuffer.rect(), Qt::black);
		painter.drawPixmap(dx, dy, d->mCurrentBuffer);
	}
	qSwap(d->mCurrentBuffer, d->mAlternateBuffer);

	// Scale missing parts
	QRegion region;
	int posX = d->hScroll();
	int posY = d->vScroll();
	int width = d->mViewport->width();
	int height = d->mViewport->height();

	QRect rect;
	if (dx > 0) {
		rect = QRect(posX, posY, dx, height);
	} else {
		rect = QRect(posX + width + dx, posY, -dx, height);
	}
	region |= rect;

	if (dy > 0) {
		rect = QRect(posX, posY, width, dy);
	} else {
		rect = QRect(posX, posY + height + dy, width, -dy);
	}
	region |= rect;

	d->mScaler->setDestinationRegion(region);
	d->mViewport->update();
}


void ImageView::updateFromScaler(int left, int top, const QImage& image) {
	left -= d->hScroll();
	top -= d->vScroll();

	{
		QPainter painter(&d->mCurrentBuffer);
		painter.setCompositionMode(QPainter::CompositionMode_Source);
		painter.drawImage(left, top, image);
		// Debug rects
		//painter.setPen(Qt::red);
		//painter.drawRect(left, top, image.width() - 1, image.height() - 1);
	}
	QPoint offset = imageOffset();
	d->mViewport->update(
		offset.x() + left,
		offset.y() + top,
		image.width(),
		image.height());
}


void ImageView::setCurrentTool(AbstractImageViewTool* tool) {
	if (d->mTool) {
		d->mTool->toolDeactivated();
	}
	d->mTool = tool;
	if (d->mTool) {
		d->mTool->toolActivated();
	}
	d->mViewport->update();
}


AbstractImageViewTool* ImageView::currentTool() const {
	return d->mTool;
}


QPoint ImageView::mapToViewport(const QPoint& src) {
	QPoint dst(int(src.x() * d->mZoom), int(src.y() * d->mZoom));

	dst += imageOffset();

	dst.rx() -= d->hScroll();
	dst.ry() -= d->vScroll();

	return dst;
}


QRect ImageView::mapToViewport(const QRect& src) {
	QRect dst(
		mapToViewport(src.topLeft()),
		mapToViewport(src.bottomRight())
	);
	return dst;
}


QPoint ImageView::mapToImage(const QPoint& src) {
	QPoint dst = src;
	
	dst.rx() += d->hScroll();
	dst.ry() += d->vScroll();

	dst -= imageOffset();

	return QPoint(int(dst.x() / d->mZoom), int(dst.y() / d->mZoom));
}


QRect ImageView::mapToImage(const QRect& src) {
	QRect dst(
		mapToImage(src.topLeft()),
		mapToImage(src.bottomRight())
	);
	return dst;
}


void ImageView::mousePressEvent(QMouseEvent* event) {
	if (d->mTool) {
		d->mTool->mousePressEvent(event);
	}
}


void ImageView::mouseMoveEvent(QMouseEvent* event) {
	if (d->mTool) {
		d->mTool->mouseMoveEvent(event);
	}
}


void ImageView::mouseReleaseEvent(QMouseEvent* event) {
	if (d->mTool) {
		d->mTool->mouseReleaseEvent(event);
	}
}


void ImageView::wheelEvent(QWheelEvent* event) {
	if (d->mTool) {
		d->mTool->wheelEvent(event);
	}
}


void ImageView::keyPressEvent(QKeyEvent* event) {
	if (d->mTool) {
		d->mTool->keyPressEvent(event);
	}
	QAbstractScrollArea::keyPressEvent(event);
}


void ImageView::keyReleaseEvent(QKeyEvent* event) {
	if (d->mTool) {
		d->mTool->keyReleaseEvent(event);
	}
	QAbstractScrollArea::keyReleaseEvent(event);
}


} // namespace
