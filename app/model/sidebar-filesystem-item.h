#ifndef SIDEBARFILESYSTEMITEM_H
#define SIDEBARFILESYSTEMITEM_H

#include "sidebar-abstract-item.h"

#include <memory>

class FileWatcher;

class SideBarFileSystemItem : public SideBarAbstractItem
{
    Q_OBJECT
public:
    explicit SideBarFileSystemItem (QString uri, SideBarFileSystemItem *parentItem, SideBarModel *model, QObject *parent = nullptr);

    Type type () override;
    QString uri () override;
    bool isMounted () override;
    QString iconName () override;
    bool hasChildren () override;
    bool isEjectable () override;
    bool isMountable () override;
    bool isRemoveable () override;
    QString displayName () override;
    QModelIndex lastColumnIndex () override;
    SideBarAbstractItem *parent () override;
    QModelIndex firstColumnIndex () override;

public Q_SLOTS:
    void eject () override;
    void format () override;
    void unmount () override;
    void onUpdated () override;
    void findChildren () override;
    void clearChildren () override;
    void findChildrenAsync () override;

protected:
    void initWatcher ();
    void stopWatcher ();
    void startWatcher ();

private:
    QString mUnixDevice;
    QString mVolumeName;
    QString mUri = nullptr;
    bool mIsMounted = false;
    bool mIsEjectable = false;
    bool mIsMountable = false;
    bool mIsRootChild = false;
    bool mIsRemoveable = false;
    QString mIconName = nullptr;
    QString mDisplayName = nullptr;
    SideBarFileSystemItem *mParent = nullptr;
    std::shared_ptr<FileWatcher> mWatcher = nullptr;
};

#endif // SIDEBARFILESYSTEMITEM_H
