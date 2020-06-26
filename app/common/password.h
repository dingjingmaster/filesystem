#ifndef PASSWORD_H
#define PASSWORD_H

#include <pwd.h>
#include <sys/types.h>

#include <QObject>

/**
 * @brief 对 struct passwd 结构体中部分属性的封装
 *
 */

class PWD
{
public:
    explicit PWD (passwd *user);
    ~PWD ();

    int userId ();
    int groupId ();

    const QString homeDir ();
    const QString shellDir ();
    const QString fullName ();
    const QString userName ();

private:
    QString mUserName;
    QString mFullName;
    QString mHomeDir;
    QString mShellDir;

    int mUid = -1;
    int mGid = -1;
};

/**
 * @brief 获取当前用户或本机所有用户的 PWD 信息
 * @note
 */
class Password
{
public:
    static const QList<PWD> getAllUserInfos ();
    static const PWD getCurrentUser ();
private:
    Password ();

};

#endif // PASSWORD_H
