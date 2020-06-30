#ifndef PATHCOMPLETE_H
#define PATHCOMPLETE_H

#include <QCompleter>



class PathComplete : public QCompleter
{
    Q_OBJECT
public:
    explicit PathComplete (QObject *parent = nullptr);
    explicit PathComplete (QAbstractItemModel *model, QObject *parent = nullptr);

protected:
    QStringList splitPath (const QString &path) const override;
    QString pathFromIndex (const QModelIndex &index) const override;
};

#endif // PATHCOMPLETE_H
