#include "MdView.h"
#include <qstylesheet.h>

MdView::MdView(QWidget *parent, const char *name)
    : QTextBrowser(parent, name)
{
    QStyleSheet *sheet = new QStyleSheet(this);
    sheet->item("pre")->setFontFamily("song");
    sheet->item("tt")->setFontFamily("song");
    sheet->item("code")->setFontFamily("song");
    setStyleSheet(sheet);
}

void MdView::setSource(const QString &name)
{
    if (name.left(7) == "toggle:") {
        emit toggleTask(name.mid(7).toInt());
        return;
    }
}
