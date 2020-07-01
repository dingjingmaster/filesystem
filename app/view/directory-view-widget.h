#ifndef DIRECTORYVIEWWIDGET_H
#define DIRECTORYVIEWWIDGET_H

#include <QWidget>

class FileItemModel;
class FileItemProxyFilterSortModel;

class DirectoryViewWidget : public QWidget
{
    Q_OBJECT
public:
    explicit DirectoryViewWidget (QWidget *parent = nullptr);
    virtual ~DirectoryViewWidget ();

    virtual int getSortType ();
    const virtual QString viewId ();
    virtual Qt::SortOrder getSortOrder ();
    const virtual QString getDirectoryUri ();
    const virtual QStringList getSelections ();
    const virtual QStringList getAllFileUris ();

    virtual bool supportZoom ();
    virtual int currentZoomLevel ();
    virtual int minimumZoomLevel ();
    virtual int maximumZoomLevel ();

Q_SIGNALS:
    void viewDirectoryChanged ();
    void viewSelectionChanged ();
    void sortTypeChanged (int type);
    void zoomRequest (bool isZoomIn);
    void menuRequest (const QPoint &pos);
    void sortOrderChanged (Qt::SortOrder order);
    void viewDoubleClicked (const QString &uri);
    void updateWindowLocationRequest (const QString &uri);

public Q_SLOTS:
    virtual void repaintView ();
    virtual void invertSelections ();
    virtual void clearIndexWidget ();
    virtual void stopLocationChange ();
    virtual void closeDirectoryView ();
    virtual void beginLocationChange ();
    virtual void setSortType (int sortType);
    virtual void setSortOrder (int sortOrder);
    virtual void editUri (const QString &uri);
    virtual void editUris (const QStringList uris);
    virtual void setCurrentZoomLevel (int zoomLevel);
    virtual void setDirectoryUri (const QString &uri);
    virtual void setCutFiles (const QStringList &uris);
    virtual void scrollToSelection (const QString &uri);
    virtual void setSelections (const QStringList &uris);
    virtual void bindModel (FileItemModel *model, FileItemProxyFilterSortModel *proxyModel);

};

#endif // DIRECTORYVIEWWIDGET_H
