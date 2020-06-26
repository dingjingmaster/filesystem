#ifndef FILEOPERATIONERRORDIALOG_H
#define FILEOPERATIONERRORDIALOG_H

#include "file-operation-error-handler.h"

#include <QDialog>



class FileOperationErrorDialog : public QDialog, public FileOperationErrorHandler
{
public:
    FileOperationErrorDialog();
};

#endif // FILEOPERATIONERRORDIALOG_H
