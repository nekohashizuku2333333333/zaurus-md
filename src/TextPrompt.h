#ifndef TEXTPROMPT_H
#define TEXTPROMPT_H

#include <qdialog.h>
#include <qstring.h>

class QLineEdit;

class TextPrompt : public QDialog {
public:
    TextPrompt(const QString &title, const QString &label, const QString &value, QWidget *parent = 0);
    QString text() const;
    static QString getText(const QString &title, const QString &label, const QString &value, bool *ok, QWidget *parent = 0);

private:
    QLineEdit *edit;
};

#endif

