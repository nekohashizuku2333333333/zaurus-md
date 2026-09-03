#include "TodoTxt.h"

#include <qregexp.h>

TodoItem TodoTxt::parseLine(const QString &line)
{
    TodoItem item;
    item.original = line;
    item.done = line.left(2) == "x ";
    item.priority = QChar();

    QString rest = line;
    if (item.done)
        rest = rest.mid(2);

    if (rest.length() >= 4 && rest[0] == '(' && rest[2] == ')' && rest[1] >= 'A' && rest[1] <= 'Z') {
        item.priority = rest[1];
        rest = rest.mid(4);
    }

    QStringList parts = QStringList::split(' ', rest);
    for (uint i = 0; i < parts.count(); ++i) {
        QString p = parts[i];
        if (p.left(1) == "+" && p.length() > 1)
            item.projects.append(p.mid(1));
        else if (p.left(1) == "@" && p.length() > 1)
            item.contexts.append(p.mid(1));
        else if (p.left(4) == "due:")
            item.due = p.mid(4);
    }
    return item;
}

QString TodoTxt::toggleDone(const QString &line, const QString &today)
{
    if (line.left(2) == "x ") {
        QString out = line.mid(2);
        if (out.length() >= 11 && out[4] == '-' && out[7] == '-')
            out = out.mid(11);
        return out;
    }
    return "x " + today + " " + line;
}

