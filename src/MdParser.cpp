#include "MdParser.h"
#include "MdRichText.h"
#include <qcstring.h>
#include <stdlib.h>

QString MdParser::toRichText(const QString &markdown, QValueList<MdBlockMap> *map)
{
    QCString utf8 = markdown.utf8();
    char *html = 0;
    unsigned size = utf8.size() ? utf8.size() - 1 : 0;
    if (map) {
        map->clear();
        int line = 0;
        for (uint i = 0; i <= markdown.length(); ++i) {
            if (i == markdown.length() || markdown[i] == '\r' || markdown[i] == '\n') {
                MdBlockMap bm;
                bm.richBlock = line; bm.firstLine = line; bm.lastLine = line;
                map->append(bm);
                ++line;
                if (i + 1 < markdown.length() && markdown[i] == '\r' && markdown[i + 1] == '\n') ++i;
            }
        }
    }
    int result = md_rich_text(size ? utf8.data() : "", size, &html);
    if (result != 0)
        return "<html><body><p>Preview unavailable: document exceeds rendering limits or memory is insufficient.</p></body></html>";
    QString rich = QString::fromUtf8(html);
    free(html);
    return rich;
}

bool MdParser::toggleTaskLine(QString *markdown, int lineNumber)
{
    if (!markdown || lineNumber < 0) return false;
    QCString utf8 = markdown->utf8();
    unsigned offset, size = utf8.size() ? utf8.size() - 1 : 0;
    if (!md_task_offset(size ? utf8.data() : "", size, lineNumber, &offset)) return false;
    utf8.data()[offset] = utf8.data()[offset] == ' ' ? 'x' : ' ';
    *markdown = QString::fromUtf8(utf8.data(), size);
    return true;
}
