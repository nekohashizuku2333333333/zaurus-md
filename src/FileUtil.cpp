#include "FileUtil.h"

#include <qdir.h>
#include <qfile.h>
#include <qtextstream.h>

bool FileUtil::readUtf8(const QString &path, QString *text)
{
    QFile f(path);
    if (!f.open(IO_ReadOnly))
        return false;
    QTextStream ts(&f);
    ts.setEncoding(QTextStream::UnicodeUTF8);
    *text = ts.read();
    f.close();
    return true;
}

bool FileUtil::writeUtf8Atomic(const QString &path, const QString &text)
{
    QString tmp = path + ".tmp";
    QFile f(tmp);
    if (!f.open(IO_WriteOnly | IO_Truncate))
        return false;
    QTextStream ts(&f);
    ts.setEncoding(QTextStream::UnicodeUTF8);
    ts << text;
    f.close();
    QFile::remove(path);
    return QDir().rename(tmp, path);
}

bool FileUtil::ensureDir(const QString &path)
{
    QDir d(path);
    if (d.exists())
        return true;
    return d.mkdir(path);
}

