#include "file-operation-error-dialog.h"
#include "file-operation.h"

#include <QLabel>
#include <QPushButton>
#include <QFormLayout>
#include <QApplication>
#include <QButtonGroup>
#include <QApplication>
#include <QDesktopWidget>
#include <QDialogButtonBox>

FileOperationErrorDialog::FileOperationErrorDialog(QWidget *parent)
{
    setParent(QApplication::desktop());
    //setWindowFlag(Qt::Dialog);
    //use WindowStaysOnTopHint flag to make sure this dialog always stay on top
    setWindowFlags(Qt::Dialog | Qt::WindowStaysOnTopHint);
    setWindowTitle(tr("File Operation Error"));
    setWindowIcon(QIcon::fromTheme("system-error"));
    mLayout = new QFormLayout(this);
    mLayout->setFieldGrowthPolicy(QFormLayout::FieldsStayAtSizeHint);
    mLayout->setRowWrapPolicy(QFormLayout::WrapAllRows);
    mLayout->setLabelAlignment(Qt::AlignRight);
    mLayout->setFormAlignment(Qt::AlignLeft);

    mErrLine = new QLabel(tr("unkwon"), this);
    mSrcLine = new QLabel(tr("null"), this);
    mDestLine = new QLabel(tr("null"), this);

    mLayout->addRow(tr("Error message:"), mErrLine);
    mLayout->addRow(tr("Source File:"), mSrcLine);
    mLayout->addRow(tr("Dest File:"), mDestLine);

    mButtonBox = new QDialogButtonBox(this);
    mButtonBox2 = new QDialogButtonBox(this);
    QPushButton *ignoreOneBt = new QPushButton(tr("Ignore"), mButtonBox);
    QPushButton *ignoreAllBt = new QPushButton(tr("Ignore All"), mButtonBox);
    QPushButton *overwriteOneBt = new QPushButton(tr("Overwrite"), mButtonBox);
    QPushButton *overwriteAllBt = new QPushButton(tr("Overwrite All"), mButtonBox);
    QPushButton *backupOneBt = new QPushButton(tr("Backup"), mButtonBox);
    QPushButton *backupAllBt = new QPushButton(tr("Backup All"), mButtonBox);
    QPushButton *retryBt = new QPushButton(tr("&Retry"), mButtonBox);
    QPushButton *cancelBt = new QPushButton(tr("&Cancel"), mButtonBox);

    btGroup = new QButtonGroup(this);
    btGroup->addButton(ignoreOneBt, 1);
    btGroup->addButton(ignoreAllBt, 2);
    btGroup->addButton(overwriteOneBt, 3);
    btGroup->addButton(overwriteAllBt, 4);
    btGroup->addButton(backupOneBt, 5);
    btGroup->addButton(backupAllBt, 6);
    btGroup->addButton(retryBt, 7);
    btGroup->addButton(cancelBt, 8);

    connect(btGroup, SIGNAL(buttonClicked(int)), this, SLOT(done(int)));

    mButtonBox->addButton(ignoreOneBt, QDialogButtonBox::ActionRole);
    mButtonBox->addButton(ignoreAllBt, QDialogButtonBox::ActionRole);
    mButtonBox->addButton(overwriteOneBt, QDialogButtonBox::ActionRole);
    mButtonBox->addButton(overwriteAllBt, QDialogButtonBox::ActionRole);
    mButtonBox2->addButton(backupOneBt, QDialogButtonBox::ActionRole);
    mButtonBox2->addButton(backupAllBt, QDialogButtonBox::ActionRole);
    mButtonBox2->addButton(retryBt, QDialogButtonBox::ActionRole);
    mButtonBox2->addButton(cancelBt, QDialogButtonBox::ActionRole);

    mLayout->addWidget(mButtonBox);
    mLayout->addWidget(mButtonBox2);

    setLayout(mLayout);
}

FileOperationErrorDialog::~FileOperationErrorDialog()
{

}

QVariant FileOperationErrorDialog::slotHandleError(const QString &srcUri, const QString &destDirUri, const GerrorWrapperPtr &err, bool isCritical)
{
    for (int i = 3; i < 8; i++) {
        btGroup->button(i)->setVisible(!isCritical);
    }

    mSrcLine->setText(srcUri);
    mDestLine->setText(destDirUri);
    mErrLine->setText(err.get()->message());

    for (int i = 1; i < 9; i++) {
        btGroup->button(i)->setFixedWidth(100);
    }

    int val = exec();
    switch (val) {
    case 1: {
        return QVariant(FileOperation::IgnoreOne);
    }
    case 2: {
        return QVariant(FileOperation::IgnoreAll);
    }
    case 3: {
        return QVariant(FileOperation::OverWriteOne);
    }
    case 4: {
        return QVariant(FileOperation::OverWriteAll);
    }
    case 5: {
        return QVariant(FileOperation::BackupOne);
    }
    case 6: {
        return QVariant(FileOperation::BackupAll);
    }
    case 7: {
        return QVariant(FileOperation::Retry);
    }
    case 8: {
        return QVariant(FileOperation::Cancel);
    }
    default: {
        return QVariant(FileOperation::IgnoreAll);
    }
    }
}
