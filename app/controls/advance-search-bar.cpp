#include "advance-search-bar.h"

#include "main/fm-window.h"
#include "vfs/search-vfs-uri-parser.h"
#include "file-utils.h"

#include <QWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QStringListModel>
#include <QFileDialog>
#include <QMessageBox>
#include <QCheckBox>

#include <QDebug>

AdvanceSearchBar::AdvanceSearchBar(FMWindowIface *window, QWidget *parent) : QScrollArea(parent)
{
    mTopWindow = window;
    init();
}

void AdvanceSearchBar::init()
{
    //add mutiple filter page
    //this->setBackgroundRole(QPalette::Light);
    mFilter = new QWidget(this);
    mFilter->setContentsMargins(0, 0, 0, 0);

    QLabel *keyLabel = new QLabel(tr("Key Words"), mFilter);
    mAdvancedKey = new QLineEdit(mFilter);
    keyLabel->setBuddy(mAdvancedKey);
    mAdvancedKey->setPlaceholderText(tr("input key words..."));
    QLabel *searchLocation = new QLabel(tr("Search Location"), mFilter);
    mSearchPath = new QLineEdit(mFilter);
    mSearchPath->setPlaceholderText(tr("choose search path..."));
    QString uri = mTopWindow->getCurrentUri();
    mSearchPath->setText(FileUtils::getFileDisplayName(uri));
    mAdvanceTargetPath = uri;
    mChoosedPaths.push_back(uri);

    QPushButton *m_browse_button = new QPushButton(tr("browse"), nullptr);
    QLabel *fileType = new QLabel(tr("File Type"), mFilter);
    typeViewCombox = new QComboBox(mFilter);
    typeViewCombox->setToolTip(tr("Choose File Type"));
    auto model = new QStringListModel(mFilter);
    model->setStringList(m_file_type_list);
    typeViewCombox->setModel(model);

    QLabel *modifyTime = new QLabel(tr("Modify Time"), mFilter);
    timeViewCombox = new QComboBox(mFilter);
    timeViewCombox->setToolTip(tr("Choose Modify Time"));
    auto time_model = new QStringListModel(mFilter);
    time_model->setStringList(m_file_mtime_list);
    timeViewCombox->setModel(time_model);

    QLabel *fileSize = new QLabel(tr("File Size"), mFilter);
    sizeViewCombox = new QComboBox(mFilter);
    sizeViewCombox->setToolTip(tr("Choose file size"));
    auto size_model = new QStringListModel(mFilter);
    size_model->setStringList(m_file_size_list);
    sizeViewCombox->setModel(size_model);

    QPushButton *m_show_hidden_button = new QPushButton(tr("show hidden file"), nullptr);
    QPushButton *m_go_back = new QPushButton(tr("go back"), nullptr);
    m_go_back->setToolTip(tr("hidden advance search page"));

    QCheckBox *file_name = new QCheckBox(tr("file name"), nullptr);
    QCheckBox *file_content = new QCheckBox(tr("content"), nullptr);
    file_name->setChecked(true);
    mSearchContent = false;
    mSearchName = true;

    QPushButton *mFilter_button = new QPushButton(tr("search"), nullptr);
    mFilter_button->setToolTip(tr("start search"));

    QFormLayout *topLayout = new QFormLayout();
    topLayout->setContentsMargins(10, 10, 10, 10);
    QWidget *b1 = new QWidget(mFilter);
    QHBoxLayout *middleLayout = new QHBoxLayout(b1);
    b1->setLayout(middleLayout);
    QWidget *b2 = new QWidget(mFilter);
    QHBoxLayout *bottomLayout = new QHBoxLayout(b2);
    b2->setLayout(bottomLayout);
    QVBoxLayout *mainLayout = new QVBoxLayout(mFilter);
    mainLayout->addLayout(topLayout);
    mainLayout->addLayout(middleLayout);
    mainLayout->addLayout(bottomLayout);

    topLayout->addWidget(keyLabel);
    topLayout->addWidget(mAdvancedKey);
    topLayout->addWidget(searchLocation);
    topLayout->addWidget(mSearchPath);
    middleLayout->addWidget(m_browse_button, Qt::AlignCenter);
    middleLayout->setContentsMargins(10,10,10,10);
    topLayout->addWidget(b1);
    topLayout->addWidget(fileType);
    topLayout->addWidget(typeViewCombox);
    topLayout->addWidget(modifyTime);
    topLayout->addWidget(timeViewCombox);
    topLayout->addWidget(fileSize);
    topLayout->addWidget(sizeViewCombox);
    topLayout->addWidget(m_show_hidden_button);
    topLayout->addWidget(m_go_back);
    topLayout->addWidget(file_name);
    topLayout->addWidget(file_content);
    bottomLayout->setContentsMargins(10,20,10,10);
    bottomLayout->addWidget(mFilter_button, Qt::AlignCenter);
    topLayout->addWidget(b2);

    mFilter->setLayout(mainLayout);
    this->setWidget(mFilter);
    //end mutiple filter

    //don't show HorizontalScroll
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    connect(m_browse_button, &QPushButton::clicked, this, &AdvanceSearchBar::browsePath);
    connect(mFilter_button, &QPushButton::clicked, this, &AdvanceSearchBar::searchFilter);
    connect(m_show_hidden_button, &QPushButton::clicked, this, &AdvanceSearchBar::setShowHidden);
    connect(mSearchPath, &QLineEdit::textChanged, this, &AdvanceSearchBar::pathChanged);
    connect(typeViewCombox, &QComboBox::currentTextChanged,this, &AdvanceSearchBar::filterUpdate);
    connect(timeViewCombox, &QComboBox::currentTextChanged,this, &AdvanceSearchBar::filterUpdate);
    connect(sizeViewCombox, &QComboBox::currentTextChanged,this, &AdvanceSearchBar::filterUpdate);
    connect(file_name, &QCheckBox::clicked, this, [=]() {
        mSearchName = file_name->isChecked();
        qDebug()<<"search name"<<mSearchName;
    });
    connect(file_content, &QCheckBox::clicked, this, [=]() {
        mSearchContent = file_content->isChecked();
        qDebug()<<"search content"<<mSearchContent;
    });

    //go back hidden this page
    connect(m_go_back, &QPushButton::clicked, [=]() {
        mTopWindow->slotAdvanceSearch();
    });
}

void AdvanceSearchBar::setdefaultpath(QString path)
{
    mChoosedPaths.clear();
    mChoosedPaths.push_back(path);
}

void AdvanceSearchBar::browsePath()
{
    QString target_path = QFileDialog::getExistingDirectory(this, "caption", mTopWindow->getCurrentUri(), QFileDialog::ShowDirsOnly);
    if (! target_path.contains("file://"))
        target_path = "file://" + target_path;

    if (! mChoosedPaths.contains(target_path))
        mChoosedPaths.push_back(target_path);
    updateLocation();
}

void AdvanceSearchBar::searchFilter()
{
    qDebug()<<"searchFilter clicked"<<mAdvancedKey->text()<<"path:"<<mAdvanceTargetPath;
    if (mAdvancedKey->text() == nullptr || mAdvanceTargetPath == nullptr) //must have key words and target path
    {
        QMessageBox *msgBox = new QMessageBox(this);
        msgBox->setWindowTitle(tr("Operate Tips"));
        msgBox->setText(tr("Have no key words or search location!"));
        msgBox->exec();
        return;
    }

    if (! mSearchName && !mSearchContent)
    {
        QMessageBox *msgBox = new QMessageBox(this);
        msgBox->setWindowTitle(tr("Operate Tips"));
        msgBox->setText(tr("Search file name or content at least choose one!"));
        msgBox->exec();
        return;
    }

    mTopWindow->slotSearchFilter(mAdvanceTargetPath, mAdvancedKey->text(), mSearchName, mSearchContent);
}

void AdvanceSearchBar::filterUpdate()
{
    mTopWindow->slotFilterUpdate(typeViewCombox->currentIndex(), timeViewCombox->currentIndex(), sizeViewCombox->currentIndex());
}

void AdvanceSearchBar::setShowHidden()
{
    mTopWindow->slotSetShowHidden();
    clearData();
}

void AdvanceSearchBar::pathChanged()
{
    if (mLastShowName == mSearchPath->text().trimmed()) {
        return;
    }
    QList<QString> names = mSearchPath->text().split(",");
    for(auto path : mChoosedPaths) {
        QString show = FileUtils::getFileDisplayName(path);
        if (! names.contains(show)) {
            mChoosedPaths.removeOne(path);
        }
    }
    updateLocation();
}

void AdvanceSearchBar::updateLocation()
{
    QString show = nullptr;
    for(auto path : mChoosedPaths) {
        if (show == nullptr) {
            show = FileUtils::getFileDisplayName(path);
            mAdvanceTargetPath = path;
        }

        else {
            show += "," + FileUtils::getFileDisplayName(path);
            mAdvanceTargetPath += "," + path;
        }
    }

    if (mChoosedPaths.size() == 0)
        mAdvanceTargetPath = "";

    mLastShowName = show;
    mSearchPath->setText(show);
}

void AdvanceSearchBar::clearData()
{
    typeViewCombox->setCurrentIndex(0);
    timeViewCombox->setCurrentIndex(0);
    sizeViewCombox->setCurrentIndex(0);
    mChoosedPaths.clear();
}
