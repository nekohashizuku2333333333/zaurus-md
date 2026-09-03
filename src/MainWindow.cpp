#include "MainWindow.h"

#include "FileUtil.h"
#include "MdEdit.h"
#include "MdParser.h"
#include "MdView.h"

#include <qdir.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qlistview.h>
#include <qmessagebox.h>
#include <qmultilineedit.h>
#include <qpushbutton.h>
#include <qstatusbar.h>
#include <qdatetime.h>
#include <qtoolbar.h>
#include <qtoolbutton.h>
#include <qvbox.h>
#include <qhbox.h>
#include <qwidgetstack.h>

MainWindow::MainWindow(QWidget *parent, const char *name)
    : QMainWindow(parent, name),
      notesDir("/home/zaurus/Documents/Notes"),
      currentDir(notesDir),
      browser(0),
      stack(0),
      editor(0),
      view(0),
      modeButton(0)
{
    FileUtil::ensureDir(notesDir);
    buildUi();
    loadDirectory(notesDir);
}

void MainWindow::buildUi()
{
    QVBox *root = new QVBox(this);
    setCentralWidget(root);

    QHBox *top = new QHBox(root);
    QPushButton *back = new QPushButton("<", top);
    connect(back, SIGNAL(clicked()), this, SLOT(showBrowser()));
    QPushButton *up = new QPushButton("Up", top);
    connect(up, SIGNAL(clicked()), this, SLOT(goUp()));
    QPushButton *fresh = new QPushButton("New", top);
    connect(fresh, SIGNAL(clicked()), this, SLOT(newFile()));
    modeButton = new QPushButton("View", top);
    connect(modeButton, SIGNAL(clicked()), this, SLOT(showView()));
    QPushButton *save = new QPushButton("Save", top);
    connect(save, SIGNAL(clicked()), this, SLOT(saveFile()));

    stack = new QWidgetStack(root);
    browser = new QListView(stack);
    browser->addColumn("Name");
    browser->addColumn("Modified");
    connect(browser, SIGNAL(doubleClicked(QListViewItem *)), this, SLOT(openSelected(QListViewItem *)));

    editor = new MdEdit(stack);
    view = new MdView(stack);
    connect(view, SIGNAL(toggleTask(int)), this, SLOT(toggleTask(int)));

    stack->addWidget(browser, 0);
    stack->addWidget(editor, 1);
    stack->addWidget(view, 2);

    QHBox *bar1 = new QHBox(root);
    makeButton(bar1, "B", SLOT(wrapBold()));
    makeButton(bar1, "I", SLOT(wrapItalic()));
    makeButton(bar1, "`", SLOT(wrapCode()));
    makeButton(bar1, "H", SLOT(cycleHeading()));
    makeButton(bar1, "-", SLOT(toggleBullet()));
    makeButton(bar1, "1.", SLOT(toggleNumber()));
    makeButton(bar1, "[ ]", SLOT(toggleTaskCurrent()));
    makeButton(bar1, ">", SLOT(quoteLine()));
    makeButton(bar1, "Link", SLOT(insertLink()));

    QHBox *bar2 = new QHBox(root);
    makeButton(bar2, "---", SLOT(insertRule()));
    makeButton(bar2, ">>", SLOT(indentLine()));
    makeButton(bar2, "<<", SLOT(outdentLine()));
    makeButton(bar2, "Date", SLOT(insertDate()));
    makeButton(bar2, "Undo", SLOT(undoEdit()));
    makeButton(bar2, "Redo", SLOT(redoEdit()));
    makeButton(bar2, "Edit", SLOT(showEditor()));
    makeButton(bar2, "View", SLOT(showView()));

    stack->raiseWidget(browser);
}

QPushButton *MainWindow::makeButton(QWidget *parent, const char *text, const char *slot)
{
    QPushButton *button = new QPushButton(text, parent);
    connect(button, SIGNAL(clicked()), this, slot);
    return button;
}

void MainWindow::loadDirectory(const QString &path)
{
    currentDir = path;
    browser->clear();
    if (path != notesDir)
        new QListViewItem(browser, "..", "");
    new QListViewItem(browser, "QuickNote.md", "");
    new QListViewItem(browser, "todo.txt", "");

    QDir dir(path);
    dir.setFilter(QDir::Dirs | QDir::Files | QDir::NoSymLinks);
    dir.setSorting(QDir::DirsFirst | QDir::Name);
    const QFileInfoList *infos = dir.entryInfoList();
    if (!infos)
        return;
    QFileInfoListIterator it(*infos);
    QFileInfo *fi;
    while ((fi = it.current()) != 0) {
        ++it;
        QString name = fi->fileName();
        if (name == "." || name == "..")
            continue;
        if (fi->isFile() && !(name.right(3) == ".md" || name.right(4) == ".txt"))
            continue;
        new QListViewItem(browser, name, fi->lastModified().toString());
    }
}

void MainWindow::openSelected(QListViewItem *item)
{
    if (!item)
        return;
    QString path = currentDir + "/" + item->text(0);
    if (item->text(0) == "..") {
        goUp();
        return;
    }
    QFileInfo fi(path);
    if (fi.isDir()) {
        loadDirectory(path);
        return;
    }
    openFile(path);
}

void MainWindow::openFile(const QString &path)
{
    currentFile = path;
    QString text;
    if (!QFileInfo(path).exists())
        FileUtil::writeUtf8Atomic(path, "");
    FileUtil::readUtf8(path, &text);
    editor->setText(text);
    showEditor();
}

void MainWindow::showBrowser()
{
    if (!currentFile.isEmpty())
        saveFile();
    stack->raiseWidget(browser);
}

void MainWindow::showEditor()
{
    stack->raiseWidget(editor);
    modeButton->setText("View");
    disconnect(modeButton, SIGNAL(clicked()), this, SLOT(showEditor()));
    connect(modeButton, SIGNAL(clicked()), this, SLOT(showView()));
}

void MainWindow::showView()
{
    saveFile();
    refreshView();
    stack->raiseWidget(view);
    modeButton->setText("Edit");
    disconnect(modeButton, SIGNAL(clicked()), this, SLOT(showView()));
    connect(modeButton, SIGNAL(clicked()), this, SLOT(showEditor()));
}

void MainWindow::refreshView()
{
    QValueList<MdBlockMap> map;
    view->setText(MdParser::toRichText(editor->text(), &map));
}

void MainWindow::saveFile()
{
    if (currentFile.isEmpty())
        return;
    FileUtil::writeUtf8Atomic(currentFile, editor->text());
    statusBar()->message("Saved", 1000);
}

void MainWindow::toggleTask(int lineNumber)
{
    QString text = editor->text();
    if (MdParser::toggleTaskLine(&text, lineNumber)) {
        editor->setText(text);
        saveFile();
        refreshView();
    }
}

void MainWindow::insertTask()
{
    editor->insert("- [ ] ");
}

void MainWindow::newFile()
{
    QString name;
    QString path;
    for (int i = 1; i < 1000; ++i) {
        name = "note" + QString::number(i) + ".md";
        path = currentDir + "/" + name;
        if (!QFileInfo(path).exists())
            break;
    }
    FileUtil::writeUtf8Atomic(path, "# " + name + "\n\n");
    loadDirectory(currentDir);
    openFile(path);
}

void MainWindow::goUp()
{
    if (stack && stack->visibleWidget() != browser) {
        showBrowser();
        return;
    }
    if (currentDir == notesDir)
        return;
    QDir d(currentDir);
    d.cdUp();
    QString next = d.absPath();
    if (next.left(notesDir.length()) != notesDir)
        next = notesDir;
    loadDirectory(next);
}

QString MainWindow::selectedText() const
{
    if (editor->hasSelection())
        return editor->selectionText();
    return QString::null;
}

void MainWindow::replaceSelectionOrInsert(const QString &text)
{
    int l1, c1, l2, c2;
    if (editor->selectionRegion(&l1, &c1, &l2, &c2)) {
        editor->deleteForward();
        editor->insertAt(text, l1, c1);
        editor->setCursorPosition(l1, c1 + text.length());
        return;
    }
    editor->insert(text);
}

void MainWindow::wrapSelection(const QString &before, const QString &after)
{
    QString sel = selectedText();
    if (sel.isNull()) {
        int line, col;
        editor->getCursorPosition(&line, &col);
        editor->insert(before + after);
        editor->setCursorPosition(line, col + before.length());
    } else {
        replaceSelectionOrInsert(before + sel + after);
    }
    touchEditor();
}

void MainWindow::setCurrentLineText(int lineNo, const QString &line)
{
    editor->removeLine(lineNo);
    editor->insertLine(line, lineNo);
}

void MainWindow::replaceCurrentLine(const QString &line)
{
    int row, col;
    editor->getCursorPosition(&row, &col);
    setCurrentLineText(row, line);
    if (col > (int)line.length())
        col = line.length();
    editor->setCursorPosition(row, col);
    touchEditor();
}

void MainWindow::applyLinePrefix(const QString &prefix, bool numbered)
{
    int row, col;
    editor->getCursorPosition(&row, &col);
    QString line = editor->textLine(row);
    QString stripped = line.stripWhiteSpace();
    if (numbered) {
        if (stripped.left(3) == "1. ")
            replaceCurrentLine(stripped.mid(3));
        else
            replaceCurrentLine("1. " + stripped);
        return;
    }
    if (stripped.left(prefix.length()) == prefix)
        replaceCurrentLine(stripped.mid(prefix.length()));
    else
        replaceCurrentLine(prefix + stripped);
}

void MainWindow::touchEditor()
{
    editor->setFocus();
}

void MainWindow::wrapBold()
{
    wrapSelection("**", "**");
}

void MainWindow::wrapItalic()
{
    wrapSelection("*", "*");
}

void MainWindow::wrapCode()
{
    wrapSelection("`", "`");
}

void MainWindow::cycleHeading()
{
    int row, col;
    editor->getCursorPosition(&row, &col);
    QString line = editor->textLine(row).stripWhiteSpace();
    if (line.left(4) == "### ")
        line = line.mid(4);
    else if (line.left(3) == "## ")
        line = "### " + line.mid(3);
    else if (line.left(2) == "# ")
        line = "## " + line.mid(2);
    else
        line = "# " + line;
    replaceCurrentLine(line);
}

void MainWindow::toggleBullet()
{
    applyLinePrefix("- ", false);
}

void MainWindow::toggleNumber()
{
    applyLinePrefix("1. ", true);
}

void MainWindow::toggleTaskCurrent()
{
    int row, col;
    editor->getCursorPosition(&row, &col);
    QString line = editor->textLine(row);
    int p = line.find("[ ]");
    if (p >= 0) {
        line.replace(p, 3, "[x]");
    } else {
        p = line.find("[x]");
        if (p < 0)
            p = line.find("[X]");
        if (p >= 0)
            line.replace(p, 3, "[ ]");
        else
            line = "- [ ] " + line.stripWhiteSpace();
    }
    replaceCurrentLine(line);
}

void MainWindow::quoteLine()
{
    applyLinePrefix("> ", false);
}

void MainWindow::insertLink()
{
    QString sel = selectedText();
    if (sel.isNull())
        sel = "text";
    replaceSelectionOrInsert("[" + sel + "](url)");
    touchEditor();
}

void MainWindow::insertRule()
{
    replaceSelectionOrInsert("\n---\n");
    touchEditor();
}

void MainWindow::indentLine()
{
    int row, col;
    editor->getCursorPosition(&row, &col);
    replaceCurrentLine("    " + editor->textLine(row));
}

void MainWindow::outdentLine()
{
    int row, col;
    editor->getCursorPosition(&row, &col);
    QString line = editor->textLine(row);
    if (line.left(4) == "    ")
        line = line.mid(4);
    else if (line.left(1) == "\t")
        line = line.mid(1);
    replaceCurrentLine(line);
}

void MainWindow::insertDate()
{
    replaceSelectionOrInsert(QDate::currentDate().toString());
    touchEditor();
}

void MainWindow::undoEdit()
{
    editor->undo();
    touchEditor();
}

void MainWindow::redoEdit()
{
    editor->redo();
    touchEditor();
}
