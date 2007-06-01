// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau

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
#ifndef FULLSCREENBAR_H
#define FULLSCREENBAR_H

#include "gwenviewlib_export.h"

// Qt

// KDE
#include <ktoolbar.h>

// Local

class QEvent;

namespace Gwenview {


class FullScreenBarPrivate;
class GWENVIEWLIB_EXPORT FullScreenBar : public KToolBar {
	Q_OBJECT
public:
	FullScreenBar(QWidget* parent);
	~FullScreenBar();

	void setActivated(bool);

public Q_SLOTS:
	void slideIn();
	void slideOut();

private Q_SLOTS:
	void autoHide();
	void moveBar(qreal);
	void slotTimeLineFinished();

protected:
	virtual bool eventFilter(QObject*, QEvent*);

private:
	FullScreenBarPrivate* const d;
};


} // namespace

#endif /* FULLSCREENBAR_H */
