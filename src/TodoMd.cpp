#include "TodoMd.h"

#include <qregexp.h>
#include <qstringlist.h>

static bool isTaskBody(const QString &line, bool *done, int *prefixLen)
{
    if (line.left(6) == "- [ ] ") {
        *done = false;
        *prefixLen = 6;
        return true;
    }
    if (line.left(6) == "- [x] " || line.left(6) == "- [X] ") {
        *done = true;
        *prefixLen = 6;
        return true;
    }
    if (line.left(5) == "-[ ] ") {
        *done = false;
        *prefixLen = 5;
        return true;
    }
    if (line.left(5) == "-[x] " || line.left(5) == "-[X] ") {
        *done = true;
        *prefixLen = 5;
        return true;
    }
    return false;
}

static int taskIndentLen(const QString &line)
{
    int p = 0;
    while (p < (int)line.length() && line[p] == ' ')
        ++p;
    if (p >= 1 && p <= 3)
        return p;
    if (line.left(1) == "\t")
        return 1;
    return 0;
}

static bool isDateToken(const QString &token, const char *name)
{
    QString prefix(name);
    if (token.left(prefix.length()) != prefix)
        return false;
    QString d = token.mid(prefix.length());
    return d.length() == 10 && d[4] == '-' && d[7] == '-';
}

static void parseTitleMeta(TodoMdEntry *e, const QString &src, const QString &today)
{
    QStringList parts = QStringList::split(' ', src, true);
    QStringList title;
    bool foundDue = false;
    bool foundMyday = false;
    bool foundDone = false;
    bool foundImportant = false;
    for (int i = 0; i < (int)parts.count(); ++i) {
        QString p = parts[i];
        title.append(p);
    }
    for (int i = (int)title.count() - 1; i >= 0; --i) {
        QString p = title[i];
        bool recognized = false;
        bool keepAsTitle = false;
        if (p == "!") {
            if (!foundImportant) {
                e->important = true;
                foundImportant = true;
                recognized = true;
            } else {
                keepAsTitle = true;
            }
        } else if (isDateToken(p, "due:")) {
            if (!foundDue) {
                e->due = p.mid(4);
                foundDue = true;
                recognized = true;
            } else {
                keepAsTitle = true;
            }
        } else if (isDateToken(p, "myday:")) {
            if (!foundMyday) {
                QString d = p.mid(6);
                if (d == today)
                    e->myday = d;
                foundMyday = true;
                recognized = true;
            } else {
                keepAsTitle = true;
            }
        } else if (isDateToken(p, "done:")) {
            if (!foundDone) {
                if (e->done)
                    e->doneDate = p.mid(5);
                foundDone = true;
                recognized = true;
            } else {
                keepAsTitle = true;
            }
        }
        if (recognized) {
            title.remove(title.at(i));
        } else {
            if (!keepAsTitle)
                break;
        }
    }
    e->title = title.join(" ");
}

static QString serializeEntry(const TodoMdEntry &e)
{
    if (e.kind == TodoMdEntry::Raw || e.kind == TodoMdEntry::Note)
        return e.raw;
    QString out;
    if (e.kind == TodoMdEntry::Step)
        out += "  ";
    out += e.done ? "- [x] " : "- [ ] ";
    out += e.title;
    if (e.important)
        out += " !";
    if (!e.due.isEmpty())
        out += " due:" + e.due;
    if (!e.myday.isEmpty())
        out += " myday:" + e.myday;
    if (e.done && !e.doneDate.isEmpty())
        out += " done:" + e.doneDate;
    return out;
}

static void renumberTasks(TodoMdDoc *doc)
{
    int task = -1;
    for (int i = 0; i < (int)doc->entries.count(); ++i) {
        TodoMdEntry e = doc->entries[i];
        if (e.kind == TodoMdEntry::Task)
            ++task;
        if (e.kind == TodoMdEntry::Task || e.kind == TodoMdEntry::Step || e.kind == TodoMdEntry::Note)
            e.task = task;
        doc->entries[i] = e;
    }
}

static int taskBlockEnd(const TodoMdDoc &doc, int start)
{
    int end = start + 1;
    while (end < (int)doc.entries.count() && doc.entries[end].kind != TodoMdEntry::Task)
        ++end;
    return end;
}

TodoMdDoc TodoMd::parseTodo(const QString &text, const QString &today)
{
    TodoMdDoc doc;
    if (text.isEmpty()) {
        doc.newline = "\n";
        doc.trailingNewline = false;
        return doc;
    }
    doc.newline = text.find("\r\n") >= 0 ? "\r\n" : "\n";
    doc.trailingNewline = text.right(doc.newline.length()) == doc.newline;
    QString body = text;
    if (doc.trailingNewline)
        body = body.left(body.length() - doc.newline.length());
    QStringList lines = QStringList::split(doc.newline, body, true);
    int task = -1;
    bool inFence = false;
    for (int i = 0; i < (int)lines.count(); ++i) {
        QString line = lines[i];
        QString stripped = line.stripWhiteSpace();
        if (stripped.left(3) == "```")
            inFence = !inFence;
        TodoMdEntry e;
        e.kind = TodoMdEntry::Raw;
        e.task = task;
        e.done = false;
        e.important = false;
        e.raw = line;
        int prefixLen = 0;
        bool done = false;
        int ind = taskIndentLen(line);
        if (!inFence && isTaskBody(line, &done, &prefixLen)) {
            ++task;
            e.kind = TodoMdEntry::Task;
            e.task = task;
            e.done = done;
            parseTitleMeta(&e, line.mid(prefixLen), today);
        } else if (!inFence && ind > 0 && isTaskBody(line.mid(ind), &done, &prefixLen)) {
            if (task < 0) {
                ++task;
                e.kind = TodoMdEntry::Task;
                e.task = task;
            } else {
                e.kind = TodoMdEntry::Step;
                e.task = task;
            }
            e.done = done;
            parseTitleMeta(&e, line.mid(ind + prefixLen), today);
        } else if (!inFence && task >= 0 && ind > 0) {
            e.kind = TodoMdEntry::Note;
            e.task = task;
            e.raw = line;
        }
        doc.entries.append(e);
    }
    return doc;
}

QString TodoMd::serializeTodo(const TodoMdDoc &doc)
{
    if (doc.entries.count() == 0)
        return "";
    QStringList lines;
    for (int i = 0; i < (int)doc.entries.count(); ++i)
        lines.append(serializeEntry(doc.entries[i]));
    QString out = lines.join(doc.newline);
    if (doc.trailingNewline)
        out += doc.newline;
    return out;
}

int TodoMd::taskCount(const TodoMdDoc &doc)
{
    int n = 0;
    for (int i = 0; i < (int)doc.entries.count(); ++i)
        if (doc.entries[i].kind == TodoMdEntry::Task)
            ++n;
    return n;
}

int TodoMd::entryForTask(const TodoMdDoc &doc, int task)
{
    for (int i = 0; i < (int)doc.entries.count(); ++i)
        if (doc.entries[i].kind == TodoMdEntry::Task && doc.entries[i].task == task)
            return i;
    return -1;
}

void TodoMd::setTaskDone(TodoMdDoc *doc, int task, bool done, const QString &today)
{
    int i = entryForTask(*doc, task);
    if (i < 0)
        return;
    TodoMdEntry e = doc->entries[i];
    e.done = done;
    e.doneDate = done ? today : QString::null;
    doc->entries[i] = e;
}

void TodoMd::setStepDone(TodoMdDoc *doc, int task, int step, bool done)
{
    int seen = 0;
    for (int i = 0; i < (int)doc->entries.count(); ++i) {
        if (doc->entries[i].kind == TodoMdEntry::Step && doc->entries[i].task == task) {
            if (seen == step) {
                TodoMdEntry e = doc->entries[i];
                e.done = done;
                doc->entries[i] = e;
                return;
            }
            ++seen;
        }
    }
}

void TodoMd::setTaskTitle(TodoMdDoc *doc, int task, const QString &title)
{
    int i = entryForTask(*doc, task);
    if (i < 0)
        return;
    TodoMdEntry e = doc->entries[i];
    e.title = title;
    doc->entries[i] = e;
}

void TodoMd::setStepTitle(TodoMdDoc *doc, int task, int step, const QString &title)
{
    int seen = 0;
    for (int i = 0; i < (int)doc->entries.count(); ++i) {
        if (doc->entries[i].kind == TodoMdEntry::Step && doc->entries[i].task == task) {
            if (seen++ == step) {
                TodoMdEntry e = doc->entries[i];
                e.title = title;
                doc->entries[i] = e;
                return;
            }
        }
    }
}

void TodoMd::toggleImportant(TodoMdDoc *doc, int task)
{
    int i = entryForTask(*doc, task);
    if (i < 0)
        return;
    TodoMdEntry e = doc->entries[i];
    e.important = !e.important;
    doc->entries[i] = e;
}

void TodoMd::setDue(TodoMdDoc *doc, int task, const QString &date)
{
    int i = entryForTask(*doc, task);
    if (i < 0)
        return;
    TodoMdEntry e = doc->entries[i];
    e.due = date;
    doc->entries[i] = e;
}

void TodoMd::toggleMyDay(TodoMdDoc *doc, int task, const QString &today)
{
    int i = entryForTask(*doc, task);
    if (i < 0)
        return;
    TodoMdEntry e = doc->entries[i];
    e.myday = e.myday == today ? QString::null : today;
    doc->entries[i] = e;
}

void TodoMd::addTask(TodoMdDoc *doc, const QString &title, bool myday, const QString &today)
{
    TodoMdEntry e;
    e.kind = TodoMdEntry::Task;
    e.task = taskCount(*doc);
    e.done = false;
    e.important = false;
    e.title = title;
    e.myday = myday ? today : QString::null;
    doc->entries.append(e);
    doc->trailingNewline = true;
}

void TodoMd::addStep(TodoMdDoc *doc, int task, const QString &title)
{
    int insertAt = -1;
    for (int i = 0; i < (int)doc->entries.count(); ++i) {
        if (doc->entries[i].task == task)
            insertAt = i + 1;
        else if (insertAt >= 0 && doc->entries[i].kind == TodoMdEntry::Task)
            break;
    }
    if (insertAt < 0)
        return;
    TodoMdEntry e;
    e.kind = TodoMdEntry::Step;
    e.task = task;
    e.done = false;
    e.important = false;
    e.title = title;
    if (insertAt >= (int)doc->entries.count())
        doc->entries.append(e);
    else
        doc->entries.insert(doc->entries.at(insertAt), e);
    doc->trailingNewline = true;
}

void TodoMd::deleteStep(TodoMdDoc *doc, int task, int step)
{
    int seen = 0;
    QValueList<TodoMdEntry> kept;
    for (int i = 0; i < (int)doc->entries.count(); ++i) {
        TodoMdEntry e = doc->entries[i];
        if (e.kind == TodoMdEntry::Step && e.task == task) {
            if (seen++ == step)
                continue;
        }
        kept.append(e);
    }
    doc->entries = kept;
}

void TodoMd::deleteTask(TodoMdDoc *doc, int task)
{
    QValueList<TodoMdEntry> kept;
    for (int i = 0; i < (int)doc->entries.count(); ++i)
        if (doc->entries[i].task != task)
            kept.append(doc->entries[i]);
    doc->entries = kept;
    renumberTasks(doc);
}

void TodoMd::moveTask(TodoMdDoc *doc, int task, int delta)
{
    if (!doc || delta == 0)
        return;
    int from = entryForTask(*doc, task);
    int other = entryForTask(*doc, task + delta);
    if (from < 0 || other < 0)
        return;
    int a = from < other ? from : other;
    int b = from < other ? other : from;
    int aEnd = taskBlockEnd(*doc, a);
    int bEnd = taskBlockEnd(*doc, b);
    QValueList<TodoMdEntry> next;
    for (int i = 0; i < a; ++i)
        next.append(doc->entries[i]);
    for (int i = b; i < bEnd; ++i)
        next.append(doc->entries[i]);
    for (int i = aEnd; i < b; ++i)
        next.append(doc->entries[i]);
    for (int i = a; i < aEnd; ++i)
        next.append(doc->entries[i]);
    for (int i = bEnd; i < (int)doc->entries.count(); ++i)
        next.append(doc->entries[i]);
    doc->entries = next;
    renumberTasks(doc);
}

void TodoMd::demoteTaskToStep(TodoMdDoc *doc, int task)
{
    if (task <= 0)
        return;
    int i = entryForTask(*doc, task);
    if (i < 0)
        return;
    for (int n = i + 1; n < (int)doc->entries.count() && doc->entries[n].kind != TodoMdEntry::Task; ++n)
        if (doc->entries[n].kind == TodoMdEntry::Step)
            return;
    TodoMdEntry e = doc->entries[i];
    e.kind = TodoMdEntry::Step;
    e.task = task - 1;
    e.doneDate = QString::null;
    doc->entries[i] = e;
    renumberTasks(doc);
}

int TodoMd::visibleTaskCount(const TodoMdDoc &doc, int viewMode, const QString &today)
{
    int n = 0;
    for (int i = 0; i < (int)doc.entries.count(); ++i) {
        TodoMdEntry e = doc.entries[i];
        if (e.kind != TodoMdEntry::Task)
            continue;
        bool visible = true;
        if (viewMode == 0)
            visible = e.myday == today || e.due == today;
        else if (viewMode == 1)
            visible = e.important;
        else if (viewMode == 2)
            visible = !e.due.isEmpty();
        if (visible)
            ++n;
    }
    return n;
}

void TodoMd::promoteStepToTask(TodoMdDoc *doc, int task, int step)
{
    int seen = 0;
    for (int i = 0; i < (int)doc->entries.count(); ++i) {
        TodoMdEntry e = doc->entries[i];
        if (e.kind == TodoMdEntry::Step && e.task == task) {
            if (seen++ == step) {
                e.kind = TodoMdEntry::Task;
                e.task = task + 1;
                doc->entries[i] = e;
                renumberTasks(doc);
                return;
            }
        }
    }
}
