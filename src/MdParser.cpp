#include "MdParser.h"

#include <qstringlist.h>

QString MdParser::escape(const QString &text)
{
    QString out;
    for (uint i = 0; i < text.length(); ++i) {
        QChar c = text[i];
        if (c == '&')
            out += "&amp;";
        else if (c == '<')
            out += "&lt;";
        else if (c == '>')
            out += "&gt;";
        else if (c == '"')
            out += "&quot;";
        else
            out += c;
    }
    return out;
}

QString MdParser::inlineRich(const QString &text)
{
    QString out;
    uint i = 0;
    while (i < text.length()) {
        if (text[i] == '\\' && i + 1 < text.length()
            && QString("\\`*_{}[]()#+-.!>~|").find(text[i + 1]) >= 0) {
            out += escape(text.mid(i + 1, 1));
            i += 2;
            continue;
        }
        if (i + 1 < text.length() && text[i] == '!' && text[i + 1] == '[') {
            int close = text.find("](", i + 2);
            if (close > (int)i) {
                int end = text.find(')', close + 2);
                if (end > close) {
                    QString alt = escape(text.mid(i + 2, close - i - 2));
                    QString src = escape(text.mid(close + 2, end - close - 2));
                    out += "<img src=\"" + src + "\" alt=\"" + alt + "\">";
                    i = end + 1;
                    continue;
                }
            }
        }
        if (text[i] == '`') {
            uint count = 1;
            while (i + count < text.length() && text[i + count] == '`') ++count;
            QString delimiter = text.mid(i, count);
            int end = text.find(delimiter, i + count);
            while (end >= 0 && end + count < text.length() && text[end + count] == '`') {
                int next = end + count;
                while (next < (int)text.length() && text[next] == '`') ++next;
                end = text.find(delimiter, next);
            }
            if (end > (int)i) {
                out += "<tt>" + escape(text.mid(i + count, end - i - count)) + "</tt>";
                i = end + count;
                continue;
            }
            out += delimiter;
            i += count;
            continue;
        }
        if (text[i] == '[') {
            int close = text.find("](", i + 1);
            if (close > (int)i) {
                int end = text.find(')', close + 2);
                if (end > close) {
                    out += "<a href=\"" + escape(text.mid(close + 2, end - close - 2)) + "\">" + escape(text.mid(i + 1, close - i - 1)) + "</a>";
                    i = end + 1;
                    continue;
                }
            }
        }
        if (i + 1 < text.length() && text[i] == '~' && text[i + 1] == '~') {
            int end = text.find("~~", i + 2);
            if (end > (int)i) {
                out += "<strike>" + escape(text.mid(i + 2, end - i - 2)) + "</strike>";
                i = end + 2;
                continue;
            }
        }
        if (i + 1 < text.length() && text[i] == '*' && text[i + 1] == '*') {
            int end = text.find("**", i + 2);
            if (end > (int)i) {
                out += "<b>" + escape(text.mid(i + 2, end - i - 2)) + "</b>";
                i = end + 2;
                continue;
            }
        }
        if (text[i] == '*') {
            int end = text.find('*', i + 1);
            if (end > (int)i) {
                out += "<i>" + escape(text.mid(i + 1, end - i - 1)) + "</i>";
                i = end + 1;
                continue;
            }
        }
        out += escape(text.mid(i, 1));
        ++i;
    }
    return out;
}

bool MdParser::isTaskLine(const QString &line, int *boxPos, bool *checked)
{
    int p = 0;
    while (p < (int)line.length() && line[p].isSpace()) ++p;
    if (p + 1 >= (int)line.length()
        || (line[p] != '-' && line[p] != '*' && line[p] != '+')
        || !line[p + 1].isSpace()) return false;
    p += 2;
    while (p < (int)line.length() && line[p].isSpace()) ++p;
    QString box = line.mid(p, 3);
    if (box != "[ ]" && box != "[x]" && box != "[X]") return false;
    if (p + 3 < (int)line.length() && !line[p + 3].isSpace()) return false;
    if (checked) *checked = box != "[ ]";
    if (boxPos)
        *boxPos = p;
    return true;
}

QString MdParser::toRichText(const QString &markdown, QValueList<MdBlockMap> *map)
{
    QStringList lines = QStringList::split('\n', markdown, true);
    QString html = "<html><body>";
    bool inCode = false;
    bool inIndentedCode = false;
    QChar fenceChar = '`';
    int fenceLength = 0;
    int fenceIndent = 0;
    int richBlock = 0;
    bool inUl = false;
    bool inOl = false;
    if (map) map->clear();

    for (uint i = 0; i < lines.count(); ++i) {
        QString line = lines[i];
        MdBlockMap bm;
        bm.richBlock = richBlock++;
        bm.firstLine = (int)i;
        bm.lastLine = (int)i;
        if (map)
            map->append(bm);

        if (line.right(1) == "\r") line.truncate(line.length() - 1);
        int indent = 0;
        while (indent < (int)line.length() && line[indent] == ' ') ++indent;
        QString trimmed = line.mid(indent);
        bool indented = indent >= 4 || line.left(1) == "\t";
        if (inIndentedCode) {
            if (indented || trimmed.isEmpty()) {
                html += escape(line.mid(indent >= 4 ? 4 : (line.left(1) == "\t" ? 1 : 0))) + "\n";
                continue;
            }
            html += "</pre>";
            inIndentedCode = false;
        }
        int fence = 0;
        if (indent <= 3 && !trimmed.isEmpty()
            && (trimmed[0] == '`' || trimmed[0] == '~')) {
            while (fence < (int)trimmed.length() && trimmed[fence] == trimmed[0]) ++fence;
        }
        if (inCode) {
            if (fence >= fenceLength && trimmed[0] == fenceChar
                && trimmed.mid(fence).stripWhiteSpace().isEmpty()) {
                html += "</pre>";
                inCode = false;
            } else {
                int remove = indent < fenceIndent ? indent : fenceIndent;
                html += escape(line.mid(remove)) + "\n";
            }
            continue;
        }
        if (fence >= 3 && (trimmed[0] != '`' || trimmed.mid(fence).find('`') < 0)) {
            if (inUl) {
                html += "</ul>";
                inUl = false;
            }
            if (inOl) {
                html += "</ol>";
                inOl = false;
            }
            inCode = true;
            fenceChar = trimmed[0];
            fenceLength = fence;
            fenceIndent = indent;
            html += "<pre>";
            continue;
        }
        if (indented && !inUl && !inOl) {
            html += "<pre>" + escape(line.mid(indent >= 4 ? 4 : 1)) + "\n";
            inIndentedCode = true;
            continue;
        }
        if (line.isEmpty()) {
            if (inUl) {
                html += "</ul>";
                inUl = false;
            }
            if (inOl) {
                html += "</ol>";
                inOl = false;
            }
            html += "<br>";
            continue;
        }

        int boxPos = -1;
        bool checked = false;
        if (isTaskLine(line, &boxPos, &checked)) {
            if (inUl) {
                html += "</ul>";
                inUl = false;
            }
            if (inOl) {
                html += "</ol>";
                inOl = false;
            }
            QString before = escape(line.left(boxPos));
            QString after = inlineRich(line.mid(boxPos + 3).stripWhiteSpace());
            QString mark = checked ? "[x]" : "[ ]";
            QString open = checked ? "<font color=\"#808080\">" : "";
            QString close = checked ? "</font>" : "";
            html += "<p>" + before + "<a href=\"toggle:" + QString::number(i) + "\">" + mark + "</a> " + open + after + close + "</p>";
            continue;
        }
        if (line[0] == '#') {
            if (inUl) {
                html += "</ul>";
                inUl = false;
            }
            if (inOl) {
                html += "</ol>";
                inOl = false;
            }
            int level = 0;
            while (level < (int)line.length() && line[level] == '#')
                ++level;
            if (level > 0 && level <= 6) {
                html += "<h" + QString::number(level) + ">" + inlineRich(line.mid(level).stripWhiteSpace()) + "</h" + QString::number(level) + ">";
                continue;
            }
        }
        if (line.left(1) == ">") {
            if (inUl) {
                html += "</ul>";
                inUl = false;
            }
            if (inOl) {
                html += "</ol>";
                inOl = false;
            }
            html += "<p><font color=\"#606060\">" + inlineRich(line.mid(1).stripWhiteSpace()) + "</font></p>";
            continue;
        }
        if (line == "---" || line == "***") {
            if (inUl) {
                html += "</ul>";
                inUl = false;
            }
            if (inOl) {
                html += "</ol>";
                inOl = false;
            }
            html += "<hr>";
            continue;
        }
        QString stripped = line.stripWhiteSpace();
        if (stripped.left(2) == "- " || stripped.left(2) == "* ") {
            if (inOl) {
                html += "</ol>";
                inOl = false;
            }
            if (!inUl) {
                html += "<ul>";
                inUl = true;
            }
            html += "<li>" + inlineRich(stripped.mid(2)) + "</li>";
            continue;
        }
        int dot = stripped.find(". ");
        if (dot > 0 && dot < 4) {
            bool digits = true;
            for (int d = 0; d < dot; ++d) {
                if (stripped[d] < '0' || stripped[d] > '9')
                    digits = false;
            }
            if (digits) {
                if (inUl) {
                    html += "</ul>";
                    inUl = false;
                }
                if (!inOl) {
                    html += "<ol>";
                    inOl = true;
                }
                html += "<li>" + inlineRich(stripped.mid(dot + 2)) + "</li>";
                continue;
            }
        }
        if (inUl) {
            html += "</ul>";
            inUl = false;
        }
        if (inOl) {
            html += "</ol>";
            inOl = false;
        }
        html += "<p>" + inlineRich(line) + "</p>";
    }

    if (inUl)
        html += "</ul>";
    if (inOl)
        html += "</ol>";
    if (inCode || inIndentedCode)
        html += "</pre>";
    html += "</body></html>";
    return html;
}

bool MdParser::toggleTaskLine(QString *markdown, int lineNumber)
{
    if (!markdown || lineNumber < 0)
        return false;
    QStringList lines = QStringList::split('\n', *markdown, true);
    if (lineNumber >= (int)lines.count())
        return false;
    QString line = lines[lineNumber];
    int p = line.find("[ ]");
    if (p >= 0) {
        line.replace(p, 3, "[x]");
    } else {
        p = line.find("[x]");
        if (p < 0)
            p = line.find("[X]");
        if (p < 0)
            return false;
        line.replace(p, 3, "[ ]");
    }
    lines[lineNumber] = line;
    *markdown = lines.join("\n");
    return true;
}
