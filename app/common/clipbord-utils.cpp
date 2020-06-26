#include "clipbord-utils.h"

ClipbordUtils *ClipbordUtils::getInstance()
{

}

void ClipbordUtils::release()
{

}

void ClipbordUtils::clearClipboard()
{

}

bool ClipbordUtils::isClipboardHasFiles()
{

}

bool ClipbordUtils::isClipboardFilesBeCut()
{

}

QStringList ClipbordUtils::getClipboardFilesUris()
{

}

const QString ClipbordUtils::getClipedFilesParentUri()
{

}

void ClipbordUtils::pasteClipboardFiles(const QString &targetDirUri)
{

}

void ClipbordUtils::setClipboardFiles(const QStringList &uris, bool isCut)
{

}

ClipbordUtils::ClipbordUtils(QObject *parent) : QObject(parent)
{

}

ClipbordUtils::~ClipbordUtils()
{

}
