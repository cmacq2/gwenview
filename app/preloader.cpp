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
#include "preloader.moc"

// Qt

// KDE
#include <kdebug.h>

// Local
#include <lib/document/documentfactory.h>

namespace Gwenview {

#undef ENABLE_LOG
#undef LOG
//#define ENABLE_LOG
#ifdef ENABLE_LOG
#define LOG(x) kDebug() << x
#else
#define LOG(x) ;
#endif

struct PreloaderPrivate {
	Document::Ptr mDocument;
	QSize mSize;
};


Preloader::Preloader(QObject* parent)
: QObject(parent)
, d(new PreloaderPrivate) {
}


Preloader::~Preloader() {
	delete d;
}


void Preloader::preload(const KUrl& url, const QSize& size) {
	LOG("url=" << url);
	if (d->mDocument) {
		disconnect(d->mDocument.data(), 0, this, 0);
	}

	d->mDocument = DocumentFactory::instance()->load(url);
	d->mSize = size;
	connect(d->mDocument.data(), SIGNAL(metaDataUpdated()),
		SLOT(doPreload()) );

	if (d->mDocument->size().isValid()) {
		LOG("size is already available");
		doPreload();
	}
}


void Preloader::doPreload() {
	if (!d->mDocument->size().isValid()) {
		LOG("size not available yet");
		return;
	}

	qreal zoom = qMin(
		d->mSize.width() / qreal(d->mDocument->width()),
		d->mSize.height() / qreal(d->mDocument->height())
		);

	if (zoom < Document::MaxDownSampledZoom) {
		LOG("preloading down sampled, zoom=" << zoom);
		d->mDocument->prepareDownSampledImageForZoom(zoom);
	} else {
		LOG("preloading full image");
		d->mDocument->loadFullImage();
	}

	// Forget about the document. Keeping a reference to it would prevent it
	// from being garbage collected.
	disconnect(d->mDocument.data(), 0, this, 0);
	d->mDocument = 0;
}


} // namespace