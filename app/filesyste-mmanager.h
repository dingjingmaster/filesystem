#ifndef FILESYSTEMMANAGER_H
#define FILESYSTEMMANAGER_H

#include <app/library/single-app/single-app.h>



class FilesystemManager : SingleApp
{
public:
    FilesystemManager(int& argc, char* argv[], const char* appName = "graceful-filesystem");
};

#endif // FILESYSTEMMANAGER_H
