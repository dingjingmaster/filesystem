#ifndef BOOKMARKMANAGER_H
#define BOOKMARKMANAGER_H

#include <QMutex>
#include <QObject>
#include <QSettings>


class BookMarkManager : public QObject
{
    Q_OBJECT
public:
    bool isLoaded();
    const QStringList getCurrentUris ();
    static BookMarkManager *getInstance ();

Q_SIGNALS:
    void urisLoaded ();
    void bookMarkAdded (const QString &uri, bool successed);
    void bookMarkRemoved (const QString &uri, bool successed);

public Q_SLOTS:
    void addBookMark (const QString &uri);
    void removeBookMark (const QString &uri);

private:
    explicit BookMarkManager (QObject *parent = nullptr);
    ~BookMarkManager ();

private:
    QMutex mMutex;
    QStringList mUris;
    bool mIsLoaded = false;
    QSettings *mBookMark = nullptr;
};


#endif // BOOKMARKMANAGER_H
