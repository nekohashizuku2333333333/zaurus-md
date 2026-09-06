#include "MarkdownActions.h"

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>

static int clampPos(int value, int max)
{
    if (value < 0)
        return 0;
    if (value > max)
        return max;
    return value;
}

static bool startsWith(const std::string &s, const std::string &p)
{
    return s.size() >= p.size() && s.substr(0, p.size()) == p;
}

static bool endsWith(const std::string &s, const std::string &p)
{
    return s.size() >= p.size() && s.substr(s.size() - p.size(), p.size()) == p;
}

static std::string repeatSpace(int n)
{
    if (n <= 0)
        return "";
    return std::string((unsigned int)n, ' ');
}

static bool orderedPrefixLen(const std::string &s, unsigned int *len, int *num)
{
    unsigned int i = 0;
    int n = 0;
    while (i < s.size() && isdigit((unsigned char)s[i]) && i < 6) {
        n = n * 10 + s[i] - '0';
        ++i;
    }
    if (i > 0 && i + 1 < s.size() && s[i] == '.' && s[i + 1] == ' ') {
        if (len)
            *len = i + 2;
        if (num)
            *num = n;
        return true;
    }
    return false;
}

static unsigned int indentLen(const std::string &line)
{
    unsigned int i = 0;
    while (i < line.size() && (line[i] == ' ' || line[i] == '\t'))
        ++i;
    return i;
}

static std::string stripBlockPrefix(const std::string &line)
{
    unsigned int ind = indentLen(line);
    std::string body = line.substr(ind);
    unsigned int len = 0;
    if (startsWith(body, "- [ ] ") || startsWith(body, "- [x] ") || startsWith(body, "- [X] "))
        return line.substr(0, ind) + body.substr(6);
    if (startsWith(body, "- ") || startsWith(body, "* ") || startsWith(body, "+ ") || startsWith(body, "> "))
        return line.substr(0, ind) + body.substr(2);
    if (orderedPrefixLen(body, &len, 0))
        return line.substr(0, ind) + body.substr(len);
    return line;
}

static void lineBounds(const std::string &text, int pos, int *start, int *end)
{
    int s = pos;
    while (s > 0 && text[(unsigned int)s - 1] != '\n')
        --s;
    int e = pos;
    while (e < (int)text.size() && text[(unsigned int)e] != '\n')
        ++e;
    *start = s;
    *end = e;
}

static MdActionResult replaceRange(const std::string &text, int s, int e, const std::string &insert, int rs, int re)
{
    MdActionResult r;
    r.text = text.substr(0, s) + insert + text.substr(e);
    r.selStart = rs;
    r.selEnd = re;
    return r;
}

static MdActionResult inlineToggle(const std::string &text, int s, int e, const std::string &mark)
{
    if (s == e) {
        int ml = (int)mark.size();
        if (s >= ml && s + ml <= (int)text.size()
                && text.substr(s - ml, ml) == mark && text.substr(s, ml) == mark)
            return replaceRange(text, s - ml, s + ml, "", s - ml, s - ml);
        return replaceRange(text, s, e, mark + mark, s + ml, s + ml);
    }

    int ml = (int)mark.size();
    std::string sel = text.substr(s, e - s);
    if (s >= ml && e + ml <= (int)text.size()
            && text.substr(s - ml, ml) == mark && text.substr(e, ml) == mark)
        return replaceRange(text, s - ml, e + ml, sel, s - ml, e - ml);
    if (startsWith(sel, mark) && endsWith(sel, mark) && (int)sel.size() >= ml * 2) {
        std::string inner = sel.substr(ml, sel.size() - ml * 2);
        return replaceRange(text, s, e, inner, s, s + (int)inner.size());
    }
    return replaceRange(text, s, e, mark + sel + mark, s + ml, e + ml);
}

static MdActionResult applyLines(const std::string &text, int s, int e, MdActionId id, const MdActionCtx &ctx)
{
    int ls = 0, dummy = 0, le = 0;
    lineBounds(text, s, &ls, &dummy);
    lineBounds(text, e, &dummy, &le);
    std::string block = text.substr(ls, le - ls);
    std::string out;
    unsigned int pos = 0;
    int ordinal = 1;
    while (pos <= block.size()) {
        unsigned int next = block.find('\n', pos);
        if (next == (unsigned int)std::string::npos)
            next = block.size();
        std::string line = block.substr(pos, next - pos);
        unsigned int ind = indentLen(line);
        std::string indent = line.substr(0, ind);
        std::string body = line.substr(ind);
        unsigned int opl = 0;
        std::string base = stripBlockPrefix(line);
        std::string baseBody = base.substr(indentLen(base));
        if (id == MdActBullet) {
            line = startsWith(body, "- ") ? indent + body.substr(2) : indent + "- " + baseBody;
        } else if (id == MdActNumber) {
            if (orderedPrefixLen(body, &opl, 0)) {
                line = indent + body.substr(opl);
            } else {
                char num[16];
                snprintf(num, sizeof(num), "%d", ordinal);
                line = indent + std::string(num) + ". " + baseBody;
            }
        } else if (id == MdActTask) {
            if (startsWith(body, "- [ ] ") || startsWith(body, "- [x] ") || startsWith(body, "- [X] "))
                line = indent + body.substr(6);
            else
                line = indent + "- [ ] " + baseBody;
        } else if (id == MdActQuote) {
            line = startsWith(body, "> ") ? indent + body.substr(2) : indent + "> " + baseBody;
        } else if (id == MdActDone) {
            if (startsWith(body, "- [ ] "))
                line = indent + "- [x] " + body.substr(6);
            else if (startsWith(body, "- [x] ") || startsWith(body, "- [X] "))
                line = indent + "- [ ] " + body.substr(6);
            else
                line = indent + "- [x] " + baseBody;
        } else if (id == MdActIndent) {
            line = repeatSpace(ctx.indentWidth) + line;
        } else if (id == MdActOutdent) {
            int remove = 0;
            while (remove < ctx.indentWidth && remove < (int)line.size() && line[(unsigned int)remove] == ' ')
                ++remove;
            if (remove > 0)
                line = line.substr(remove);
            else if (!line.empty() && line[0] == '\t')
                line = line.substr(1);
        }
        out += line;
        if (next == block.size())
            break;
        out += '\n';
        pos = next + 1;
        ++ordinal;
    }
    int delta = (int)out.size() - (le - ls);
    int maxPos = (int)text.size() + delta;
    return replaceRange(text, ls, le, out, clampPos(s + delta, maxPos), clampPos(e + delta, maxPos));
}

static MdActionResult headingCycle(const std::string &text, int s, int e)
{
    int ls, le;
    lineBounds(text, s, &ls, &le);
    std::string line = text.substr(ls, le - ls);
    unsigned int ind = indentLen(line);
    std::string indent = line.substr(0, ind);
    std::string body = line.substr(ind);
    int level = 0;
    while (level < 3 && level < (int)body.size() && body[(unsigned int)level] == '#')
        ++level;
    bool valid = level > 0 && level < (int)body.size() && body[(unsigned int)level] == ' ';
    std::string content = valid ? body.substr(level + 1) : body;
    int next = valid ? level + 1 : 1;
    std::string out = (next > 3) ? indent + content : indent + std::string((unsigned int)next, '#') + " " + content;
    int delta = (int)out.size() - (le - ls);
    return replaceRange(text, ls, le, out, clampPos(s + delta, (int)text.size() + delta), clampPos(e + delta, (int)text.size() + delta));
}

static MdActionResult enterAction(const std::string &text, int s)
{
    int ls, le;
    lineBounds(text, s, &ls, &le);
    std::string line = text.substr(ls, le - ls);
    unsigned int ind = indentLen(line);
    std::string indent = line.substr(0, ind);
    std::string body = line.substr(ind);
    unsigned int len = 0;
    int num = 0;
    std::string cont;
    if (startsWith(body, "- [ ] ") || startsWith(body, "- [x] ") || startsWith(body, "- [X] "))
        cont = indent + "- [ ] ";
    else if (startsWith(body, "- ") || startsWith(body, "* ") || startsWith(body, "+ "))
        cont = indent + body.substr(0, 2);
    else if (startsWith(body, "> "))
        cont = indent + "> ";
    else if (orderedPrefixLen(body, &len, &num)) {
        char next[16];
        snprintf(next, sizeof(next), "%d", num + 1);
        cont = indent + std::string(next) + ". ";
    }
    bool emptyMarker = false;
    if (startsWith(body, "- [ ] ") || startsWith(body, "- [x] ") || startsWith(body, "- [X] "))
        emptyMarker = body.substr(6).find_first_not_of(" \t") == std::string::npos;
    else if (startsWith(body, "- ") || startsWith(body, "* ") || startsWith(body, "+ ") || startsWith(body, "> "))
        emptyMarker = body.substr(2).find_first_not_of(" \t") == std::string::npos;
    else if (orderedPrefixLen(body, &len, &num))
        emptyMarker = body.substr(len).find_first_not_of(" \t") == std::string::npos;
    if (!cont.empty() && emptyMarker && s == le) {
        return replaceRange(text, ls, le, "", ls, ls);
    }
    return replaceRange(text, s, s, "\n" + cont, s + 1 + (int)cont.size(), s + 1 + (int)cont.size());
}

MdActionResult mdApplyAction(MdActionId id, const std::string &text, int selStart, int selEnd, const MdActionCtx &ctx)
{
    int s = clampPos(selStart, (int)text.size());
    int e = clampPos(selEnd, (int)text.size());
    if (s > e) {
        int t = s;
        s = e;
        e = t;
    }
    switch (id) {
    case MdActBold: return inlineToggle(text, s, e, "**");
    case MdActItalic: return inlineToggle(text, s, e, "*");
    case MdActCode: return inlineToggle(text, s, e, "`");
    case MdActStrike: return inlineToggle(text, s, e, "~~");
    case MdActHeading: return headingCycle(text, s, e);
    case MdActBullet:
    case MdActNumber:
    case MdActTask:
    case MdActQuote:
    case MdActDone:
    case MdActIndent:
    case MdActOutdent:
        return applyLines(text, s, e, id, ctx);
    case MdActLink:
        return replaceRange(text, s, e, "[" + (s == e ? std::string("text") : text.substr(s, e - s)) + "](url)", s + 1, s + 1 + (s == e ? 4 : e - s));
    case MdActRule:
        return replaceRange(text, s, e, "\n---\n", s + 5, s + 5);
    case MdActDate:
        return replaceRange(text, s, e, ctx.dateText, s + (int)ctx.dateText.size(), s + (int)ctx.dateText.size());
    case MdActTime:
        return replaceRange(text, s, e, ctx.timeText, s + (int)ctx.timeText.size(), s + (int)ctx.timeText.size());
    case MdActEnter:
        return enterAction(text, s);
    }
    MdActionResult r;
    r.text = text;
    r.selStart = s;
    r.selEnd = e;
    return r;
}
