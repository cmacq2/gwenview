// vim: set tabstop=4 shiftwidth=4 noexpandtab
/*
Gwenview - A simple image viewer for KDE
Copyright 2000-2004 Aur�lien G�teau

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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// Qt 
#include <qbuttongroup.h>
#include <qcheckbox.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qmap.h>
#include <qradiobutton.h>
#include <qspinbox.h>
#include <qstylesheet.h>

// KDE
#include <kcolorbutton.h>
#include <kdeversion.h>
#include <kdirsize.h>
#include <kfiledialog.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kio/netaccess.h>
#include <kmessagebox.h>
#include <kurlrequester.h>

// Local 
#include "fileoperation.h"
#include "gvconfigfileoperationspage.h"
#include "gvconfigfullscreenpage.h"
#include "gvconfigimagelistpage.h"
#include "gvconfigimageviewpage.h"
#include "gvconfigmiscpage.h"
#include "gvdocument.h"
#include "gvfilethumbnailview.h"
#include "gvfileviewstack.h"
#include "gvjpegtran.h"
#include "gvmainwindow.h"
#include "gvscrollpixmapview.h"
#include "thumbnailloadjob.h"

#include "gvconfigdialog.moc"

class GVConfigDialogPrivate {
public:
	GVConfigImageViewPage* mImageViewPage;
	GVConfigImageListPage* mImageListPage;
	GVConfigFullScreenPage* mFullScreenPage;
	GVConfigFileOperationsPage* mFileOperationsPage;
	GVConfigMiscPage* mMiscPage;
	GVMainWindow* mMainWindow;
};

template<class T>
T* addConfigPage(KDialogBase* dialog, const QString& name, const char* iconName) {
	T* content=new T;
	QFrame* page=dialog->addPage(name, content->caption(), BarIcon(iconName, 32));
	content->reparent(page, QPoint(0,0));
	QVBoxLayout* layout=new QVBoxLayout(page, 0, KDialog::spacingHint());
	layout->addWidget(content);
	layout->addStretch();
	return content;
	
}

GVConfigDialog::GVConfigDialog(QWidget* parent,GVMainWindow* mainWindow)
: KDialogBase(
	KDialogBase::IconList,
	i18n("Configure"),
	KDialogBase::Ok | KDialogBase::Cancel | KDialogBase::Apply,
	KDialogBase::Ok,
	parent,
	"GVConfigDialog",
	true,
	true)
{
	d=new GVConfigDialogPrivate;
	d->mMainWindow=mainWindow;

	d->mImageListPage      = addConfigPage<GVConfigImageListPage>(this, i18n("Image List"), "view_icon");
	d->mImageViewPage      = addConfigPage<GVConfigImageViewPage>(this, i18n("Image View"), "looknfeel");
	d->mFullScreenPage     = addConfigPage<GVConfigFullScreenPage>(this, i18n("Full Screen"), "window_fullscreen");
	d->mFileOperationsPage = addConfigPage<GVConfigFileOperationsPage>(this, i18n("File Operations"), "folder");
	d->mMiscPage           = addConfigPage<GVConfigMiscPage>(this, i18n("Misc"), "gear");

	GVFileViewStack* fileViewStack=d->mMainWindow->fileViewStack();
	GVScrollPixmapView* pixmapView=d->mMainWindow->pixmapView();
	GVDocument* document=d->mMainWindow->document();

	// Image List tab
	d->mImageListPage->mThumbnailMargin->setValue(fileViewStack->fileThumbnailView()->marginSize());
	d->mImageListPage->mWordWrapFilename->setChecked(fileViewStack->fileThumbnailView()->wordWrapIconText());
	d->mImageListPage->mAutoLoadImage->setChecked(fileViewStack->autoLoadImage());
	d->mImageListPage->mShowDirs->setChecked(fileViewStack->showDirs());
	d->mImageListPage->mShownColor->setColor(fileViewStack->shownColor());
	d->mImageListPage->mStoreThumbnailsInCache->setChecked(ThumbnailLoadJob::storeThumbnailsInCache());
	d->mImageListPage->mAutoDeleteThumbnailCache->setChecked(d->mMainWindow->showAutoDeleteThumbnailCache());

	connect(d->mImageListPage->mCalculateCacheSize,SIGNAL(clicked()),
		this,SLOT(calculateCacheSize()));
	connect(d->mImageListPage->mEmptyCache,SIGNAL(clicked()),
		this,SLOT(emptyCache()));

	// Image View tab
	d->mImageViewPage->mSmoothGroup->setButton(pixmapView->smoothAlgorithm());
	d->mImageViewPage->mDelayedSmoothing->setChecked(pixmapView->delayedSmoothing());
	d->mImageViewPage->mAutoZoomEnlarge->setChecked(pixmapView->enlargeSmallImages());
	d->mImageViewPage->mShowScrollBars->setChecked(pixmapView->showScrollBars());
	d->mImageViewPage->mMouseWheelGroup->setButton(pixmapView->mouseWheelScroll()?1:0);
	
	// Full Screen tab
	d->mFullScreenPage->mOSDModeGroup->setButton(pixmapView->osdMode());
	d->mFullScreenPage->mFreeOutputFormat->setText(pixmapView->freeOutputFormat());
	d->mFullScreenPage->mShowMenuBarInFullScreen->setChecked(d->mMainWindow->showMenuBarInFullScreen());
	d->mFullScreenPage->mShowToolBarInFullScreen->setChecked(d->mMainWindow->showToolBarInFullScreen());
	d->mFullScreenPage->mShowStatusBarInFullScreen->setChecked(d->mMainWindow->showStatusBarInFullScreen());
	d->mFullScreenPage->mShowBusyPtrInFullScreen->setChecked(d->mMainWindow->showBusyPtrInFullScreen());

	// File Operations tab
	d->mFileOperationsPage->mShowCopyDialog->setChecked(FileOperation::confirmCopy());
	d->mFileOperationsPage->mShowMoveDialog->setChecked(FileOperation::confirmMove());

	d->mFileOperationsPage->mDefaultDestDir->setURL(FileOperation::destDir());
	d->mFileOperationsPage->mDefaultDestDir->fileDialog()->setMode(
		static_cast<KFile::Mode>(KFile::Directory | KFile::ExistingOnly | KFile::LocalOnly));
	
	d->mFileOperationsPage->mConfirmBeforeDelete->setChecked(FileOperation::confirmDelete());
	d->mFileOperationsPage->mDeleteGroup->setButton(FileOperation::deleteToTrash()?1:0);
	
	// Misc tab
	d->mMiscPage->mJPEGTran->setURL(GVJPEGTran::programPath());
	d->mMiscPage->mModifiedBehaviorGroup->setButton( int(document->modifiedBehavior()) );
}



GVConfigDialog::~GVConfigDialog() {
	delete d;
}


void GVConfigDialog::slotOk() {
	slotApply();
	accept();
}


#if QT_VERSION<0x030200
// This function emulates int QButtonGroup::selectedId() which does not exist
// in Qt 3.1
inline int buttonGroupSelectedId(const QButtonGroup* group) {
	QButton* button=group->selected();
	if (!button) return -1;
	return group->id(button);
}
#endif

void GVConfigDialog::slotApply() {
	GVFileViewStack* fileViewStack=d->mMainWindow->fileViewStack();
	GVScrollPixmapView* pixmapView=d->mMainWindow->pixmapView();
	GVDocument* document=d->mMainWindow->document();

	// Image List tab
	fileViewStack->fileThumbnailView()->setMarginSize(d->mImageListPage->mThumbnailMargin->value());
	fileViewStack->fileThumbnailView()->setWordWrapIconText(d->mImageListPage->mWordWrapFilename->isChecked());
	fileViewStack->fileThumbnailView()->arrangeItemsInGrid();
	fileViewStack->setAutoLoadImage(d->mImageListPage->mAutoLoadImage->isChecked());
	fileViewStack->setShowDirs(d->mImageListPage->mShowDirs->isChecked());
	fileViewStack->setShownColor(d->mImageListPage->mShownColor->color());
	ThumbnailLoadJob::setStoreThumbnailsInCache(d->mImageListPage->mStoreThumbnailsInCache->isChecked());
	d->mMainWindow->setAutoDeleteThumbnailCache(d->mImageListPage->mAutoDeleteThumbnailCache->isChecked());
	
	// Image View tab
#if QT_VERSION>=0x030200
	int algo=d->mImageViewPage->mSmoothGroup->selectedId();
#else
	int algo=buttonGroupSelectedId(d->mImageViewPage->mSmoothGroup);
#endif
	
	pixmapView->setSmoothAlgorithm( static_cast<GVImageUtils::SmoothAlgorithm>(algo));
	pixmapView->setDelayedSmoothing(d->mImageViewPage->mDelayedSmoothing->isChecked());
	pixmapView->setEnlargeSmallImages(d->mImageViewPage->mAutoZoomEnlarge->isChecked());
	pixmapView->setShowScrollBars(d->mImageViewPage->mShowScrollBars->isChecked());
	pixmapView->setMouseWheelScroll(d->mImageViewPage->mMouseWheelGroup->selected()==d->mImageViewPage->mMouseWheelScroll);
	
	// Full Screen tab
#if QT_VERSION>=0x030200
	int osdMode=d->mFullScreenPage->mOSDModeGroup->selectedId();
#else
	int osdMode=buttonGroupSelectedId(d->mImageViewPage->mOSDModeGroup);
#endif
	pixmapView->setOSDMode( static_cast<GVScrollPixmapView::OSDMode>(osdMode) );
	pixmapView->setFreeOutputFormat( d->mFullScreenPage->mFreeOutputFormat->text() );
	d->mMainWindow->setShowMenuBarInFullScreen( d->mFullScreenPage->mShowMenuBarInFullScreen->isChecked() );
	d->mMainWindow->setShowToolBarInFullScreen( d->mFullScreenPage->mShowToolBarInFullScreen->isChecked() );
	d->mMainWindow->setShowStatusBarInFullScreen( d->mFullScreenPage->mShowStatusBarInFullScreen->isChecked() );
	d->mMainWindow->setShowBusyPtrInFullScreen(d->mFullScreenPage->mShowBusyPtrInFullScreen->isChecked() );

	// File Operations tab
	FileOperation::setConfirmCopy(d->mFileOperationsPage->mShowCopyDialog->isChecked());
	FileOperation::setConfirmMove(d->mFileOperationsPage->mShowMoveDialog->isChecked());
	FileOperation::setDestDir(d->mFileOperationsPage->mDefaultDestDir->url());
	FileOperation::setConfirmDelete(d->mFileOperationsPage->mConfirmBeforeDelete->isChecked());
	FileOperation::setDeleteToTrash(d->mFileOperationsPage->mDeleteGroup->selected()==d->mFileOperationsPage->mDeleteToTrash);

	// Misc tab
	GVJPEGTran::setProgramPath(d->mMiscPage->mJPEGTran->url());
#if QT_VERSION>=0x030200
	int behavior=d->mMiscPage->mModifiedBehaviorGroup->selectedId();
#else
	int behavior=buttonGroupSelectedId(d->mMiscPage->mModifiedBehaviorGroup);
#endif
	document->setModifiedBehavior( static_cast<GVDocument::ModifiedBehavior>(behavior) );
}


void GVConfigDialog::calculateCacheSize() {
	KURL url;
	url.setPath(ThumbnailLoadJob::thumbnailBaseDir());
	unsigned long size=KDirSize::dirSize(url);
	KMessageBox::information( this,i18n("Cache size is %1").arg(KIO::convertSize(size)) );
}


void GVConfigDialog::emptyCache() {
	QString dir=ThumbnailLoadJob::thumbnailBaseDir();

	if (!QFile::exists(dir)) {
		KMessageBox::information( this,i18n("Cache is already empty.") );
		return;
	}

	int response=KMessageBox::questionYesNo(this,
		"<qt>" + i18n("Are you sure you want to empty the thumbnail cache?"
		" This will remove the folder <b>%1</b>.").arg(QStyleSheet::escape(dir)) + "</qt>");

	if (response==KMessageBox::No) return;

	KURL url;
	url.setPath(dir);
#if KDE_VERSION >= 0x30200
	if (KIO::NetAccess::del(url, 0)) {
#else
	if (KIO::NetAccess::del(url)) {
#endif
		KMessageBox::information( this,i18n("Cache emptied.") );
	}
}


void GVConfigDialog::onCacheEmptied(KIO::Job* job) {
	if ( job->error() ) {
		job->showErrorDialog(this);
		return;
	}
	KMessageBox::information( this,i18n("Cache emptied.") );
}

