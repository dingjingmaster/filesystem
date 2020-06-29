#include "file-operation-progress-wizard.h"



FileOperationAfterProgressPage::FileOperationAfterProgressPage(QWidget *parent)
{

}

FileOperationAfterProgressPage::~FileOperationAfterProgressPage()
{

}

FileOperationProgressWizard::FileOperationProgressWizard(QWidget *parent) : QWizard(parent)
{

}

FileOperationProgressWizard::~FileOperationProgressWizard()
{

}

void FileOperationProgressWizard::delayShow()
{

}

void FileOperationProgressWizard::onStartSync()
{

}

void FileOperationProgressWizard::onElementFoundAll()
{

}

void FileOperationProgressWizard::switchToRollbackPage()
{

}

void FileOperationProgressWizard::switchToPreparedPage()
{

}

void FileOperationProgressWizard::switchToProgressPage()
{

}

void FileOperationProgressWizard::switchToAfterProgressPage()
{

}

void FileOperationProgressWizard::onFileOperationProgressedAll()
{

}

void FileOperationProgressWizard::onElementClearOne(const QString &uri)
{

}

void FileOperationProgressWizard::onElementFoundOne(const QString &uri, const qint64 &size)
{

}

void FileOperationProgressWizard::onFileRollbacked(const QString &destUri, const QString &srcUri)
{

}

void FileOperationProgressWizard::onFileOperationProgressedOne(const QString &uri, const QString &destUri, const qint64 &size)
{

}

void FileOperationProgressWizard::updateProgress(const QString &srcUri, const QString &destUri, quint64 current, quint64 total)
{

}

void FileOperationProgressWizard::closeEvent(QCloseEvent *e)
{

}

FileOperationPreparePage::FileOperationPreparePage(QWidget *parent)
{

}

FileOperationPreparePage::~FileOperationPreparePage()
{

}

FileOperationProgressPage::FileOperationProgressPage(QWidget *parent)
{

}

FileOperationProgressPage::~FileOperationProgressPage()
{

}

FileOperationRollbackPage::FileOperationRollbackPage(QWidget *parent)
{

}

FileOperationRollbackPage::~FileOperationRollbackPage()
{

}
