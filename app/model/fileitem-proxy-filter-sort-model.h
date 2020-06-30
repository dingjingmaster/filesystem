#ifndef FILEITEMPROXYFILTERSORTMODEL_H
#define FILEITEMPROXYFILTERSORTMODEL_H

#include <QColor>
#include <QSortFilterProxyModel>

class FileItem;
class FileItemModel;

class FileItemProxyFilterSortModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    enum FilterFileType
    {
        VIDEO,
        AUDIO,
        PICTURE,
        ALL_TYPE,
        TXT_FILE,
        FILE_FOLDER,
        OTHERS
    };
    Q_ENUM(FilterFileType)

    enum FilterFileModifyTime
    {
        TODAY,
        YEAR_AGO,
        THIS_YEAR,
        THIS_WEEK,
        THIS_MONTH,
        ALL_TIME,
    };
    Q_ENUM(FilterFileModifyTime)

    enum FilterFileSize {
        BIG,
        TINY,
        SMALL,
        LARGE,
        MEDIUM,
        ALL_SIZE,
    };
    Q_ENUM(FilterFileSize)

    const QString cTextType = "text/";
    const QString cImageType = "image/";
    const QString cVideoType = "video/";
    const QString cAudioType = "audio/";
    const QString cFolderType = "inode/directory";

    explicit FileItemProxyFilterSortModel (QObject *parent = nullptr);

    void clearConditions ();
    QStringList getAllFileUris ();
    QModelIndexList getAllFileIndexes ();
    void setShowHidden (bool showHidden);
    void setFolderFirst (bool folderFirst);
    void setUseDefaultNameSortOrder (bool use);
    const QModelIndex indexFromUri (const QString &uri);
    FileItem *itemFromIndex (const QModelIndex &proxyIndex);
    void setSourceModel (QAbstractItemModel *model) override;
    QModelIndex getSourceIndex (const QModelIndex &proxyIndex);
    void setMutipleLabelConditions (QStringList names, QList<QColor> colors);
    void setLabelBlurName (QString blurName = "", bool caseSensitive = false);
    void addFilterCondition (int option, int classify, bool updateNow = false);
    void setFilterConditions (int fileType=0, int modifyTime=0, int fileSize=0);
    void removeFilterCondition (int option, int classify, bool updateNow = false);
    void setFilterLabelConditions (QString name = "", QColor color=Qt::transparent);

public Q_SLOTS:
    void update ();

protected:
    bool lessThan (const QModelIndex &left, const QModelIndex &right) const override;
    bool filterAcceptsRow (int sourceRow, const QModelIndex &sourceParent) const override;

private:
    bool checkFileTypeFilter (QString type) const;
    bool checkFileSizeFilter (quint64 size) const;
    bool startWithChinese (const QString &displayName) const;
    bool checkFileModifyTimeFilter (quint64 modifiedTime) const;

private:
    bool mShowHidden;
    bool mFolderFirst;
    const int ALL_FILE = 0;
    QString mBlurName = "";
    QString mLabelName = "";
    const quint64 K_BASE = 1000;
    bool mCaseSensitive = false;
    bool mUseDefaultNameSortOrder;
    QColor mLabelColor = Qt::transparent;

    int mShowFileType = ALL_FILE;
    int mShowFileSize = ALL_FILE;
    int mShowModifyTime = ALL_FILE;

    QList<int> mFileSizeList;
    QList<int> mFileTypeList;
    QList<int> mModifyTimeList;
    QStringList mShowLabelNames;
    QList<QColor> mShowLabelColors;
};

#endif // FILEITEMPROXYFILTERSORTMODEL_H
