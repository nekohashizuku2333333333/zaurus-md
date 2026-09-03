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
        if (text[i] == '`') {
            int end = text.find('`', i + 1);
            if (end > (int)i) {
                out += "<font color=\"#5b5b5b\"><tt>" + escape(text.mid(i + 1, end - i - 1)) + "</tt></font>";
                i = end + 1;
                continue;
            }
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
    int p = line.find("[ ]");
    if (p < 0) {
        p = line.find("[x]");
        if (p < 0)
            p = line.find("[X]");
        if (p < 0)
            return false;
        if (checked)
            *checked = true;
    } else if (checked) {
        *checked = false;
    }
    if (boxPos)
        *boxPos = p;
    return true;
}

QString MdParser::toRichText(const QString &markdown, QValueList<MdBlockMap> *map)
{
    QStringList lines = QStringList::split('\n', markdown, true);
    QString html = "<html><body>";
    bool inCode = false;
    int richBlock = 0;

    for (uint i = 0; i < lines.count(); ++i) {
        QString line = lines[i];
        MdBlockMap bm;
        bm.richBlock = richBlock++;
        bm.firstLine = (int)i;
        bm.lastLine = (int)i;
        if (map)
            map->append(bm);

        if (line.left(3) == "```") {
            inCode = !inCode;
            html += "<pre>" + escape(line) + "</pre>";
            continue;
        }
        if (inCode) {
            html += "<pre>" + escape(line) + "</pre>";
            continue;
        }
        if (line.isEmpty()) {
            html += "<br>";
            continue;
        }

        int boxPos = -1;
        bool checked = false;
        if (isTaskLine(line, &boxPos, &checked)) {
            QString before = escape(line.left(boxPos));
            QString after = inlineRich(line.mid(boxPos + 3).stripWhiteSpace());
            QString mark = checked ? "[x]" : "[ ]";
            QString open = checked ? "<font color=\"#808080\">" : "";
            QString close = checked ? "</font>" : "";
            html += "<p>" + before + "<a href=\"toggle:" + QString::number(i) + "\">" + mark + "</a> " + open + after + close + "</p>";
            continue;
        }
        if (line[0] == '#') {
            int level = 0;
            while (level < (int)line.length() && line[level] == '#')
                ++level;
            if (level > 0 && level <= 6) {
                html += "<h" + QString::number(level) + ">" + inlineRich(line.mid(level).stripWhiteSpace()) + "</h" + QString::number(level) + ">";
                continue;
            }
        }
        if (line.left(1) == ">") {
            html += "<p><font color=\"#606060\">" + inlineRich(line.mid(1).stripWhiteSpace()) + "</font></p>";
            continue;
        }
        if (line == "---" || line == "***") {
            html += "<hr>";
            continue;
        }
        html += "<p>" + inlineRich(line) + "</p>";
    }

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
