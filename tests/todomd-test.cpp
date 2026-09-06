#include "TodoMd.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

static const QString today = "2026-09-06";

static void eq(const char *name, const QString &a, const QString &b)
{
    if (a != b) {
        fprintf(stderr, "%s failed\nout : %s\nwant: %s\n", name, a.latin1(), b.latin1());
        exit(1);
    }
}

static unsigned int rnd(unsigned int *seed)
{
    *seed = *seed * 1103515245u + 12345u;
    return *seed;
}

static QString sampleLine(unsigned int *seed)
{
    switch (rnd(seed) % 12) {
    case 0: return "- [ ] task due:2026-09-06 ! myday:2026-09-06";
    case 1: return "- [x] done done:2026-09-06";
    case 2: return "  - [ ] step";
    case 3: return "  - [x] step done:2020-01-01";
    case 4: return "  note line";
    case 5: return "# heading";
    case 6: return "";
    case 7: return "plain paragraph due:2024-01-01";
    case 8: return "```";
    case 9: return "- [ ] unknown later:abc due:2026-09-07 due:2026-09-08";
    case 10: return "   - [ ] three-space should stay raw";
    default: return "\t- [ ] tab should stay raw";
    }
}

static QString randomDoc(unsigned int *seed)
{
    QString nl = (rnd(seed) % 3 == 0) ? "\r\n" : "\n";
    QString out;
    int lines = rnd(seed) % 30;
    for (int i = 0; i < lines; ++i) {
        if (i)
            out += nl;
        out += sampleLine(seed);
    }
    if (rnd(seed) % 2)
        out += nl;
    return out;
}

static void golden()
{
    eq("empty roundtrip", TodoMd::serializeTodo(TodoMd::parseTodo("", today)), "");
    eq("plain roundtrip", TodoMd::serializeTodo(TodoMd::parseTodo("# H\nplain\n", today)), "# H\nplain\n");
    eq("crlf roundtrip", TodoMd::serializeTodo(TodoMd::parseTodo("- [ ] a ! due:2026-09-07\r\n  note\r\n", today)), "- [ ] a ! due:2026-09-07\r\n  note\r\n");
    eq("metadata order", TodoMd::serializeTodo(TodoMd::parseTodo("- [ ] buy due:2026-09-07 ! myday:2026-09-06\n", today)), "- [ ] buy ! due:2026-09-07 myday:2026-09-06\n");
    eq("stale myday ignored", TodoMd::serializeTodo(TodoMd::parseTodo("- [ ] buy myday:2020-01-01\n", today)), "- [ ] buy\n");
    eq("unchecked done stripped", TodoMd::serializeTodo(TodoMd::parseTodo("- [ ] buy done:2020-01-01\n", today)), "- [ ] buy\n");
    eq("orphan step becomes task", TodoMd::serializeTodo(TodoMd::parseTodo("  - [ ] orphan\n", today)), "- [ ] orphan\n");
    eq("compact checkbox", TodoMd::serializeTodo(TodoMd::parseTodo("-[ ] compact\n", today)), "- [ ] compact\n");
    eq("upper done", TodoMd::serializeTodo(TodoMd::parseTodo("- [X] big\n", today)), "- [x] big\n");
    eq("one-space step", TodoMd::serializeTodo(TodoMd::parseTodo("- [ ] a\n - [ ] s\n", today)), "- [ ] a\n  - [ ] s\n");
    eq("three-space note", TodoMd::serializeTodo(TodoMd::parseTodo("- [ ] a\n   note\n", today)), "- [ ] a\n   note\n");
    eq("tab step", TodoMd::serializeTodo(TodoMd::parseTodo("- [ ] a\n\t- [ ] s\n", today)), "- [ ] a\n  - [ ] s\n");
    eq("invalid due kept", TodoMd::serializeTodo(TodoMd::parseTodo("- [ ] a due:bad\n", today)), "- [ ] a due:bad\n");
    eq("middle bang kept", TodoMd::serializeTodo(TodoMd::parseTodo("- [ ] a ! b\n", today)), "- [ ] a ! b\n");
    eq("duplicate due keeps old", TodoMd::serializeTodo(TodoMd::parseTodo("- [ ] a due:2026-01-01 due:2026-02-02\n", today)), "- [ ] a due:2026-01-01 due:2026-02-02\n");

    TodoMdDoc doc = TodoMd::parseTodo("- [ ] a\n  - [ ] s\n  note\n- [ ] b\n", today);
    TodoMd::setTaskDone(&doc, 0, true, today);
    eq("task done", TodoMd::serializeTodo(doc), "- [x] a done:2026-09-06\n  - [ ] s\n  note\n- [ ] b\n");
    TodoMd::setTaskDone(&doc, 0, false, today);
    eq("task undone", TodoMd::serializeTodo(doc), "- [ ] a\n  - [ ] s\n  note\n- [ ] b\n");
    TodoMd::setStepDone(&doc, 0, 0, true);
    eq("step done", TodoMd::serializeTodo(doc), "- [ ] a\n  - [x] s\n  note\n- [ ] b\n");
    TodoMd::toggleImportant(&doc, 1);
    eq("important", TodoMd::serializeTodo(doc), "- [ ] a\n  - [x] s\n  note\n- [ ] b !\n");
    TodoMd::moveTask(&doc, 1, -1);
    eq("move up", TodoMd::serializeTodo(doc), "- [ ] b !\n- [ ] a\n  - [x] s\n  note\n");
    TodoMd::setTaskTitle(&doc, 0, "bb");
    TodoMd::setDue(&doc, 0, "2026-10-01");
    TodoMd::toggleMyDay(&doc, 0, today);
    eq("edit metadata", TodoMd::serializeTodo(doc), "- [ ] bb ! due:2026-10-01 myday:2026-09-06\n- [ ] a\n  - [x] s\n  note\n");
    TodoMd::toggleMyDay(&doc, 0, today);
    eq("myday toggle off", TodoMd::serializeTodo(doc), "- [ ] bb ! due:2026-10-01\n- [ ] a\n  - [x] s\n  note\n");
    TodoMd::demoteTaskToStep(&doc, 1);
    eq("demote with steps rejected", TodoMd::serializeTodo(doc), "- [ ] bb ! due:2026-10-01\n- [ ] a\n  - [x] s\n  note\n");
    TodoMd::deleteStep(&doc, 1, 0);
    eq("delete step", TodoMd::serializeTodo(doc), "- [ ] bb ! due:2026-10-01\n- [ ] a\n  note\n");
    doc = TodoMd::parseTodo("- [ ] a\n  - [ ] s\n- [ ] b\n", today);
    TodoMd::demoteTaskToStep(&doc, 0);
    eq("demote first rejected", TodoMd::serializeTodo(doc), "- [ ] a\n  - [ ] s\n- [ ] b\n");
    TodoMd::demoteTaskToStep(&doc, 1);
    eq("demote leaf", TodoMd::serializeTodo(doc), "- [ ] a\n  - [ ] s\n  - [ ] b\n");
    assert(TodoMd::visibleTaskCount(TodoMd::parseTodo("- [ ] a !\n- [ ] b due:2026-09-06\n- [ ] c myday:2026-09-06\n", today), 0, today) == 2);
    assert(TodoMd::visibleTaskCount(TodoMd::parseTodo("- [ ] a !\n- [ ] b due:2026-09-06\n", today), 1, today) == 1);
    assert(TodoMd::visibleTaskCount(TodoMd::parseTodo("- [ ] a !\n- [ ] b due:2026-09-06\n", today), 2, today) == 1);
}

static void fuzz()
{
    for (unsigned int seed0 = 1; seed0 <= 5; ++seed0) {
        unsigned int seed = seed0;
        for (int i = 0; i < 20000; ++i) {
            QString text = randomDoc(&seed);
            TodoMdDoc doc = TodoMd::parseTodo(text, today);
            QString stable = TodoMd::serializeTodo(doc);
            TodoMdDoc doc2 = TodoMd::parseTodo(stable, today);
            eq("parse serialize idempotent", TodoMd::serializeTodo(doc2), stable);
            int tasks = TodoMd::taskCount(doc);
            if (tasks > 0) {
                int task = rnd(&seed) % tasks;
                QString before = TodoMd::serializeTodo(doc);
                TodoMd::toggleImportant(&doc, task);
                TodoMd::toggleImportant(&doc, task);
                eq("important involution", TodoMd::serializeTodo(doc), before);
                TodoMd::setTaskDone(&doc, task, true, today);
                TodoMd::setTaskDone(&doc, task, false, today);
                TodoMdDoc reparsed = TodoMd::parseTodo(TodoMd::serializeTodo(doc), today);
                assert(TodoMd::taskCount(reparsed) == tasks);
                TodoMd::toggleMyDay(&doc, task, today);
                TodoMd::toggleMyDay(&doc, task, today);
                TodoMd::setDue(&doc, task, "2027-01-02");
                TodoMd::moveTask(&doc, task, -1);
                TodoMd::moveTask(&doc, task - 1, 1);
                (void)before;
            }
        }
    }
}

int main()
{
    golden();
    fuzz();
    puts("todomd ok: golden + 100000 random docs");
    return 0;
}
