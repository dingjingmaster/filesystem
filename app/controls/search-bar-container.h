#ifndef SEARCHBARCONTAINER_H
#define SEARCHBARCONTAINER_H

#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QListView>
#include <QStringList>
#include <QHBoxLayout>
#include <QStringListModel>

class SearchBarContainer : public QWidget
{
    Q_OBJECT
public:
    explicit SearchBarContainer(QWidget *parent = nullptr);

    QSize sizeHint() const override;

    void setPlaceholderText(const QString &content) {
        mSearchBox->setPlaceholderText(content);
    }
    void setFocus() {
        mSearchBox->setFocus();
    }
    QString text() {
        return mSearchBox->text();
    }
    void setText(QString text) {
        mSearchBox->setText(text);
    }

    //get user selected index of file type
    int getFilterIndex() {
        return mFilterBox->currentIndex();
    }
    void clearFilter() {
        mFilterBox->setCurrentIndex(0);
    }

Q_SIGNALS:
    void returnPressed();
    void filterUpdate(const int &index);

public Q_SLOTS:
    void onTableClicked(const QModelIndex &index);

private:
    QHBoxLayout *mLayout = nullptr;

    QLineEdit *mSearchBox;
    QComboBox *mFilterBox;

    QStringListModel *mModel = nullptr;
    QListView *mListView = nullptr;

    QStringList mFileTypeList = {tr("all"), tr("file folder"), tr("image"),
                                    tr("video"), tr("text file"), tr("audio"), tr("others")
                                   };
};

#endif // SEARCHBARCONTAINER_H
