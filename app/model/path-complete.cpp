#include "path-complete.h"
#include "pathbar-model.h"


PathComplete::PathComplete(QObject *parent) : QCompleter(parent)
{

}

PathComplete::PathComplete(QAbstractItemModel *model, QObject *parent)
{
    Q_UNUSED(model);
}

QStringList PathComplete::splitPath(const QString &path) const
{
    QAbstractItemModel *m = model();
    PathBarModel* model = static_cast<PathBarModel*>(m);
    if (path.endsWith("/")) {
        model->setRootUri(path);
    } else {
        QString tmp0 = path;
        QString tmp = path;
        tmp.chop(path.size() - path.lastIndexOf("/"));
        if (tmp.endsWith("/")) {
            tmp.append("/");
        }
        model->setRootUri(tmp);
    }

    return QCompleter::splitPath(path);
}

QString PathComplete::pathFromIndex(const QModelIndex &index) const
{
    return QCompleter::pathFromIndex(index);
}
