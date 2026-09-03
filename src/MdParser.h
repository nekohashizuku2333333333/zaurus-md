#ifndef MDPARSER_H
#define MDPARSER_H

#include <qstring.h>
#include <qvaluelist.h>

struct MdBlockMap {
    int richBlock;
    int firstLine;
    int lastLine;
};

class MdParser {
public:
    static QString toRichText(const QString &markdown, QValueList<MdBlockMap> *map);
    static bool toggleTaskLine(QString *markdown, int lineNumber);

private:
    static QString escape(const QString &text);
    static QString inlineRich(const QString &text);
    static bool isTaskLine(const QString &line, int *boxPos, bool *checked);
};

#endif

