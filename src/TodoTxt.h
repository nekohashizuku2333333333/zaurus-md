#ifndef TODOTXT_H
#define TODOTXT_H

#include <qstring.h>
#include <qstringlist.h>

struct TodoItem {
    QString original;
    bool done;
    QChar priority;
    QString completionDate;
    QString creationDate;
    QStringList projects;
    QStringList contexts;
    QString due;
};

class TodoTxt {
public:
    static TodoItem parseLine(const QString &line);
    static QString toggleDone(const QString &line, const QString &today);
};

#endif

