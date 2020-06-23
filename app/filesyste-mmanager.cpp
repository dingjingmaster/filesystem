#include "filesyste-mmanager.h"


FilesystemManager::FilesystemManager(int& argc, char *argv[], const char *appName)
        : SingleApp(argc, argv, appName, true)
{
    setApplicationVersion("v1.0.0");
    setApplicationName(appName);
}
