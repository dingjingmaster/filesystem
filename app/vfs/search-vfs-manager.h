#ifndef SEARCHVFSMANAGER_H
#define SEARCHVFSMANAGER_H

#include <QHash>
#include <QMutex>
#include <QObject>

class SearchVFSManager : public QObject
{
    Q_OBJECT
public:
    static SearchVFSManager* getInstance();

public Q_SLOTS:
    void clearHistory();
    bool hasHistory(const QString &searchUri);
    void clearHistoryOne(const QString &searchUri);
    QStringList getHistroyResults(const QString &searchUri);
    void addHistory(const QString &searchUri, const QStringList &results);

private:
    QMutex mMutex;
    QHash<QString, QStringList> mSearchDirResultsHash;

    explicit SearchVFSManager(QObject* parent = nullptr);
    ~SearchVFSManager();
};

#endif // SEARCHVFSMANAGER_H
