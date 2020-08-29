#ifndef SEARCHBAR_H
#define SEARCHBAR_H

#include "main/fm-window.h"

#include <QLineEdit>
#include <QStandardItemModel>
#include <QTableView>

class QStringListModel;


class SearchBar : public QLineEdit
{
    Q_OBJECT
public:
    explicit SearchBar(FMWindow *window, QWidget *parent = nullptr);

Q_SIGNALS:
    void searchKeyChanged(const QString &key);
    void searchRequest(const QString &key);

protected:
    void focusInEvent(QFocusEvent *e) override;
    void focusOutEvent(QFocusEvent *e) override;

public Q_SLOTS:
    void init(bool hasTopWindow);
    void initTableModel();
    void updateTableModel();
    void onTableClicked(const QModelIndex &index);
    void clearSearchRecord();
    void hideTableView();

private:
    FMWindow *m_top_window;
    QStandardItemModel *m_model = nullptr;
    QTableView *m_table_view = nullptr;
};


#endif // SEARCHBAR_H
