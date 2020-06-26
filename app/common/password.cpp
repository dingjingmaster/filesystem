#include "password.h"

#include <unistd.h>

const QList<PWD> Password::getAllUserInfos()
{
    setpwent();
    QList<PWD> l;
    struct passwd *user;
    while ((user = getpwent())!=nullptr) {
        l << PWD(user);
    }
    endpwent();

    return l;
}

const PWD Password::getCurrentUser()
{
    uid_t uid = geteuid ();
    struct passwd * pwd = getpwuid (uid);
    return PWD (pwd);
}

Password::Password()
{

}

PWD::PWD(passwd *user)
{
    mUserName = user->pw_name;
    mFullName = user->pw_gecos;
    mHomeDir = user->pw_dir;
    mShellDir = user->pw_shell;
    mUid = user->pw_uid;
    mGid = user->pw_gid;
}

PWD::~PWD()
{

}

const QString PWD::userName()
{
    return mUserName;
}

int PWD::userId()
{
    return mUid;
}

int PWD::groupId()
{
    return mGid;
}

const QString PWD::fullName()
{
    return mFullName;
}

const QString PWD::homeDir()
{
    return mHomeDir;
}

const QString PWD::shellDir()
{
    return mShellDir;
}
