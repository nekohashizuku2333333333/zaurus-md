#include "MdView.h"

MdView::MdView(QWidget *parent, const char *name)
    : QTextBrowser(parent, name)
{
}

void MdView::setSource(const QString &name)
{
    if (name.left(7) == "toggle:") {
        emit toggleTask(name.mid(7).toInt());
        return;
    }
}

