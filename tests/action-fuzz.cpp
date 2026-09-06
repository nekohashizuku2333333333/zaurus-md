#include "MarkdownActions.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>

static MdActionCtx ctx()
{
    MdActionCtx c;
    c.dateText = "2026-09-06";
    c.timeText = "12:34:56";
    c.indentWidth = 2;
    return c;
}

static void eq(const char *name, MdActionId id, const char *in, int s, int e, const char *out)
{
    MdActionResult r = mdApplyAction(id, in, s, e, ctx());
    if (r.text != out) {
        fprintf(stderr, "%s failed\nin : %s\nout: %s\nwant:%s\n", name, in, r.text.c_str(), out);
        exit(1);
    }
}

static unsigned int rnd(unsigned int *seed)
{
    *seed = *seed * 1103515245u + 12345u;
    return *seed;
}

static bool hasMoreThanOneBlockPrefix(const std::string &line)
{
    unsigned int i = 0;
    while (i < line.size() && (line[i] == ' ' || line[i] == '\t'))
        ++i;
    std::string b = line.substr(i);
    int count = 0;
    if (b.find("- ") == 0 || b.find("* ") == 0 || b.find("+ ") == 0)
        ++count;
    if (b.find("- [ ] ") == 0 || b.find("- [x] ") == 0 || b.find("- [X] ") == 0)
        ++count;
    if (b.find("> ") == 0)
        ++count;
    unsigned int j = 0;
    while (j < b.size() && b[j] >= '0' && b[j] <= '9')
        ++j;
    if (j > 0 && j + 1 < b.size() && b[j] == '.' && b[j + 1] == ' ')
        ++count;
    return count > 1;
}

static void assertNoDoubledBlockPrefix(const std::string &text)
{
    unsigned int pos = 0;
    while (pos <= text.size()) {
        unsigned int next = text.find('\n', pos);
        if (next == (unsigned int)std::string::npos)
            next = text.size();
        assert(!hasMoreThanOneBlockPrefix(text.substr(pos, next - pos)));
        if (next == text.size())
            break;
        pos = next + 1;
    }
}

static void golden()
{
    eq("bold wrap", MdActBold, "abc", 0, 3, "**abc**");
    eq("bold cursor", MdActBold, "abc", 1, 1, "a****bc");
    eq("italic unwrap", MdActItalic, "*abc*", 0, 5, "abc");
    eq("code unwrap outside", MdActCode, "`abc`", 1, 4, "abc");
    eq("strike wrap", MdActStrike, "abc", 0, 3, "~~abc~~");
    eq("heading 1", MdActHeading, "title", 0, 0, "# title");
    eq("heading 2", MdActHeading, "# title", 2, 2, "## title");
    eq("heading 3", MdActHeading, "### title", 3, 3, "title");
    eq("bullet", MdActBullet, "x", 0, 1, "- x");
    eq("number", MdActNumber, "- x", 0, 3, "1. x");
    eq("task", MdActTask, "> x", 0, 3, "- [ ] x");
    eq("quote", MdActQuote, "1. x", 0, 4, "> x");
    eq("done", MdActDone, "- [ ] x", 0, 7, "- [x] x");
    eq("indent", MdActIndent, "x", 0, 1, "  x");
    eq("outdent", MdActOutdent, "  x", 0, 3, "x");
    eq("link", MdActLink, "abc", 0, 3, "[abc](url)");
    eq("rule", MdActRule, "abc", 1, 1, "a\n---\nbc");
    eq("date", MdActDate, "abc", 1, 2, "a2026-09-06c");
    eq("time", MdActTime, "abc", 1, 2, "a12:34:56c");
    eq("enter bullet", MdActEnter, "- x", 3, 3, "- x\n- ");
}

static void fuzz()
{
    const char *alphabet = "abc def\n- #>[]()_`~012";
    for (unsigned int seed0 = 1; seed0 <= 9; ++seed0) {
        unsigned int seed = seed0;
        for (int i = 0; i < 20000; ++i) {
            int len = rnd(&seed) % 90;
            std::string text;
            for (int j = 0; j < len; ++j)
                text += alphabet[rnd(&seed) % 24];
            int s = len ? rnd(&seed) % (len + 1) : 0;
            int e = len ? rnd(&seed) % (len + 1) : 0;
            MdActionId id = (MdActionId)(rnd(&seed) % 16);
            MdActionResult r = mdApplyAction(id, text, s, e, ctx());
            if (r.selStart < 0 || r.selEnd < 0 || r.selStart > (int)r.text.size() || r.selEnd > (int)r.text.size()) {
                fprintf(stderr, "cursor bounds failed seed=%u i=%d id=%d len=%d s=%d e=%d outlen=%u rs=%d re=%d\n",
                        seed0, i, id, len, s, e, (unsigned int)r.text.size(), r.selStart, r.selEnd);
                exit(1);
            }
            assertNoDoubledBlockPrefix(mdApplyAction(MdActBullet, mdApplyAction(MdActNumber, r.text, 0, r.text.size(), ctx()).text, 0, (int)r.text.size(), ctx()).text);
            MdActionResult a = mdApplyAction(MdActBold, text, s, e, ctx());
            MdActionResult b = mdApplyAction(MdActBold, a.text, a.selStart, a.selEnd, ctx());
            assert(b.text == text);
            a = mdApplyAction(MdActCode, text, s, e, ctx());
            b = mdApplyAction(MdActCode, a.text, a.selStart, a.selEnd, ctx());
            assert(b.text == text);
        }
    }
}

int main()
{
    golden();
    fuzz();
    puts("action fuzz ok: golden + 180000 random cases");
    return 0;
}
