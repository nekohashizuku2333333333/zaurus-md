#ifndef FILEUTIL_H
#define FILEUTIL_H

#include <qstring.h>

class FileUtil {
public:
    static bool readUtf8(const QString &path, QString *text);
    static bool writeUtf8Atomic(const QString &path, const QString &text);
    static bool ensureDir(const QString &path);
};

#endif

