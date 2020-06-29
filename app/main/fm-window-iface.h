#ifndef FMWINDOWIFACE_H
#define FMWINDOWIFACE_H

#include <memory>
#include <QString>
#include <QStringList>

#include <file/file-info.h>


class FMWindowFactory;
class DirectoryViewContainer;

class FMWindowIface
{
public:
    virtual int getCurrentSortColumn ();
    virtual bool getWindowShowHidden ();
    virtual int currentViewZoomLevel ();
    virtual FMWindowFactory *getFactory ();
    virtual bool currentViewSupportZoom ();
    virtual bool getWindowSortFolderFirst ();
    virtual const QString getCurrentUri () = 0;
    virtual const QString getLastNonSearchUri ();
    virtual Qt::SortOrder getCurrentSortOrder ();
    virtual const QString getCurrentPageViewType ();
    virtual bool getWindowUseDefaultNameSortOrder ();
    virtual DirectoryViewContainer *getCurrentPage ();
    virtual const QStringList getCurrentSelections ();
    virtual const QStringList getCurrentAllFileUris ();
    virtual FMWindowIface *create (const QString &uri) = 0;
    virtual FMWindowIface *create (const QStringList &uris) = 0;
    virtual const QList<std::shared_ptr<FileInfo>> getCurrentSelectionFileInfos ();

public Q_SLOTS:
    virtual void slotRefresh ();
    virtual void slotClearRecord ();
    virtual void slotAdvanceSearch ();
    virtual void slotSetShowHidden ();
    virtual void slotForceStopLoading ();
    virtual void slotSetSortFolderFirst ();
    virtual void slotEditUri (const QString &uri);
    virtual void slotSetUseDefaultNameSortOrder ();
    virtual void slotEditUris (const QStringList &uris);
    virtual void slotAddNewTabs (const QStringList &uris);
    virtual void slotSetCurrentSortColumn (int sortColumn);
    virtual void slotBeginSwitchView (const QString &viewId);
    virtual void slotSetCurrentViewZoomLevel (int zoomLevel);
    virtual void slotOnPreviewPageSwitch (const QString &uri);
    virtual void slotSetCurrentSortOrder (Qt::SortOrder order);
    virtual void slotSetCurrentSelectionUris (const QStringList &uris);
    virtual void slotFilterUpdate (int type_index=0, int time_index=0, int size_index=0);
    virtual void slotGoToUri (const QString &uri, bool addHistory, bool forceUpdate = false);
    virtual void slotSearchFilter (QString target_path, QString keyWord, bool search_file_name, bool search_content);
};

#endif // FMWINDOWIFACE_H
