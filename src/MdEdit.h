#ifndef MDEDIT_H
#define MDEDIT_H

#include <qmultilineedit.h>

class MdEdit : public QMultiLineEdit {
    Q_OBJECT
public:
    MdEdit(QWidget *parent = 0, const char *name = 0);
    void setFont(const QFont &font);
    int logicalLineCount() const;
    QString logicalLine(int line) const;
    void logicalCursor(int *line, int *col) const;
    void setLogicalCursor(int line, int col);
    void selectLogical(int line1, int col1, int line2, int col2);

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
        if (!getMarkedRegion(line1, col1, line2, col2))
            return false;
        toLogical(line1, col1);
        toLogical(line2, col2);
        return true;
    }

    void deleteForward()
    {
        del();
    }

signals:
    void requestIndent();
    void requestOutdent();
    void requestNewLine();

public slots:
    void toggleLineNumbers();

private slots:
    void updateLineNumberMargin();

protected:
    void keyPressEvent(QKeyEvent *event);
    void paintCell(QPainter *painter, int row, int col);

private:
    void toLogical(int *row, int *col) const;
    void toVisual(int *row, int *col) const;
    bool lineNumbers;
};

#endif
