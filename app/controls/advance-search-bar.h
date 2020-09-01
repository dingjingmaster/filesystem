#ifndef ADVANCESEARCHBAR_H
#define ADVANCESEARCHBAR_H
#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QScrollArea>

class FMWindowIface;

class AdvanceSearchBar : public QScrollArea
{
    Q_OBJECT
public:
    explicit AdvanceSearchBar(FMWindowIface *window, QWidget *parent = nullptr);

Q_SIGNALS:

public Q_SLOTS:
    void clearData();
    void browsePath();
    void pathChanged();
    void searchFilter();
    void filterUpdate();
    void setShowHidden();
    void updateLocation();
    void setdefaultpath(QString path);

public:
    //advance search filter options
    QStringList m_file_type_list = {tr("all"), tr("file folder"), tr("image"), tr("video"), tr("text file"), tr("audio"), tr("others")};
    QStringList m_file_mtime_list = {tr("all"), tr("today"), tr("this week"), tr("this month"), tr("this year"), tr("year ago")};
    QStringList m_file_size_list = {tr("all"), tr("tiny(0-16K)"), tr("small(16k-1M)"), tr("medium(1M-100M)"), tr("big(100M-1G)"),tr("large(>1G)")};

protected:
    void init();

private:
    FMWindowIface *mTopWindow;

    QWidget *mFilter;
    QLineEdit *mAdvancedKey, *mSearchPath;
    QComboBox *typeViewCombox, *timeViewCombox, *sizeViewCombox;

    QString mAdvanceTargetPath, mLastShowName;
    QList<QString> mChoosedPaths;

    bool mSearchContent, mSearchName;
};


#endif // ADVANCESEARCHBAR_H
