#include "MainWindow.h"

#include "FileUtil.h"
#include "MdEdit.h"
#include "MdParser.h"
#include "MdView.h"
#include "TextPrompt.h"
#include "TodoTxt.h"

#include <qdir.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qapplication.h>
#include <qclipboard.h>
#include <qlistview.h>
#include <qmessagebox.h>
#include <qmultilineedit.h>
#include <qpalette.h>
#include <qpushbutton.h>
#include <qscrollview.h>
#include <qstatusbar.h>
#include <qdatetime.h>
#include <qtimer.h>
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
      modeButton(0),
      autosaveTimer(0),
      todoFilter(""),
      darkTheme(false),
      hideDone(false),
      fontSize(12)
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
    QPushButton *folder = new QPushButton("Dir", top);
    connect(folder, SIGNAL(clicked()), this, SLOT(newFolder()));
    QPushButton *saveAs = new QPushButton("As", top);
    connect(saveAs, SIGNAL(clicked()), this, SLOT(saveAsFile()));
    QPushButton *ren = new QPushButton("Ren", top);
    connect(ren, SIGNAL(clicked()), this, SLOT(renameFile()));
    QPushButton *del = new QPushButton("Del", top);
    connect(del, SIGNAL(clicked()), this, SLOT(deleteFile()));
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
    editor->setUndoEnabled(true);
    connect(editor, SIGNAL(textChanged()), this, SLOT(autosaveTick()));
    connect(editor, SIGNAL(requestIndent()), this, SLOT(indentLine()));
    connect(editor, SIGNAL(requestOutdent()), this, SLOT(outdentLine()));
    connect(editor, SIGNAL(requestNewLine()), this, SLOT(smartNewLine()));
    view = new MdView(stack);
    connect(view, SIGNAL(toggleTask(int)), this, SLOT(toggleTask(int)));

    stack->addWidget(browser, 0);
    stack->addWidget(editor, 1);
    stack->addWidget(view, 2);

    QScrollView *tools = new QScrollView(root);
    tools->setVScrollBarMode(QScrollView::AlwaysOff);
    tools->setHScrollBarMode(QScrollView::Auto);
    QHBox *bar = new QHBox(tools->viewport());
    tools->addChild(bar);
    makeButton(bar, "B", SLOT(wrapBold()));
    makeButton(bar, "I", SLOT(wrapItalic()));
    makeButton(bar, "`", SLOT(wrapCode()));
    makeButton(bar, "H", SLOT(cycleHeading()));
    makeButton(bar, "-", SLOT(toggleBullet()));
    makeButton(bar, "1.", SLOT(toggleNumber()));
    makeButton(bar, "[ ]", SLOT(toggleTaskCurrent()));
    makeButton(bar, ">", SLOT(quoteLine()));
    makeButton(bar, "Link", SLOT(insertLink()));
    makeButton(bar, "Find", SLOT(findText()));
    makeButton(bar, "Next", SLOT(findNext()));
    makeButton(bar, "R1", SLOT(replaceOne()));
    makeButton(bar, "All", SLOT(replaceText()));
    makeButton(bar, "---", SLOT(insertRule()));
    makeButton(bar, ">>", SLOT(indentLine()));
    makeButton(bar, "<<", SLOT(outdentLine()));
    makeButton(bar, "Up", SLOT(moveLineUp()));
    makeButton(bar, "Dn", SLOT(moveLineDown()));
    makeButton(bar, "Date", SLOT(insertDate()));
    makeButton(bar, "Time", SLOT(insertTime()));
    makeButton(bar, "Undo", SLOT(undoEdit()));
    makeButton(bar, "Redo", SLOT(redoEdit()));
    makeButton(bar, "Copy", SLOT(copyText()));
    makeButton(bar, "Cut", SLOT(cutText()));
    makeButton(bar, "Paste", SLOT(pasteText()));
    makeButton(bar, "AllSel", SLOT(selectAllText()));
    makeButton(bar, "Dup", SLOT(duplicateLine()));
    makeButton(bar, "Done", SLOT(todoToggleDone()));
    makeButton(bar, "A", SLOT(todoPriorityA()));
    makeButton(bar, "B", SLOT(todoPriorityB()));
    makeButton(bar, "C", SLOT(todoPriorityC()));
    makeButton(bar, "+", SLOT(todoProject()));
    makeButton(bar, "@", SLOT(todoContext()));
    makeButton(bar, "Due", SLOT(todoDue()));
    makeButton(bar, "Sort", SLOT(todoSortPriority()));
    makeButton(bar, "Hide", SLOT(toggleHideDone()));
    makeButton(bar, "F+", SLOT(filterTodoProject()));
    makeButton(bar, "F@", SLOT(filterTodoContext()));
    makeButton(bar, "FClr", SLOT(clearTodoFilter()));
    makeButton(bar, "End", SLOT(moveDoneTasksToEnd()));
    makeButton(bar, "Clr", SLOT(clearDoneTasks()));
    makeButton(bar, "A+", SLOT(fontBigger()));
    makeButton(bar, "A-", SLOT(fontSmaller()));
    makeButton(bar, "Theme", SLOT(toggleTheme()));
    makeButton(bar, "Edit", SLOT(showEditor()));
    makeButton(bar, "View", SLOT(showView()));
    bar->resize(1380, 28);
    tools->resizeContents(1380, 28);
    tools->setFixedHeight(44);

    autosaveTimer = new QTimer(this);
    connect(autosaveTimer, SIGNAL(timeout()), this, SLOT(saveFile()));
    applyFontSize();
    applyTheme();

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
    editor->setEdited(false);
    setCaption(QFileInfo(path).fileName());
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
    touchEditor();
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
    QString text = filteredPreviewText(editor->text());
    view->setText(MdParser::toRichText(text, &map));
}

void MainWindow::saveFile()
{
    if (currentFile.isEmpty())
        return;
    if (autosaveTimer)
        autosaveTimer->stop();
    FileUtil::writeUtf8Atomic(currentFile, editor->text());
    editor->setEdited(false);
    statusBar()->message("Saved", 1000);
}

void MainWindow::toggleTask(int lineNumber)
{
    if (lineNumber >= 0 && lineNumber < (int)previewLineMap.count())
        lineNumber = previewLineMap[lineNumber];
    QString text = editor->text();
    if (MdParser::toggleTaskLine(&text, lineNumber)) {
        editor->setText(text);
        editor->setEdited(true);
        saveFile();
        refreshView();
    }
}

void MainWindow::insertTask()
{
    editor->insert("- [ ] ");
    scheduleAutosave();
}

void MainWindow::newFile()
{
    bool ok = false;
    QString name = TextPrompt::getText("New file", "Name", "note.md", &ok, this);
    if (!ok || name.isEmpty())
        return;
    if (!(name.right(3) == ".md" || name.right(4) == ".txt"))
        name += ".md";
    QString path = currentDir + "/" + name;
    if (QFileInfo(path).exists()) {
        QMessageBox::warning(this, "New file", "File exists.");
        return;
    }
    FileUtil::writeUtf8Atomic(path, "# " + name + "\n\n");
    loadDirectory(currentDir);
    openFile(path);
}

void MainWindow::newFolder()
{
    bool ok = false;
    QString name = TextPrompt::getText("New folder", "Name", "", &ok, this);
    if (!ok || name.isEmpty())
        return;
    if (name.find('/') >= 0) {
        QMessageBox::warning(this, "New folder", "Invalid name.");
        return;
    }
    QString path = currentDir + "/" + name;
    if (QFileInfo(path).exists()) {
        QMessageBox::warning(this, "New folder", "Folder exists.");
        return;
    }
    if (!FileUtil::ensureDir(path))
        QMessageBox::warning(this, "New folder", "Cannot create folder.");
    loadDirectory(currentDir);
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

QString MainWindow::currentSelectedPath() const
{
    QListViewItem *item = browser->currentItem();
    if (!item)
        return QString::null;
    if (item->text(0) == "..")
        return QString::null;
    return currentDir + "/" + item->text(0);
}

void MainWindow::saveAsFile()
{
    if (currentFile.isEmpty())
        return;
    bool ok = false;
    QString name = TextPrompt::getText("Save as", "Name", QFileInfo(currentFile).fileName(), &ok, this);
    if (!ok || name.isEmpty())
        return;
    if (!(name.right(3) == ".md" || name.right(4) == ".txt"))
        name += ".md";
    QString path = currentDir + "/" + name;
    if (QFileInfo(path).exists()) {
        QMessageBox::warning(this, "Save as", "File exists.");
        return;
    }
    currentFile = path;
    saveFile();
    loadDirectory(currentDir);
    setCaption(name);
}

void MainWindow::renameFile()
{
    QString path = currentFile;
    if (stack && stack->visibleWidget() == browser)
        path = currentSelectedPath();
    if (path.isEmpty())
        return;
    QFileInfo info(path);
    bool ok = false;
    QString name = TextPrompt::getText("Rename", "Name", info.fileName(), &ok, this);
    if (!ok || name.isEmpty())
        return;
    QString next = info.dirPath(true) + "/" + name;
    if (QFileInfo(next).exists()) {
        QMessageBox::warning(this, "Rename", "File exists.");
        return;
    }
    if (QDir().rename(path, next)) {
        if (path == currentFile) {
            currentFile = next;
            setCaption(name);
        }
        loadDirectory(currentDir);
    }
}

void MainWindow::deleteFile()
{
    QString path = currentFile;
    if (stack && stack->visibleWidget() == browser)
        path = currentSelectedPath();
    if (path.isEmpty())
        return;
    int answer = QMessageBox::warning(this, "Delete", "Delete this file?", QMessageBox::Yes, QMessageBox::No);
    if (answer != QMessageBox::Yes)
        return;
    QFile::remove(path);
    if (path == currentFile)
        currentFile = QString::null;
    loadDirectory(currentDir);
    stack->raiseWidget(browser);
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
    scheduleAutosave();
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
    scheduleAutosave();
}

void MainWindow::setCurrentLineText(int lineNo, const QString &line)
{
    editor->removeLine(lineNo);
    editor->insertLine(line, lineNo);
}

void MainWindow::replaceAllText(const QString &text, int cursorLine)
{
    editor->setText(text);
    if (cursorLine < 0)
        cursorLine = 0;
    if (cursorLine >= editor->numLines())
        cursorLine = editor->numLines() - 1;
    editor->setCursorPosition(cursorLine, 0);
    touchEditor();
    scheduleAutosave();
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
    int first, last;
    if (selectedLineRange(&first, &last) && first != last) {
        applyLineRangePrefix(prefix, numbered);
        return;
    }

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

bool MainWindow::selectedLineRange(int *firstLine, int *lastLine) const
{
    int l1, c1, l2, c2;
    if (!editor->selectionRegion(&l1, &c1, &l2, &c2)) {
        editor->getCursorPosition(&l1, &c1);
        l2 = l1;
    } else {
        if (l2 < l1 || (l2 == l1 && c2 < c1)) {
            int tl = l1;
            l1 = l2;
            l2 = tl;
            int tc = c1;
            c1 = c2;
            c2 = tc;
        }
        if (c2 == 0 && l2 > l1)
            --l2;
    }
    if (l1 < 0)
        l1 = 0;
    if (l2 >= editor->numLines())
        l2 = editor->numLines() - 1;
    if (firstLine)
        *firstLine = l1;
    if (lastLine)
        *lastLine = l2;
    return true;
}

void MainWindow::applyLineRangePrefix(const QString &prefix, bool numbered)
{
    int first, last;
    selectedLineRange(&first, &last);
    for (int row = first; row <= last; ++row) {
        QString line = editor->textLine(row);
        QString stripped = line.stripWhiteSpace();
        if (numbered) {
            int dot = stripped.find(". ");
            if (dot > 0 && dot < 4) {
                bool digits = true;
                for (int i = 0; i < dot; ++i) {
                    if (stripped[i] < '0' || stripped[i] > '9')
                        digits = false;
                }
                if (digits)
                    line = stripped.mid(dot + 2);
                else
                    line = QString::number(row - first + 1) + ". " + stripped;
            } else {
                line = QString::number(row - first + 1) + ". " + stripped;
            }
        } else if (stripped.left(prefix.length()) == prefix) {
            line = stripped.mid(prefix.length());
        } else {
            line = prefix + stripped;
        }
        setCurrentLineText(row, line);
    }
    editor->setCursorPosition(first, 0);
    touchEditor();
    scheduleAutosave();
}

void MainWindow::transformLineRange(const QString &mode)
{
    int first, last;
    selectedLineRange(&first, &last);
    for (int row = first; row <= last; ++row) {
        QString line = editor->textLine(row);
        if (mode == "indent") {
            line = "    " + line;
        } else if (mode == "outdent") {
            if (line.left(4) == "    ")
                line = line.mid(4);
            else if (line.left(1) == "\t")
                line = line.mid(1);
        } else if (mode == "task") {
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
        } else if (mode == "priority") {
            line = line.stripWhiteSpace();
        }
        setCurrentLineText(row, line);
    }
    editor->setCursorPosition(first, 0);
    touchEditor();
    scheduleAutosave();
}

QString MainWindow::continuationForLine(const QString &line) const
{
    int p = 0;
    while (p < (int)line.length() && (line[p] == ' ' || line[p] == '\t'))
        ++p;
    QString indent = line.left(p);
    QString s = line.mid(p);
    if (s.left(6) == "- [ ] " || s.left(6) == "- [x] " || s.left(6) == "- [X] ")
        return indent + "- [ ] ";
    if (s.left(2) == "- " || s.left(2) == "* ")
        return indent + s.left(2);
    if (s.left(2) == "> ")
        return indent + "> ";
    int dot = s.find(". ");
    if (dot > 0 && dot < 4) {
        bool digits = true;
        int n = 0;
        for (int i = 0; i < dot; ++i) {
            if (s[i] < '0' || s[i] > '9')
                digits = false;
            else
                n = n * 10 + s[i].latin1() - '0';
        }
        if (digits)
            return indent + QString::number(n + 1) + ". ";
    }
    return "";
}

void MainWindow::touchEditor()
{
    editor->setFocus();
}

void MainWindow::scheduleAutosave()
{
    if (autosaveTimer && !currentFile.isEmpty())
        autosaveTimer->start(2500, true);
}

void MainWindow::autosaveTick()
{
    scheduleAutosave();
}

void MainWindow::applyFontSize()
{
    QFont f = editor->font();
    f.setPointSize(fontSize);
    editor->setFont(f);
    view->setFont(f);
}

void MainWindow::applyTheme()
{
    if (!darkTheme) {
        editor->unsetPalette();
        view->unsetPalette();
        browser->unsetPalette();
        return;
    }
    QPalette pal(QColor(238, 238, 238), QColor(32, 32, 32));
    editor->setPalette(pal);
    view->setPalette(pal);
    browser->setPalette(pal);
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
    int first, last;
    if (selectedLineRange(&first, &last) && first != last) {
        transformLineRange("task");
        return;
    }

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
    int first, last;
    if (selectedLineRange(&first, &last) && first != last) {
        transformLineRange("indent");
        return;
    }

    int row, col;
    editor->getCursorPosition(&row, &col);
    replaceCurrentLine("    " + editor->textLine(row));
}

void MainWindow::outdentLine()
{
    int first, last;
    if (selectedLineRange(&first, &last) && first != last) {
        transformLineRange("outdent");
        return;
    }

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

void MainWindow::insertTime()
{
    replaceSelectionOrInsert(QTime::currentTime().toString());
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

void MainWindow::copyText()
{
    if (!editor->hasSelection())
        return;
    QApplication::clipboard()->setText(editor->selectionText());
    statusBar()->message("Copied", 900);
    touchEditor();
}

void MainWindow::cutText()
{
    if (!editor->hasSelection())
        return;
    QApplication::clipboard()->setText(editor->selectionText());
    editor->deleteForward();
    touchEditor();
    scheduleAutosave();
}

void MainWindow::pasteText()
{
    QString text = QApplication::clipboard()->text();
    if (text.isEmpty())
        return;
    replaceSelectionOrInsert(text);
    touchEditor();
    scheduleAutosave();
}

void MainWindow::selectAllText()
{
    int lastLine = editor->numLines() - 1;
    if (lastLine < 0)
        return;
    int lastCol = 0;
    lastCol = editor->textLine(lastLine).length();
    editor->setSelection(0, 0, lastLine, lastCol);
    touchEditor();
}

void MainWindow::duplicateLine()
{
    int first, last;
    selectedLineRange(&first, &last);
    QStringList lines;
    for (int row = first; row <= last; ++row)
        lines.append(editor->textLine(row));
    editor->setCursorPosition(last, editor->textLine(last).length());
    editor->insert("\n" + lines.join("\n"));
    touchEditor();
    scheduleAutosave();
}

void MainWindow::smartNewLine()
{
    int row, col;
    editor->getCursorPosition(&row, &col);
    QString line = editor->textLine(row);
    QString cont = continuationForLine(line);
    if (!cont.isEmpty() && line.mid(col).stripWhiteSpace().isEmpty()) {
        QString before = line.left(col).stripWhiteSpace();
        QString prefix = cont.stripWhiteSpace();
        if (before == prefix || before == "- [x]" || before == "- [X]") {
            setCurrentLineText(row, "");
            editor->setCursorPosition(row, 0);
            scheduleAutosave();
            return;
        }
    }
    editor->insert("\n" + cont);
    scheduleAutosave();
}

void MainWindow::findText()
{
    bool ok = false;
    QString needle = TextPrompt::getText("Find", "Text", lastFind, &ok, this);
    if (!ok || needle.isEmpty())
        return;
    lastFind = needle;
    findNext();
}

void MainWindow::findNext()
{
    QString needle = lastFind;
    if (needle.isEmpty()) {
        findText();
        return;
    }
    int row, col;
    editor->getCursorPosition(&row, &col);
    for (int i = row; i < editor->numLines(); ++i) {
        QString line = editor->textLine(i);
        int from = (i == row) ? col + 1 : 0;
        int p = line.find(needle, from);
        if (p >= 0) {
            editor->setCursorPosition(i, p);
            editor->setSelection(i, p, i, p + needle.length());
            touchEditor();
            return;
        }
    }
    QMessageBox::information(this, "Find", "Not found.");
}

void MainWindow::replaceOne()
{
    bool ok = false;
    if (lastFind.isEmpty()) {
        lastFind = TextPrompt::getText("Replace one", "Find", "", &ok, this);
        if (!ok || lastFind.isEmpty())
            return;
    }
    QString repl = TextPrompt::getText("Replace one", "With", "", &ok, this);
    if (!ok)
        return;
    int row, col;
    editor->getCursorPosition(&row, &col);
    QString line = editor->textLine(row);
    int p = line.find(lastFind, col);
    if (p < 0) {
        findNext();
        editor->getCursorPosition(&row, &col);
        line = editor->textLine(row);
        p = line.find(lastFind, col);
    }
    if (p >= 0) {
        line.replace(p, lastFind.length(), repl);
        setCurrentLineText(row, line);
        editor->setCursorPosition(row, p + repl.length());
        scheduleAutosave();
    }
}

void MainWindow::replaceText()
{
    bool ok = false;
    QString needle = TextPrompt::getText("Replace", "Find", "", &ok, this);
    if (!ok || needle.isEmpty())
        return;
    lastFind = needle;
    QString repl = TextPrompt::getText("Replace", "With", "", &ok, this);
    if (!ok)
        return;
    QString text = editor->text();
    int p = 0;
    int count = 0;
    while ((p = text.find(needle, p)) >= 0) {
        text.replace(p, needle.length(), repl);
        p += repl.length();
        ++count;
    }
    editor->setText(text);
    scheduleAutosave();
    statusBar()->message(QString::number(count) + " replaced", 1500);
    touchEditor();
}

void MainWindow::moveLineUp()
{
    int first, last;
    selectedLineRange(&first, &last);
    if (first <= 0)
        return;
    QString prev = editor->textLine(first - 1);
    for (int row = first - 1; row < last; ++row)
        setCurrentLineText(row, editor->textLine(row + 1));
    setCurrentLineText(last, prev);
    editor->setCursorPosition(first - 1, 0);
    touchEditor();
    scheduleAutosave();
}

void MainWindow::moveLineDown()
{
    int first, last;
    selectedLineRange(&first, &last);
    if (last >= editor->numLines() - 1)
        return;
    QString next = editor->textLine(last + 1);
    for (int row = last + 1; row > first; --row)
        setCurrentLineText(row, editor->textLine(row - 1));
    setCurrentLineText(first, next);
    editor->setCursorPosition(first + 1, 0);
    touchEditor();
    scheduleAutosave();
}

bool MainWindow::isTodoFile() const
{
    return QFileInfo(currentFile).fileName() == "todo.txt";
}

void MainWindow::todoToggleDone()
{
    int row, col;
    editor->getCursorPosition(&row, &col);
    QString line = editor->textLine(row);
    if (isTodoFile())
        line = TodoTxt::toggleDone(line, QDate::currentDate().toString());
    else
        toggleTaskCurrent();
    if (isTodoFile())
        replaceCurrentLine(line);
    scheduleAutosave();
}

void MainWindow::setTodoPriority(const QString &priority)
{
    int first, last;
    selectedLineRange(&first, &last);
    for (int row = first; row <= last; ++row) {
        QString line = editor->textLine(row).stripWhiteSpace();
        if (line.length() >= 4 && line[0] == '(' && line[2] == ')' && line[1] >= 'A' && line[1] <= 'Z')
            line = line.mid(4);
        if (!priority.isEmpty())
            line = "(" + priority + ") " + line;
        setCurrentLineText(row, line);
    }
    editor->setCursorPosition(first, 0);
    touchEditor();
    scheduleAutosave();
}

void MainWindow::todoPriorityA()
{
    setTodoPriority("A");
}

void MainWindow::todoPriorityB()
{
    setTodoPriority("B");
}

void MainWindow::todoPriorityC()
{
    setTodoPriority("C");
}

void MainWindow::appendToCurrentLine(const QString &text)
{
    int row, col;
    editor->getCursorPosition(&row, &col);
    QString line = editor->textLine(row);
    if (!line.isEmpty() && line.right(1) != " ")
        line += " ";
    line += text;
    replaceCurrentLine(line);
}

QString MainWindow::stripTodoPriority(const QString &line) const
{
    QString s = line.stripWhiteSpace();
    if (s.length() >= 4 && s[0] == '(' && s[2] == ')' && s[1] >= 'A' && s[1] <= 'Z')
        return s.mid(4);
    return s;
}

int MainWindow::todoPriorityRank(const QString &line) const
{
    QString s = line.stripWhiteSpace();
    if (s.length() >= 3 && s[0] == '(' && s[2] == ')' && s[1] >= 'A' && s[1] <= 'Z')
        return s[1].latin1() - 'A';
    return 99;
}

void MainWindow::todoSortPriority()
{
    QStringList lines = QStringList::split('\n', editor->text(), true);
    for (uint i = 0; i < lines.count(); ++i) {
        for (uint j = i + 1; j < lines.count(); ++j) {
            if (todoPriorityRank(lines[j]) < todoPriorityRank(lines[i])) {
                QString tmp = lines[i];
                lines[i] = lines[j];
                lines[j] = tmp;
            }
        }
    }
    replaceAllText(lines.join("\n"), 0);
    scheduleAutosave();
}

QString MainWindow::withoutDoneLines(const QString &text) const
{
    QStringList lines = QStringList::split('\n', text, true);
    QStringList kept;
    for (uint i = 0; i < lines.count(); ++i) {
        QString s = lines[i].stripWhiteSpace();
        if (!(s.left(2) == "x " || s.find("[x]") >= 0 || s.find("[X]") >= 0))
            kept.append(lines[i]);
    }
    return kept.join("\n");
}

bool MainWindow::lineMatchesTodoFilter(const QString &line) const
{
    if (todoFilter.isEmpty())
        return true;
    return line.find(todoFilter) >= 0;
}

QString MainWindow::filteredPreviewText(const QString &text)
{
    previewLineMap.clear();
    QStringList lines = QStringList::split('\n', text, true);
    QStringList kept;
    for (uint i = 0; i < lines.count(); ++i) {
        QString s = lines[i].stripWhiteSpace();
        if (hideDone && (s.left(2) == "x " || s.find("[x]") >= 0 || s.find("[X]") >= 0))
            continue;
        if (!lineMatchesTodoFilter(lines[i]))
            continue;
        kept.append(lines[i]);
        previewLineMap.append((int)i);
    }
    return kept.join("\n");
}

void MainWindow::toggleHideDone()
{
    hideDone = !hideDone;
    refreshView();
    statusBar()->message(hideDone ? "Done hidden" : "Done shown", 1200);
}

void MainWindow::filterTodoProject()
{
    bool ok = false;
    QString value = TextPrompt::getText("Filter +", "Project", "", &ok, this);
    if (!ok)
        return;
    if (value.isEmpty())
        todoFilter = "";
    else
        todoFilter = "+" + value;
    refreshView();
    if (todoFilter.isEmpty())
        statusBar()->message("Filter cleared", 1200);
    else
        statusBar()->message(todoFilter, 1200);
}

void MainWindow::filterTodoContext()
{
    bool ok = false;
    QString value = TextPrompt::getText("Filter @", "Context", "", &ok, this);
    if (!ok)
        return;
    if (value.isEmpty())
        todoFilter = "";
    else
        todoFilter = "@" + value;
    refreshView();
    if (todoFilter.isEmpty())
        statusBar()->message("Filter cleared", 1200);
    else
        statusBar()->message(todoFilter, 1200);
}

void MainWindow::clearTodoFilter()
{
    todoFilter = "";
    hideDone = false;
    refreshView();
    statusBar()->message("Filter cleared", 1200);
}

void MainWindow::fontBigger()
{
    if (fontSize < 22)
        ++fontSize;
    applyFontSize();
}

void MainWindow::fontSmaller()
{
    if (fontSize > 8)
        --fontSize;
    applyFontSize();
}

void MainWindow::toggleTheme()
{
    darkTheme = !darkTheme;
    applyTheme();
}

void MainWindow::todoProject()
{
    bool ok = false;
    QString value = TextPrompt::getText("Project", "Name", "", &ok, this);
    if (ok && !value.isEmpty())
        appendToCurrentLine("+" + value);
}

void MainWindow::todoContext()
{
    bool ok = false;
    QString value = TextPrompt::getText("Context", "Name", "", &ok, this);
    if (ok && !value.isEmpty())
        appendToCurrentLine("@" + value);
}

void MainWindow::todoDue()
{
    bool ok = false;
    QString value = TextPrompt::getText("Due", "YYYY-MM-DD", QDate::currentDate().toString(), &ok, this);
    if (ok && !value.isEmpty())
        appendToCurrentLine("due:" + value);
}

void MainWindow::moveDoneTasksToEnd()
{
    QStringList lines = QStringList::split('\n', editor->text(), true);
    QStringList open;
    QStringList done;
    for (uint i = 0; i < lines.count(); ++i) {
        QString s = lines[i].stripWhiteSpace();
        if (s.left(2) == "x " || s.find("[x]") >= 0 || s.find("[X]") >= 0)
            done.append(lines[i]);
        else
            open.append(lines[i]);
    }
    replaceAllText(open.join("\n") + "\n" + done.join("\n"), 0);
    scheduleAutosave();
}

void MainWindow::clearDoneTasks()
{
    int answer = QMessageBox::warning(this, "Clear done", "Remove done tasks?", QMessageBox::Yes, QMessageBox::No);
    if (answer != QMessageBox::Yes)
        return;
    QStringList lines = QStringList::split('\n', editor->text(), true);
    QStringList kept;
    for (uint i = 0; i < lines.count(); ++i) {
        QString s = lines[i].stripWhiteSpace();
        if (!(s.left(2) == "x " || s.find("[x]") >= 0 || s.find("[X]") >= 0))
            kept.append(lines[i]);
    }
    replaceAllText(kept.join("\n"), 0);
    scheduleAutosave();
}
