#ifndef FILELAUNCHDIALOG_H
#define FILELAUNCHDIALOG_H

#include <QDialog>

class QCheckBox;
class QVBoxLayout;
class QListWidget;
class QListWidgetItem;
class QDialogButtonBox;
class FileLaunchAction;

class FileLauchDialog : public QDialog
{
    Q_OBJECT
public:
    explicit FileLauchDialog (const QString &uri, QWidget *parent = nullptr);

    QSize sizeHint () const override;

private:
    QString mUri;
    QListWidget *mView;
    QCheckBox *mCheckBox;
    QVBoxLayout *mLayout;
    QDialogButtonBox *mButtonBox;
    QHash<QListWidgetItem*, FileLaunchAction*> mHash;
};
#endif // FILELAUNCHDIALOG_H
