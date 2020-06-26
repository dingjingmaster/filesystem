#ifndef CLIPBORDUTILS_H
#define CLIPBORDUTILS_H

#include <QObject>

class ClipbordUtils : public QObject
{
    Q_OBJECT
public:
    ClipbordUtils* getInstance ();

    void release ();
    static void clearClipboard ();
    static bool isClipboardHasFiles ();
    static bool isClipboardFilesBeCut ();
    static QStringList getClipboardFilesUris ();
    static const QString getClipedFilesParentUri ();
    static void pasteClipboardFiles (const QString &targetDirUri);
    static void setClipboardFiles (const QStringList &uris, bool isCut);

private:
    explicit ClipbordUtils (QObject *parent = nullptr);
    ~ClipbordUtils ();

Q_SIGNALS:
    void clipboardChanged ();

};

#endif // CLIPBORDUTILS_H
