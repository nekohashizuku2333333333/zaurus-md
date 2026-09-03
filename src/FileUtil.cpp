#include "FileUtil.h"

#include <qcstring.h>
#include <qdir.h>
#include <qfile.h>

bool FileUtil::readUtf8(const QString &path, QString *text)
{
    QFile f(path);
    if (!f.open(IO_ReadOnly))
        return false;
    QByteArray data = f.readAll();
    f.close();
    *text = QString::fromUtf8(data.data(), data.size());
    return true;
}

bool FileUtil::writeUtf8Atomic(const QString &path, const QString &text)
{
    QString tmp = path + ".tmp";
    QFile f(tmp);
    if (!f.open(IO_WriteOnly | IO_Truncate))
        return false;
    QCString data = text.utf8();
    if (f.writeBlock(data.data(), data.length()) != (int)data.length()) {
        f.close();
        QFile::remove(tmp);
        return false;
    }
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
