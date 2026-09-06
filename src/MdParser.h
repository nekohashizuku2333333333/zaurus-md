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

};

#endif
