#include "MdEdit.h"

#include <qevent.h>
#include <qpainter.h>
#include <qfontmetrics.h>

MdEdit::MdEdit(QWidget *parent, const char *name)
    : QMultiLineEdit(parent, name), lineNumbers(false)
{
    setWrapPolicy(Anywhere);
    setWordWrap(WidgetWidth);
    connect(this, SIGNAL(textChanged()), this, SLOT(updateLineNumberMargin()));
}

void MdEdit::setFont(const QFont &font)
{
    QString digits = QString::number(logicalLineCount());
    if (digits.length() < 3) digits = "000";
    // Qt2 setHMargin alone does not reflow; setFont reflows after the margin changes.
    setHMargin(lineNumbers ? QFontMetrics(font).width(digits) + 8 : 3);
    QMultiLineEdit::setFont(font);
}

void MdEdit::updateLineNumberMargin()
{
    if (!lineNumbers) return;
    QString digits = QString::number(logicalLineCount());
    if (digits.length() < 3) digits = "000";
    if (hMargin() != fontMetrics().width(digits) + 8)
        setFont(font());
}

void MdEdit::toggleLineNumbers()
{
    lineNumbers = !lineNumbers;
    setFont(font());
    update();
}

// Qt2 exposes visual rows. Commands operate on source lines, including wrapped text.
void MdEdit::toLogical(int *row, int *col) const
{
    int line = 0, offset = 0;
    for (int r = 0; r < *row; ++r) {
        if (isEndOfParagraph(r)) { ++line; offset = 0; }
        else offset += QMultiLineEdit::textLine(r).length();
    }
    *row = line;
    *col += offset;
}

void MdEdit::toVisual(int *row, int *col) const
{
    int r = 0, line = 0;
    while (r < QMultiLineEdit::numLines() - 1 && line < *row) {
        if (isEndOfParagraph(r)) ++line;
        ++r;
    }
    while (r < QMultiLineEdit::numLines() - 1 && !isEndOfParagraph(r)
           && *col >= (int)QMultiLineEdit::textLine(r).length()) {
        *col -= QMultiLineEdit::textLine(r).length();
        ++r;
    }
    *row = r;
}

int MdEdit::logicalLineCount() const
{
    int row = QMultiLineEdit::numLines() - 1, col = 0;
    toLogical(&row, &col);
    return row + 1;
}

QString MdEdit::logicalLine(int line) const
{
    int row = line, col = 0;
    toVisual(&row, &col);
    QString result;
    do {
        result += QMultiLineEdit::textLine(row);
        if (isEndOfParagraph(row)) break;
    } while (++row < QMultiLineEdit::numLines());
    return result;
}

void MdEdit::logicalCursor(int *line, int *col) const
{
    getCursorPosition(line, col);
    toLogical(line, col);
}

void MdEdit::setLogicalCursor(int line, int col)
{
    toVisual(&line, &col);
    setCursorPosition(line, col);
}

void MdEdit::selectLogical(int line1, int col1, int line2, int col2)
{
    toVisual(&line1, &col1);
    toVisual(&line2, &col2);
    setSelection(line1, col1, line2, col2);
}

void MdEdit::paintCell(QPainter *painter, int row, int col)
{
    QMultiLineEdit::paintCell(painter, row, col);
    if (!lineNumbers) return;
    painter->save();
    painter->fillRect(0, 0, hMargin() - 3, cellHeight(), colorGroup().background());
    painter->setPen(colorGroup().text());
    if (row == 0 || isEndOfParagraph(row - 1)) {
        int line = row, column = 0;
        toLogical(&line, &column);
        painter->drawText(0, 0, hMargin() - 7, cellHeight(),
                          AlignRight | AlignVCenter, QString::number(line + 1));
    }
    painter->restore();
}

void MdEdit::keyPressEvent(QKeyEvent *event)
{
    if (!event) {
        QMultiLineEdit::keyPressEvent(event);
        return;
    }

    if (event->key() == Key_Tab) {
        if (event->state() & ShiftButton)
            emit requestOutdent();
        else
            emit requestIndent();
        return;
    }

    if (event->key() == Key_Return || event->key() == Key_Enter) {
        emit requestNewLine();
        return;
    }

    QMultiLineEdit::keyPressEvent(event);
}
