#include "TextPrompt.h"

#include <qlineedit.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qhbox.h>
#include <qvbox.h>

TextPrompt::TextPrompt(const QString &title, const QString &label, const QString &value, QWidget *parent)
    : QDialog(parent, 0, true)
{
    setCaption(title);
    QVBox *box = new QVBox(this);
    new QLabel(label, box);
    edit = new QLineEdit(box);
    edit->setText(value);
    QHBox *buttons = new QHBox(box);
    QPushButton *ok = new QPushButton("OK", buttons);
    QPushButton *cancel = new QPushButton("Cancel", buttons);
    connect(ok, SIGNAL(clicked()), this, SLOT(accept()));
    connect(cancel, SIGNAL(clicked()), this, SLOT(reject()));
    edit->setFocus();
}

QString TextPrompt::text() const
{
    return edit->text();
}

QString TextPrompt::getText(const QString &title, const QString &label, const QString &value, bool *ok, QWidget *parent)
{
    TextPrompt prompt(title, label, value, parent);
    int result = prompt.exec();
    if (ok)
        *ok = result == QDialog::Accepted;
    return prompt.text();
}

