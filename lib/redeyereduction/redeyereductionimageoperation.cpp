// vim: set tabstop=4 shiftwidth=4 noexpandtab:
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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

*/
// Self
#include "redeyereductionimageoperation.h"

// Stdc
#include <math.h>

// Qt
#include <QImage>
#include <QPainter>

// KDE
#include <kdebug.h>
#include <klocale.h>

// Local
#include "document/document.h"
#include "document/abstractdocumenteditor.h"

namespace Gwenview {


struct RedEyeReductionImageOperationPrivate {
	QRect mRect;
	QImage mOriginalImage;
};


RedEyeReductionImageOperation::RedEyeReductionImageOperation(const QRect& rect)
: d(new RedEyeReductionImageOperationPrivate) {
	d->mRect = rect;
	setText(i18n("RedEyeReduction"));
}


RedEyeReductionImageOperation::~RedEyeReductionImageOperation() {
	delete d;
}


void RedEyeReductionImageOperation::redo() {
	QImage img = document()->image();
	d->mOriginalImage = img.copy(d->mRect);
	apply(&img, d->mRect);
	if (!document()->editor()) {
		kWarning() << "!document->editor()";
		return;
	}
	document()->editor()->setImage(img);
}


void RedEyeReductionImageOperation::undo() {
	if (!document()->editor()) {
		kWarning() << "!document->editor()";
		return;
	}
	QImage img = document()->image();
	{
		QPainter painter(&img);
		painter.setCompositionMode(QPainter::CompositionMode_Source);
		painter.drawImage(d->mRect.topLeft(), d->mOriginalImage);
	}
	document()->editor()->setImage(img);
}


void RedEyeReductionImageOperation::apply(QImage* img, const QRect& rect) {
	const qreal radius = rect.width() / 2;
	uchar* line = img->scanLine(rect.top()) + rect.left() * 4;
	const qreal shadeRadius = qMin(radius * 0.7, radius - 1);
	const int width = rect.width();
	const int height = rect.height();
	for (int y = 0; y < height; ++y, line += img->bytesPerLine()) {
		QRgb* ptr = (QRgb*)line;
		for (int x = 0; x < width; ++x, ++ptr) {
			const qreal currentRadius = sqrt(pow(y - radius, 2) + pow(x - radius, 2));
			qreal k;
			if (currentRadius > radius) {
				continue;
			}
			if (currentRadius > shadeRadius) {
				k = (radius - currentRadius) / (radius - shadeRadius);
			} else {
				k = 1;
			}

			QColor src(*ptr);
			QColor dst = src;
			dst.setRed(( src.green() + src.blue() ) / 2);
			int h, s, v, a;
			dst.getHsv(&h, &s, &v, &a);
			dst.setHsv(h, 0, v, a);

			dst.setRed(int((1 -k) * src.red() + k * dst.red()));
			dst.setGreen(int((1 -k) * src.green() + k * dst.green()));
			dst.setBlue(int((1 -k) * src.blue() + k * dst.blue()));

			*ptr = dst.rgba();
		}
	}
}


} // namespace