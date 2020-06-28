#ifndef FILEOPERATIONERRORHANDLER_H
#define FILEOPERATIONERRORHANDLER_H

#include <QVariant>

#include <gobject/gerror-wrapper.h>


class FileOperationErrorHandler
{
public:
    virtual ~FileOperationErrorHandler();

public Q_SLOTS:
    virtual QVariant slotHandleError(const QString &srcUri, const QString &destDirUri, const GerrorWrapperPtr &err, bool isCritical = false) = 0;
};

#endif // FILEOPERATIONERRORHANDLER_H
