#ifndef FILELABELMODEL_H
#define FILELABELMODEL_H

#include <QColor>
#include <QSettings>
#include <QAbstractListModel>

#define PEONY_FILE_LABEL_IDS "fm-file-label-ids"

class FileLabelItem;

class FileLabelModel : public QAbstractListModel
{
    Q_OBJECT
public:
    int lastLabelId ();
    void removeLabel (int id);
    const QStringList getLabels ();
    const QList<QColor> getColors ();
    FileLabelItem *itemFromId (int id);
    static FileLabelModel *getGlobalModel ();
    QList<FileLabelItem*> getAllFileLabelItems ();
    void setLabelName (int id, const QString &name);
    void setLabelColor (int id, const QColor &color);
    const QStringList getFileLabels (const QString &uri);
    void addLabelToFile (const QString &uri, int labelId);
    const QList<int> getFileLabelIds (const QString &uri);
    const QList<QColor> getFileColors (const QString &uri);
    FileLabelItem *itemFormIndex (const QModelIndex &index);
    void addLabel (const QString &label, const QColor &color);
    void removeFileLabel (const QString &uri, int labelId = -1);
    Qt::ItemFlags flags (const QModelIndex& index) const override;
    int rowCount (const QModelIndex &parent = QModelIndex()) const override;
    QVariant data (const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool insertRows (int row, int count, const QModelIndex &parent = QModelIndex()) override;
    bool removeRows (int row, int count, const QModelIndex &parent = QModelIndex()) override;
    bool setData (const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

Q_SIGNALS:
    void fileLabelChanged(const QString &uri);

public Q_SLOTS:
    void setName (FileLabelItem *item, const QString &name);
    void setColor (FileLabelItem *item, const QColor &color);

protected:
    void addId ();
    void initLabelItems ();

private:
    explicit FileLabelModel (QObject *parent = nullptr);
    ~FileLabelModel ();

private:
    QSettings *mLabelSettings;
    QList<FileLabelItem*> mLabels;
};

#endif // FILELABELMODEL_H
