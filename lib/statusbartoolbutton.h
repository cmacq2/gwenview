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
#ifndef STATUSBARTOOLBUTTON_H
#define STATUSBARTOOLBUTTON_H

#include <lib/gwenviewlib_export.h>

// Qt
#include <QToolButton>

namespace Gwenview {

/**
 * A thin tool button which can be grouped with another and look like one solid
 * bar:
 *
 * ( button1 | button2 )
 */
class GWENVIEWLIB_EXPORT StatusBarToolButton : public QToolButton {
	Q_OBJECT
public:
	enum GroupPosition {
		GroupLeft,
		GroupRight,
		NotGrouped
	};

	StatusBarToolButton(QWidget* parent=0);

	virtual QSize minimumSizeHint() const;

	virtual QSize sizeHint() const;

	void setGroupPosition(StatusBarToolButton::GroupPosition groupPosition);

protected:
	virtual void paintEvent(QPaintEvent* event);

private:
	GroupPosition mGroupPosition;
};

} // namespace

#endif /* STATUSBARTOOLBUTTON_H */