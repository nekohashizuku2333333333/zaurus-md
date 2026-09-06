#ifndef TODOMD_H
#define TODOMD_H

#include <qstring.h>
#include <qvaluelist.h>

struct TodoMdEntry {
    enum Kind { Raw, Task, Step, Note };
    Kind kind;
    int task;
    bool done;
    bool important;
    QString title;
    QString due;
    QString myday;
    QString doneDate;
    QString raw;
};

struct TodoMdDoc {
    QValueList<TodoMdEntry> entries;
    QString newline;
    bool trailingNewline;
};

class TodoMd {
public:
    static TodoMdDoc parseTodo(const QString &text, const QString &today);
    static QString serializeTodo(const TodoMdDoc &doc);
    static int taskCount(const TodoMdDoc &doc);
    static int entryForTask(const TodoMdDoc &doc, int task);
    static void setTaskDone(TodoMdDoc *doc, int task, bool done, const QString &today);
    static void setStepDone(TodoMdDoc *doc, int task, int step, bool done);
    static void toggleImportant(TodoMdDoc *doc, int task);
    static void setDue(TodoMdDoc *doc, int task, const QString &date);
    static void toggleMyDay(TodoMdDoc *doc, int task, const QString &today);
    static void addTask(TodoMdDoc *doc, const QString &title, bool myday, const QString &today);
    static void addStep(TodoMdDoc *doc, int task, const QString &title);
    static void deleteStep(TodoMdDoc *doc, int task, int step);
    static void deleteTask(TodoMdDoc *doc, int task);
    static void moveTask(TodoMdDoc *doc, int task, int delta);
    static void demoteTaskToStep(TodoMdDoc *doc, int task);
    static void promoteStepToTask(TodoMdDoc *doc, int task, int step);
};

#endif
