#include "MdEdit.h"

#include <qevent.h>

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
