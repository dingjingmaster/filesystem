#ifndef PATHBARMODEL_H
#define PATHBARMODEL_H

#include <QObject>
#include <QStringListModel>

class PathBarModel : public QStringListModel
{
    Q_OBJECT
public:
    explicit PathBarModel (QObject *parent = nullptr);

    QString currentDirUri ();
    QString findDisplayName (const QString &uri);

Q_SIGNALS:
    void updated ();

public Q_SLOTS:
    void setRootUri (const QString &uri, bool force = false);
    void setRootPath (const QString &path, bool force = false);

private:
    QString mCurrentUri = nullptr;
    QHash<QString, QString> mUriDisplayNameHash;
};

#endif // PATHBARMODEL_H
