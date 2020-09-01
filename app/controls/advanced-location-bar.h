#ifndef ADVANCEDLOCATIONBAR_H
#define ADVANCEDLOCATIONBAR_H

#include <QWidget>

class PathEdit;
class LocationBar;
class QStackedLayout;
class SearchBarContainer;

class AdvancedLocationBar : public QWidget
{
    Q_OBJECT
public:
    explicit AdvancedLocationBar(QWidget *parent = nullptr);
    bool isEditing();

Q_SIGNALS:
    void refreshRequest();
    void searchRequest(const QString &key);
    void updateFileTypeFilter(const int &index);
    void updateWindowLocationRequest(const QString &uri, bool addHistory = true, bool forceUpdate = false);

public Q_SLOTS:
    void startEdit();
    void finishEdit();
    void switchEditMode(bool bSearchMode);
    void updateLocation(const QString &uri);

private:
    QString mText;
    PathEdit *mEdit;
    LocationBar *mBar;
    QStackedLayout *mLayout;
    QString mLastNonSearchPath;
    SearchBarContainer *mSearchBar;
};


#endif // ADVANCEDLOCATIONBAR_H
