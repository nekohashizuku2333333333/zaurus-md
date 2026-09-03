#ifndef MDEDIT_H
#define MDEDIT_H

#include <qmultilineedit.h>

class MdEdit : public QMultiLineEdit {
    Q_OBJECT
public:
    MdEdit(QWidget *parent = 0, const char *name = 0)
        : QMultiLineEdit(parent, name) {}

    bool hasSelection() const
    {
        return hasMarkedText();
    }

    QString selectionText() const
    {
        return markedText();
    }

    bool selectionRegion(int *line1, int *col1, int *line2, int *col2) const
    {
        return getMarkedRegion(line1, col1, line2, col2);
    }

    void deleteForward()
    {
        del();
    }

signals:
    void requestIndent();
    void requestOutdent();
    void requestNewLine();

protected:
    void keyPressEvent(QKeyEvent *event);
};

#endif
