#ifndef SEARCH_VFS_URI_PARSER_H
#define SEARCH_VFS_URI_PARSER_H

#include <QString>

class SearchVFSUriParser
{
    SearchVFSUriParser();

public:
    const static QString getSearchUriNameRegexp(const QString &searchUri);
    const static QString getSearchUriTargetDirectory(const QString &searchUri);
    const static QString parseSearchKey(const QString &uri, const QString &key,
                        const bool &searchFileName=true, const bool &searchContent=false,
                        const QString &extendKey="", const bool &recursive = true);
};

#endif // VFSURIPARSER_H
