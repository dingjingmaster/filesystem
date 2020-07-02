#include "file-launch-dialog.h"
#include "file-launch-action.h"
#include "file-launch-manager.h"

#include <QLabel>
#include <QCheckBox>
#include <QListWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QDialogButtonBox>


FileLauchDialog::FileLauchDialog(const QString &uri, QWidget *parent) : QDialog(parent)
{
    mLayout = new QVBoxLayout(this);
    setLayout(mLayout);

    setWindowTitle(tr("Applications"));
    mLayout->addWidget(new QLabel(tr("Choose an Application to open this file"), this));
    mView = new QListWidget(this);
    mView->setIconSize(QSize(48, 48));
    mView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    mLayout->addWidget(mView, 1);
    mCheckBox = new QCheckBox(tr("Set as Default"), this);
    mLayout->addWidget(mCheckBox);
    mButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    mLayout->addWidget(mButtonBox);

    //add button translate
    mButtonBox->button(QDialogButtonBox::Ok)->setText(tr("OK"));
    mButtonBox->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));

    mUri = uri;
    auto actions = FileLaunchManager::getAllActions(uri);
    for (auto action : actions) {
        action->setParent(this);
        auto item = new QListWidgetItem(!action->icon().isNull()? action->icon(): QIcon::fromTheme("application-x-desktop"),
                                        action->text(),
                                        mView);
        mView->addItem(item);
        mHash.insert(item, action);
    }

    connect(this, &QDialog::accepted, [=]() {
        if (mView->currentItem()) {
            auto action = mHash.value(mView->currentItem());
            if (mCheckBox->isChecked()) {
                FileLaunchManager::setDefaultLauchAction(mUri, action);
            }
            action->lauchFileAsync(true);
        } else {
            FileLaunchManager::openAsync(mUri);
        }
    });

    connect(mButtonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(mButtonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

QSize FileLauchDialog::sizeHint() const
{
    return QSize(400, 600);
}
