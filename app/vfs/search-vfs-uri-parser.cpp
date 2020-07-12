#include "search-vfs-uri-parser.h"
#include <syslog/clib_syslog.h>

#include <QStringList>
#include <file-utils.h>

SearchVFSUriParser::SearchVFSUriParser()
{

}

const QString SearchVFSUriParser::getSearchUriNameRegexp(const QString &searchUri)
{
    auto string = searchUri;
    string.remove("search:///");
    auto list = string.split("&");
    QString ret = "";
    for (auto arg : list) {
        if (arg.startsWith("name_regexp=")) {
            CT_SYSLOG(LOG_DEBUG, "search - name: %s", arg.toUtf8().constData())
            auto tmp = arg.remove("name_regexp=");
            if (ret == "") {
                ret = tmp;
            } else {
                ret += "," + tmp;
            }
        }

        if (arg.startsWith("extend_regexp")) {
            CT_SYSLOG(LOG_DEBUG, "search - extend: %s", arg.toUtf8().constData())
            auto tmp = arg.remove("extend_regexp=");
            if (ret == "") {
                ret = tmp;
            } else {
                ret += "," + tmp;
            }
        }
    }
    return ret;
}

const QString SearchVFSUriParser::getSearchUriTargetDirectory(const QString &searchUri)
{
    auto string = searchUri;
    string.remove("search:///");
    auto list = string.split("&");
    for (auto arg : list) {
        if (arg.startsWith("search_uris=")) {
            CT_SYSLOG(LOG_DEBUG, "search_uris=%s", arg.toUtf8().constData())
            auto tmp = arg.remove("search_uris=");
            auto uris = tmp.split(",");
            if (uris.count() == 1) {
                return FileUtils::getFileDisplayName(tmp);
            }
            tmp = nullptr;
            QStringList names;
            for (auto uri : uris) {
                auto displayName = FileUtils::getFileDisplayName(uri);
                if (tmp == nullptr)
                    tmp = displayName;
                else
                    tmp = tmp + ", " + displayName;
            }
            return tmp;
        }
    }
    return nullptr;
}

const QString SearchVFSUriParser::parseSearchKey(const QString &uri, const QString &key, const bool &searchFileName, const bool &searchContent, const QString &extendKey, const bool &recursive)
{
    QString searchStr = "search:///search_uris=" + uri;
    if (searchFileName) {
        searchStr += "&name_regexp=" + key;
    }

    if (searchContent) {
        searchStr += "&content_regexp="+key;
    }

    if (extendKey != "") {
        searchStr += "&extend_regexp=" + extendKey;
    } else if (!searchFileName && ! searchContent && extendKey == "") {
        CT_SYSLOG(LOG_WARNING, "Search content or file name at least one be true!")
        searchStr += "&name_regexp="+key;
    }

    if (recursive) {
        return QString(searchStr + "&recursive=1");
    }

    return QString(searchStr + "&recursive=0");
}
