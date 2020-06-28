#ifndef FILEOPERATIONERRORDIALOG_H
#define FILEOPERATIONERRORDIALOG_H

#include "gobject/gerror-wrapper.h"
#include "file-operation-error-handler.h"

#include <QLabel>
#include <QDialog>
#include <QFormLayout>
#include <QButtonGroup>
#include <QDialogButtonBox>

class FileOperationErrorDialog : public QDialog, public FileOperationErrorHandler
{
    Q_OBJECT
    Q_INTERFACES(FileOperationErrorHandler)
public:
    explicit FileOperationErrorDialog (QWidget *parent = nullptr);
    ~FileOperationErrorDialog () override;

public Q_SLOTS:
    QVariant slotHandleError (const QString &srcUri, const QString &destDirUri, const GerrorWrapperPtr &err, bool isCritical = false) override;

private:
    QLabel *mErrLine = nullptr;
    QLabel *mSrcLine = nullptr;
    QLabel *mDestLine = nullptr;
    QFormLayout *mLayout = nullptr;
    QButtonGroup *btGroup = nullptr;
    QDialogButtonBox *mButtonBox = nullptr;
    QDialogButtonBox *mButtonBox2 = nullptr;

};

#endif // FILEOPERATIONERRORDIALOG_H
