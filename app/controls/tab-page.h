#ifndef TABPAGE_H
#define TABPAGE_H

#include <QTabWidget>
#include <QTimer>


class DirectoryViewContainer;

class TabPage : public QTabWidget
{
    Q_OBJECT
public:
    explicit TabPage(QWidget *parent = nullptr);

    DirectoryViewContainer *getActivePage();

Q_SIGNALS:
    void viewTypeChanged();
    void currentLocationChanged();
    void currentSelectionChanged();
    void currentActiveViewChanged();
    void menuRequest(const QPoint &pos);
    void updateWindowLocationRequest(const QString &uri, bool addHistory = true, bool forceUpdate = false);

public Q_SLOTS:
    void addPage(const QString &uri);
    void refreshCurrentTabText();

    void stopLocationChange();

protected:
    void rebindContainer();

private:
    QTimer mDoubleClickLimiter;
};
#endif // TABPAGE_H
