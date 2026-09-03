#ifndef MDVIEW_H
#define MDVIEW_H

#include <qtextbrowser.h>

class MdView : public QTextBrowser {
    Q_OBJECT
public:
    MdView(QWidget *parent = 0, const char *name = 0);

signals:
    void toggleTask(int lineNumber);

public slots:
    virtual void setSource(const QString &name);
};

#endif

