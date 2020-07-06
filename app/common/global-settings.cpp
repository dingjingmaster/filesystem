#include "global-settings.h"

#include <QSize>
#include <QPalette>
#include <QtConcurrent>
#include <QApplication>

static GlobalSettings *gInstance = nullptr;

bool GlobalSettings::isExist(const QString &key)
{
    return !mCache.value(key).isNull();
}

GlobalSettings *GlobalSettings::getInstance()
{
    if (!gInstance) {
        gInstance = new GlobalSettings;
    }
    return gInstance;
}

const QVariant GlobalSettings::getValue(const QString &key)
{
    return mCache.value(key);
}

void GlobalSettings::resetAll()
{
    QStringList tmp = mCache.keys();
    mCache.clear();
    for (auto key : tmp) {
        Q_EMIT this->valueChanged(key);
    }
    QtConcurrent::run([=]() {
        if (mMutex.tryLock(1000)) {
            mSettings->clear();
            mSettings->sync();
            mMutex.unlock();
        }
    });
}

void GlobalSettings::reset(const QString &key)
{
    mCache.remove(key);
    QtConcurrent::run([=]() {
        if (mMutex.tryLock(1000)) {
            mSettings->remove(key);
            mSettings->sync();
            mMutex.unlock();
        }
    });
    Q_EMIT this->valueChanged(key);
}

void GlobalSettings::forceSync(const QString &key)
{
    mSettings->sync();
    if (key.isNull()) {
        mCache.clear();
        for (auto key : mSettings->allKeys()) {
            mCache.insert(key, mSettings->value(key));
        }
    } else {
        mCache.remove(key);
        mCache.insert(key, mSettings->value(key));
    }
}

void GlobalSettings::setValue(const QString &key, const QVariant &value)
{
    mCache.insert(key, value);
    QtConcurrent::run([=]() {
        if (mMutex.tryLock(1000)) {
            mSettings->setValue(key, value);
            mSettings->sync();
            mMutex.unlock();
        }
    });
}

GlobalSettings::GlobalSettings(QObject *parent) : QObject(parent)
{
    mSettings = new QSettings("org.graceful", "filesystem-preferences", this);
    for (auto key : mSettings->allKeys()) {
        mCache.insert(key, mSettings->value(key));
    }

    mCache.insert(SIDEBAR_BG_OPACITY, 50);
    if (QGSettings::isSchemaInstalled("org.graceful.style")) {
        mGsettings = new QGSettings("org.graceful.style", QByteArray(), this);
        connect(mGsettings, &QGSettings::changed, this, [=](const QString &key) {
            if (key == "FilesystemSideBarTransparency") {
                mCache.remove(SIDEBAR_BG_OPACITY);
                mCache.insert(SIDEBAR_BG_OPACITY, mGsettings->get(key).toString());
                qApp->paletteChanged(qApp->palette());
            }
        });
        mCache.remove(SIDEBAR_BG_OPACITY);
        mCache.insert(SIDEBAR_BG_OPACITY, mGsettings->get("FilesystemSideBarTransparency").toString());
    }

    if (mCache.value(DEFAULT_WINDOW_SIZE).isNull()) {
        setValue(DEFAULT_WINDOW_SIZE, QSize(850, 850*0.618));
        setValue(DEFAULT_SIDEBAR_WIDTH, 180);
    }

    if (mCache.value(DEFAULT_VIEW_ID).isNull()) {
        setValue(DEFAULT_VIEW_ID, "Icon View");
    }

    if (mCache.value(DEFAULT_VIEW_ZOOM_LEVEL).isNull()) {
        setValue(DEFAULT_VIEW_ZOOM_LEVEL, 25);
    }
}

GlobalSettings::~GlobalSettings()
{

}
