#include "MainWindow.h"

#include "FileUtil.h"
#include "MdEdit.h"
#include "MarkdownActions.h"
#include "MdParser.h"
#include "MdView.h"
#include "TextPrompt.h"
#include "TodoMd.h"
#include "TodoTxt.h"

#include <qtopia/applnk.h>
#include <qtopia/fileselector.h>
#include <qtopia/resource.h>
#include <qtopia/storage.h>

#include <qdir.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qapplication.h>
#include <qclipboard.h>
#include <qevent.h>
#include <qlabel.h>
#include <qlistview.h>
#include <qlineedit.h>
#include <qmessagebox.h>
#include <qmultilineedit.h>
#include <qobjectlist.h>
#include <qpalette.h>
#include <qpushbutton.h>
#include <qstatusbar.h>
#include <qdatetime.h>
#include <qfont.h>
#include <qpixmap.h>
#include <qtimer.h>
#include <qtoolbutton.h>
#include <qvbox.h>
#include <qhbox.h>
#include <qwidgetstack.h>
#include <qlist.h>

class TodoListItem : public QCheckListItem {
public:
    TodoListItem(QListView *parent, const QString &text, int t, int s, bool step)
        : QCheckListItem(parent, text, CheckBox), taskIndex(t), stepIndex(s), isStep(step) { setText(2, step ? "step" : "task"); }
    TodoListItem(QListViewItem *parent, const QString &text, int t, int s, bool step)
        : QCheckListItem(parent, text, CheckBox), taskIndex(t), stepIndex(s), isStep(step) { setText(2, step ? "step" : "task"); }
    int taskIndex;
    int stepIndex;
    bool isStep;
};

static QString todayIsoDate()
{
    QDate d = QDate::currentDate();
    QString s;
    s.sprintf("%04d-%02d-%02d", d.year(), d.month(), d.day());
    return s;
}

MainWindow::MainWindow(QWidget *parent, const char *name, WFlags flags)
    : QMainWindow(parent, name, flags),
      notesDir("/home/zaurus/Documents/Notes"),
      currentDir(notesDir),
      dirHistoryIndex(-1),
      browser(0),
      documentSelector(0),
      stack(0),
      splitPane(0),
      todoPane(0),
      editor(0),
      view(0),
      splitView(0),
      todoList(0),
      todoAdd(0),
      modeButton(0),
      todoButton(0),
      splitButton(0),
      saveIndicator(0),
      fileBar(0),
      docBar(0),
      toolBar(0),
      autosaveTimer(0),
      todoFilter(""),
      darkTheme(false),
      hideDone(false),
      todoViewMode(3),
      toolPage(0),
      fontSize(12)
{
    for (int i = 0; i < 10; ++i)
        toolButtons[i] = 0;
    FileUtil::ensureDir(notesDir);
    buildUi();
    loadDirectory(notesDir);
}

void MainWindow::buildUi()
{
    QVBox *root = new QVBox(this);
    setCentralWidget(root);

    fileBar = new QWidget(root);
    fileBar->setFixedHeight(32);
    makeTopButton(fileBar, "Back", "back", SLOT(goBack()));
    makeTopButton(fileBar, "Fwd", "forward", SLOT(goForward()));
    makeTopButton(fileBar, "Up", "folderup", SLOT(goUp()));
    makeTopButton(fileBar, "New", "new", SLOT(newFile()));
    makeTopButton(fileBar, "Dir", "foldernew", SLOT(newFolder()));
    makeTopButton(fileBar, "Ren", "edit", SLOT(renameFile()));
    makeTopButton(fileBar, "Del", "editdelete", SLOT(deleteFile()));
    makeTopButton(fileBar, "Docs", "fileopen", SLOT(showDocumentLibrary()));
    makeTopButton(fileBar, "Root", "home", SLOT(showRoot()));

    docBar = new QWidget(root);
    docBar->setFixedHeight(32);
    makeTopButton(docBar, "Open", "fileopen", SLOT(showBrowser()));
    modeButton = makeTopButton(docBar, "View", "TextEditor", SLOT(showView()));
    splitButton = makeTopButton(docBar, "Split", "forward", SLOT(showSplit()));
    todoButton = makeTopButton(docBar, "Todo", "todo", SLOT(showTodo()));
    makeTopButton(docBar, "Save", "Save", SLOT(saveFile()));
    saveIndicator = new QLabel("", docBar);
    saveIndicator->setFont(QFont("song", 10));
    saveIndicator->setAlignment(AlignRight | AlignVCenter);

    stack = new QWidgetStack(root);
    browser = new QListView(stack);
    browser->addColumn("Name");
    browser->addColumn("Modified");
    browser->addColumn("Path");
    browser->setColumnWidth(2, 0);
    connect(browser, SIGNAL(doubleClicked(QListViewItem *)), this, SLOT(openSelected(QListViewItem *)));

    documentSelector = new FileSelector("text/*", stack, "documentSelector", FALSE, FALSE);
    connect(documentSelector, SIGNAL(fileSelected(const DocLnk &)), this, SLOT(openDocumentLibraryFile(const DocLnk &)));

    splitPane = new QHBox(stack);
    editor = new MdEdit(splitPane);
    splitView = new MdView(splitPane);
    splitView->hide();
    QToolButton *linesButton = makeTopButton(docBar, "Lines", "textedit", 0);
    linesButton->setToggleButton(true);
    connect(linesButton, SIGNAL(clicked()), editor, SLOT(toggleLineNumbers()));
    editor->setUndoEnabled(true);
    connect(editor, SIGNAL(textChanged()), this, SLOT(autosaveTick()));
    connect(editor, SIGNAL(requestIndent()), this, SLOT(indentLine()));
    connect(editor, SIGNAL(requestOutdent()), this, SLOT(outdentLine()));
    connect(editor, SIGNAL(requestNewLine()), this, SLOT(smartNewLine()));
    view = new MdView(stack);
    connect(view, SIGNAL(toggleTask(int)), this, SLOT(toggleTask(int)));
    connect(splitView, SIGNAL(toggleTask(int)), this, SLOT(toggleTask(int)));

    todoPane = new QVBox(stack);
    todoList = new QListView(todoPane);
    todoList->addColumn("Task");
    todoList->addColumn("Info");
    todoList->addColumn("Kind");
    todoList->setColumnWidth(2, 0);
    todoList->setRootIsDecorated(true);
    todoList->setAllColumnsShowFocus(true);
    connect(todoList, SIGNAL(clicked(QListViewItem *)), this, SLOT(todoItemClicked(QListViewItem *)));
    todoAdd = new QLineEdit(todoPane);
    todoAdd->setFont(QFont("song", 12));
    todoAdd->setText("+ Add task");
    connect(todoAdd, SIGNAL(returnPressed()), this, SLOT(addTodoFromInput()));

    stack->addWidget(browser, 0);
    stack->addWidget(splitPane, 1);
    stack->addWidget(view, 2);
    stack->addWidget(documentSelector, 3);
    stack->addWidget(todoPane, 4);

    toolBar = new QWidget(root);
    toolBar->setFixedHeight(30);
    for (int i = 0; i < 10; ++i) {
        toolButtons[i] = new QToolButton(toolBar);
        toolButtons[i]->setAutoRaise(true);
        toolButtons[i]->setFont(QFont("song", 10));
    }
    connect(toolButtons[0], SIGNAL(clicked()), this, SLOT(tool0()));
    connect(toolButtons[1], SIGNAL(clicked()), this, SLOT(tool1()));
    connect(toolButtons[2], SIGNAL(clicked()), this, SLOT(tool2()));
    connect(toolButtons[3], SIGNAL(clicked()), this, SLOT(tool3()));
    connect(toolButtons[4], SIGNAL(clicked()), this, SLOT(tool4()));
    connect(toolButtons[5], SIGNAL(clicked()), this, SLOT(tool5()));
    connect(toolButtons[6], SIGNAL(clicked()), this, SLOT(tool6()));
    connect(toolButtons[7], SIGNAL(clicked()), this, SLOT(tool7()));
    connect(toolButtons[8], SIGNAL(clicked()), this, SLOT(tool8()));
    connect(toolButtons[9], SIGNAL(clicked()), this, SLOT(tool9()));
    layoutTopBars();
    layoutToolButtons();
    rebuildToolBar();

    autosaveTimer = new QTimer(this);
    connect(autosaveTimer, SIGNAL(timeout()), this, SLOT(saveFile()));
    applyFontSize();
    applyTheme();

    stack->raiseWidget(browser);
    docBar->hide();
    fileBar->show();
    updateCaption();
    updateSaveIndicator();
}

QToolButton *MainWindow::makeButton(QWidget *parent, const char *text, const char *slot)
{
    QToolButton *button = new QToolButton(parent);
    button->setText(text);
    button->setTextLabel(text, true);
    button->setUsesTextLabel(true);
    button->setFont(QFont("song", 10));
    button->setFixedSize(38, 24);
    QObjectList *siblings = parent->queryList("QToolButton");
    int index = siblings ? siblings->count() - 1 : 0;
    delete siblings;
    button->move(index * 39, 3);
    connect(button, SIGNAL(clicked()), this, slot);
    return button;
}

void MainWindow::openInitialFile(const QString &path)
{
    if (path.isEmpty())
        return;
    QFileInfo info(path);
    if (info.isDir()) {
        QString abs = info.absFilePath();
        if (QFileInfo(abs).isReadable())
            loadDirectory(abs);
        return;
    }
    if (isEditableFileName(info.fileName()))
        openFile(info.absFilePath());
}

void MainWindow::setDocument(const QString &path)
{
    openInitialFile(path);
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    layoutTopBars();
    layoutToolButtons();
}

void MainWindow::layoutTopBars()
{
    if (saveIndicator) {
        int w = width();
        if (w < 240)
            w = 240;
        saveIndicator->setGeometry(w - 122, 1, 118, 30);
    }
}

void MainWindow::layoutToolButtons()
{
    int w = width();
    if (w < 240)
        w = 240;
    int buttonW = w / 10;
    if (buttonW < 32)
        buttonW = 32;
    for (int i = 0; i < 10; ++i) {
        if (!toolButtons[i])
            continue;
        toolButtons[i]->setFixedSize(buttonW - 1, 24);
        toolButtons[i]->move(i * buttonW, 3);
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (!confirmSaveIfNeeded()) {
        event->ignore();
        return;
    }
    event->accept();
}

bool MainWindow::confirmSaveIfNeeded()
{
    if (currentFile.isEmpty() || !editor->edited())
        return true;
    if (autosaveTimer)
        autosaveTimer->stop();
    int answer = QMessageBox::warning(this, "Save", "Save changes?", QMessageBox::Yes, QMessageBox::No, QMessageBox::Cancel);
    if (answer == QMessageBox::Cancel)
        return false;
    if (answer == QMessageBox::Yes)
        saveFile();
    return true;
}

void MainWindow::rebuildToolBar()
{
    if (todoPane && stack && stack->visibleWidget() == todoPane) {
        setToolButton(0, 0, "Day");
        setToolButton(1, 0, "!");
        setToolButton(2, 0, "Due");
        setToolButton(3, 0, "All");
        setToolButton(4, 0, "!*");
        setToolButton(5, 0, "+Sub");
        setToolButton(6, "editdelete", "Del");
        setToolButton(7, "TextEditor", "Edit");
        setToolButton(8, "Save", "Save");
        setToolButton(9, "forward", "More");
        return;
    }
    if (toolPage == 0) {
        setToolButton(0, 0, "**");
        setToolButton(1, 0, "//");
        setToolButton(2, 0, "`");
        setToolButton(3, 0, "#");
        setToolButton(4, 0, "-");
        setToolButton(5, 0, "1.");
        setToolButton(6, 0, "[ ]");
        setToolButton(7, 0, ">");
        setToolButton(8, 0, "---");
    } else if (toolPage == 1) {
        setToolButton(0, 0, "[]");
        setToolButton(1, 0, "!");
        setToolButton(2, 0, "| |");
        setToolButton(3, 0, "{ }");
        setToolButton(4, 0, "D");
        setToolButton(5, 0, "T");
        setToolButton(6, 0, "A+");
        setToolButton(7, 0, "A-");
        setToolButton(8, 0, "Sun");
    } else if (toolPage == 2) {
        setToolButton(0, "find", "?");
        setToolButton(1, "find_icon", "?>");
        setToolButton(2, 0, "R");
        setToolButton(3, 0, "R*");
        setToolButton(4, "back", "<-");
        setToolButton(5, "forward", "->");
        setToolButton(6, "copy", "++");
        setToolButton(7, 0, "All");
        setToolButton(8, "paste", "Paste");
    } else if (toolPage == 3) {
        setToolButton(0, "forward", ">>");
        setToolButton(1, "back", "<<");
        setToolButton(2, "up", "^");
        setToolButton(3, 0, "v");
        setToolButton(4, "copy", "Copy");
        setToolButton(5, "cut", "Cut");
        setToolButton(6, "paste", "Paste");
        setToolButton(7, 0, "D");
        setToolButton(8, 0, "T");
    } else {
        if (toolPage == 4) {
            setToolButton(0, 0, "[x]");
            setToolButton(1, 0, "x+");
            setToolButton(2, 0, "x-");
            setToolButton(3, 0, "A");
            setToolButton(4, 0, "B");
            setToolButton(5, 0, "C");
            setToolButton(6, 0, "+");
            setToolButton(7, 0, "@");
            setToolButton(8, 0, "!");
        } else {
            setToolButton(0, 0, "A/Z");
            setToolButton(1, 0, "Hide");
            setToolButton(2, "find", "F+");
            setToolButton(3, "find", "F@");
            setToolButton(4, "close", "F-");
            setToolButton(5, 0, "End");
            setToolButton(6, "editdelete", "Del");
            setToolButton(7, 0, "A+");
            setToolButton(8, 0, "A-");
        }
    }
    setToolButton(9, "forward", "...");
    toolBar->update();
}

void MainWindow::setToolLabel(int index, const char *text)
{
    if (index < 0 || index >= 10 || !toolButtons[index])
        return;
    toolButtons[index]->setText(text);
    toolButtons[index]->setTextLabel(text, true);
    toolButtons[index]->setUsesTextLabel(true);
    toolButtons[index]->show();
}

void MainWindow::setToolButton(int index, const char *icon, const char *text)
{
    if (index < 0 || index >= 10 || !toolButtons[index])
        return;
    toolButtons[index]->setText(text);
    toolButtons[index]->setTextLabel(text, true);
    if (icon && Resource::findPixmap(icon) != QString::null) {
        toolButtons[index]->setPixmap(Resource::loadPixmap(icon));
        toolButtons[index]->setUsesTextLabel(false);
    } else {
        toolButtons[index]->setPixmap(QPixmap());
        toolButtons[index]->setUsesTextLabel(true);
    }
    toolButtons[index]->show();
}

void MainWindow::updateSaveIndicator()
{
    if (!saveIndicator)
        return;
    if (currentFile.isEmpty()) {
        saveIndicator->setText("");
        return;
    }
    saveIndicator->setText(editor->edited() ? "Modified" : "Saved");
}

void MainWindow::nextToolPage()
{
    ++toolPage;
    if (toolPage > 5)
        toolPage = 0;
    rebuildToolBar();
    touchEditor();
}

void MainWindow::runTool(int index)
{
    if (index == 9) {
        nextToolPage();
        return;
    }
    if (todoPane && stack && stack->visibleWidget() == todoPane) {
        if (index == 0) todoViewMode = 0;
        else if (index == 1) todoViewMode = 1;
        else if (index == 2) todoViewMode = 2;
        else if (index == 3) todoViewMode = 3;
        else if (index == 4) toggleTodoImportant();
        else if (index == 5) addTodoStep();
        else if (index == 6) deleteTodoCurrent();
        else if (index == 7) showEditor();
        else if (index == 8) saveFile();
        refreshTodo();
        return;
    }

    if (toolPage == 0) {
        if (index == 0) wrapBold();
        else if (index == 1) wrapItalic();
        else if (index == 2) wrapCode();
        else if (index == 3) cycleHeading();
        else if (index == 4) toggleBullet();
        else if (index == 5) toggleNumber();
        else if (index == 6) toggleTaskCurrent();
        else if (index == 7) quoteLine();
        else if (index == 8) insertRule();
    } else if (toolPage == 1) {
        if (index == 0) insertLink();
        else if (index == 1) insertImage();
        else if (index == 2) insertTable();
        else if (index == 3) insertCodeBlock();
        else if (index == 4) insertDate();
        else if (index == 5) insertTime();
        else if (index == 6) fontBigger();
        else if (index == 7) fontSmaller();
        else if (index == 8) toggleTheme();
    } else if (toolPage == 2) {
        if (index == 0) findText();
        else if (index == 1) findNext();
        else if (index == 2) replaceOne();
        else if (index == 3) replaceText();
        else if (index == 4) undoEdit();
        else if (index == 5) redoEdit();
        else if (index == 6) duplicateLine();
        else if (index == 7) selectAllText();
        else if (index == 8) pasteText();
    } else if (toolPage == 3) {
        if (index == 0) indentLine();
        else if (index == 1) outdentLine();
        else if (index == 2) moveLineUp();
        else if (index == 3) moveLineDown();
        else if (index == 4) copyText();
        else if (index == 5) cutText();
        else if (index == 6) pasteText();
        else if (index == 7) insertDate();
        else if (index == 8) insertTime();
    } else if (toolPage == 4) {
        if (index == 0) todoToggleDone();
        else if (index == 1) markSelectedTasksDone();
        else if (index == 2) markSelectedTasksOpen();
        else if (index == 3) todoPriorityA();
        else if (index == 4) todoPriorityB();
        else if (index == 5) todoPriorityC();
        else if (index == 6) todoProject();
        else if (index == 7) todoContext();
        else if (index == 8) todoDue();
    } else {
        if (index == 0) todoSortPriority();
        else if (index == 1) toggleHideDone();
        else if (index == 2) filterTodoProject();
        else if (index == 3) filterTodoContext();
        else if (index == 4) clearTodoFilter();
        else if (index == 5) moveDoneTasksToEnd();
        else if (index == 6) clearDoneTasks();
        else if (index == 7) fontBigger();
        else if (index == 8) fontSmaller();
    }
}

void MainWindow::tool0() { runTool(0); }
void MainWindow::tool1() { runTool(1); }
void MainWindow::tool2() { runTool(2); }
void MainWindow::tool3() { runTool(3); }
void MainWindow::tool4() { runTool(4); }
void MainWindow::tool5() { runTool(5); }
void MainWindow::tool6() { runTool(6); }
void MainWindow::tool7() { runTool(7); }
void MainWindow::tool8() { runTool(8); }
void MainWindow::tool9() { runTool(9); }

void MainWindow::updateCaption()
{
    if (currentFile.isEmpty())
        setCaption("Zaurus MDEditor");
    else
        setCaption("Zaurus MDEditor - " + QFileInfo(currentFile).fileName());
}

QToolButton *MainWindow::makeTopButton(QWidget *parent, const char *text, const char *icon, const char *slot)
{
    QToolButton *button = new QToolButton(parent);
    button->setFont(QFont("song", 10));
    button->setFixedSize(70, 30);
    button->setAutoRaise(true);
    button->setText(text);
    button->setTextLabel(text, true);
    if (icon && Resource::findPixmap(icon) != QString::null) {
        button->setPixmap(Resource::loadPixmap(icon));
        button->setUsesTextLabel(false);
    } else {
        button->setPixmap(QPixmap());
        button->setUsesTextLabel(true);
    }
    QObjectList *siblings = parent->queryList("QToolButton");
    int index = siblings ? siblings->count() - 1 : 0;
    delete siblings;
    button->move(index * 71, 1);
    if (slot)
        connect(button, SIGNAL(clicked()), this, slot);
    return button;
}

void MainWindow::loadDirectory(const QString &path, bool remember)
{
    currentDir = QDir(path).absPath();
    if (remember) {
        if ((int)dirHistory.count() > dirHistoryIndex + 1) {
            QStringList kept;
            for (int i = 0; i <= dirHistoryIndex; ++i)
                kept.append(dirHistory[i]);
            dirHistory = kept;
        }
        if (dirHistoryIndex < 0 || dirHistory[dirHistoryIndex] != currentDir) {
            dirHistory.append(currentDir);
            dirHistoryIndex = dirHistory.count() - 1;
        }
    }
    if (stack && stack->visibleWidget() == browser)
        currentFile = QString::null;
    updateCaption();
    browser->clear();
    if (currentDir == "/") {
        addLocationItem("Documents", "/home/zaurus/Documents");
        addLocationItem("Notes", notesDir);
        addLocationItem("Todo", notesDir + "/Todo");
        addStorageLocations();
        addLocationItem("CF Card", "/mnt/cf");
        addLocationItem("SD Card", "/mnt/card");
        addLocationItem("SD Card", "/mnt/sd");
        addLocationItem("Home", "/home/zaurus");
    }
    if (currentDir != "/")
        new QListViewItem(browser, "..", "");
    if (currentDir == notesDir) {
        QString quick = currentDir + "/QuickNote.md";
        QString todo = currentDir + "/todo.txt";
        QString todoDir = currentDir + "/Todo";
        if (!QFileInfo(quick).exists())
            FileUtil::writeUtf8Atomic(quick, "# QuickNote\n\n");
        if (!QFileInfo(todo).exists())
            FileUtil::writeUtf8Atomic(todo, "");
        FileUtil::ensureDir(todoDir);
        if (!QFileInfo(todoDir + "/TODO.md").exists())
            FileUtil::writeUtf8Atomic(todoDir + "/TODO.md", "");
    }

    QDir dir(currentDir);
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
        if (fi->isFile() && !isEditableFileName(name))
            continue;
        QListViewItem *entry = new QListViewItem(browser, name, fi->lastModified().toString());
        entry->setText(2, fi->absFilePath());
    }
}

void MainWindow::addLocationItem(const QString &name, const QString &path)
{
    QFileInfo info(path);
    if (!info.exists() || !info.isDir() || !info.isReadable())
        return;
    QListViewItem *scan = browser->firstChild();
    while (scan) {
        if (scan->text(2) == info.absFilePath())
            return;
        scan = scan->nextSibling();
    }
    QListViewItem *entry = new QListViewItem(browser, name, path);
    entry->setText(2, info.absFilePath());
}

void MainWindow::addStorageLocations()
{
    StorageInfo storage;
    const QList<FileSystem> &fileSystems = storage.fileSystems();
    QListIterator<FileSystem> it(fileSystems);
    for (; it.current(); ++it) {
        const FileSystem *fs = *it;
        if (!fs)
            continue;
        QString path = fs->path();
        if (path.isEmpty() || path == "/")
            continue;
        QString name = fs->name();
        if (name.isEmpty())
            name = path;
        addLocationItem(name, path);
        QString documents = path + "/Documents";
        if (QFileInfo(documents).isDir())
            addLocationItem(name + " Documents", documents);
    }
}

void MainWindow::openSelected(QListViewItem *item)
{
    if (!item)
        return;
    if (item->text(0) == "..") {
        goUp();
        return;
    }
    QString path = item->text(2);
    if (path.isEmpty())
        path = currentDir + "/" + item->text(0);
    QFileInfo fi(path);
    if (fi.isDir()) {
        loadDirectory(path);
        return;
    }
    openFileInNewWindow(path);
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
    updateCaption();
    if (isMarkdownTodoFile())
        showTodo();
    else
        showEditor();
}

void MainWindow::openFileInNewWindow(const QString &path)
{
    MainWindow *window = new MainWindow(0, 0, WDestructiveClose);
    window->openInitialFile(path);
    window->showMaximized();
}

bool MainWindow::isEditableFileName(const QString &name) const
{
    QString lower = name.lower();
    return lower.right(3) == ".md"
        || lower.right(4) == ".txt"
        || lower.right(5) == ".mkd"
        || lower.right(5) == ".text"
        || lower.right(9) == ".markdown";
}

bool MainWindow::isMarkdownTodoFile() const
{
    QFileInfo info(currentFile);
    QString dir = info.dirPath(true);
    return info.fileName().lower() == "todo.md" || dir.right(5) == "/Todo";
}

void MainWindow::showBrowser()
{
    if (!confirmSaveIfNeeded())
        return;
    currentFile = QString::null;
    updateCaption();
    updateSaveIndicator();
    docBar->hide();
    fileBar->show();
    stack->raiseWidget(browser);
    browser->setFocus();
}

void MainWindow::showRoot()
{
    if (!confirmSaveIfNeeded())
        return;
    currentFile = QString::null;
    docBar->hide();
    fileBar->show();
    stack->raiseWidget(browser);
    loadDirectory("/");
    browser->setFocus();
}

void MainWindow::showDocumentLibrary()
{
    if (!confirmSaveIfNeeded())
        return;
    currentFile = QString::null;
    updateCaption();
    updateSaveIndicator();
    docBar->hide();
    fileBar->show();
    if (documentSelector)
        documentSelector->reread();
    stack->raiseWidget(documentSelector);
    documentSelector->setFocus();
}

void MainWindow::openDocumentLibraryFile(const DocLnk &doc)
{
    QString path = doc.file();
    if (!path.isEmpty())
        openFileInNewWindow(path);
}

void MainWindow::showEditor()
{
    fileBar->hide();
    docBar->show();
    if (splitView)
        splitView->hide();
    stack->raiseWidget(splitPane);
    modeButton->setText("View");
    modeButton->setTextLabel("View", true);
    disconnect(modeButton, SIGNAL(clicked()), this, SLOT(showEditor()));
    connect(modeButton, SIGNAL(clicked()), this, SLOT(showView()));
    rebuildToolBar();
    updateSaveIndicator();
    touchEditor();
}

void MainWindow::showSplit()
{
    refreshSplit();
    fileBar->hide();
    docBar->show();
    if (splitView)
        splitView->show();
    stack->raiseWidget(splitPane);
    modeButton->setText("View");
    modeButton->setTextLabel("View", true);
    disconnect(modeButton, SIGNAL(clicked()), this, SLOT(showEditor()));
    connect(modeButton, SIGNAL(clicked()), this, SLOT(showView()));
    rebuildToolBar();
    updateSaveIndicator();
    touchEditor();
}

void MainWindow::showView()
{
    saveFile();
    refreshView();
    fileBar->hide();
    docBar->show();
    stack->raiseWidget(view);
    modeButton->setText("Edit");
    modeButton->setTextLabel("Edit", true);
    disconnect(modeButton, SIGNAL(clicked()), this, SLOT(showView()));
    connect(modeButton, SIGNAL(clicked()), this, SLOT(showEditor()));
    rebuildToolBar();
}

void MainWindow::refreshView()
{
    QString text = filteredPreviewText(editor->text());
    view->setText(MdParser::toRichText(text, 0), currentFile);
}

void MainWindow::showTodo()
{
    if (!isMarkdownTodoFile()) {
        statusBar()->message("Todo view is for Todo/*.md", 1200);
        return;
    }
    saveFile();
    refreshTodo();
    fileBar->hide();
    docBar->show();
    if (splitView)
        splitView->hide();
    stack->raiseWidget(todoPane);
    modeButton->setText("Edit");
    modeButton->setTextLabel("Edit", true);
    disconnect(modeButton, SIGNAL(clicked()), this, SLOT(showView()));
    disconnect(modeButton, SIGNAL(clicked()), this, SLOT(showEditor()));
    connect(modeButton, SIGNAL(clicked()), this, SLOT(showEditor()));
    rebuildToolBar();
    updateSaveIndicator();
    todoList->setFocus();
}

void MainWindow::refreshTodo()
{
    if (!todoList)
        return;
    QString today = todayIsoDate();
    TodoMdDoc doc = TodoMd::parseTodo(editor->text(), today);
    todoList->clear();
    QListViewItem *doneGroup = 0;
    int doneCount = 0;
    TodoListItem *currentTaskItem = 0;
    int currentTask = -1;
    int stepDone = 0;
    int stepTotal = 0;
    for (int i = 0; i < (int)doc.entries.count(); ++i) {
        TodoMdEntry e = doc.entries[i];
        if (e.kind == TodoMdEntry::Task) {
            if (currentTaskItem && stepTotal > 0)
                currentTaskItem->setText(1, currentTaskItem->text(1) + " " + QString::number(stepDone) + "/" + QString::number(stepTotal));
            currentTask = e.task;
            stepDone = 0;
            stepTotal = 0;
            bool visible = true;
            if (todoViewMode == 0)
                visible = e.myday == today || e.due == today;
            else if (todoViewMode == 1)
                visible = e.important;
            else if (todoViewMode == 2)
                visible = !e.due.isEmpty();
            if (!visible) {
                currentTaskItem = 0;
                continue;
            }
            QString info;
            if (e.important)
                info += "!";
            if (!e.due.isEmpty()) {
                if (!info.isEmpty())
                    info += " ";
                info += "due:" + e.due;
            }
            if (e.done) {
                if (!doneGroup)
                    doneGroup = new QListViewItem(todoList, "Done", "");
                currentTaskItem = new TodoListItem(doneGroup, e.title, e.task, -1, false);
                ++doneCount;
            } else {
                currentTaskItem = new TodoListItem(todoList, e.title, e.task, -1, false);
            }
            currentTaskItem->setOn(e.done);
            currentTaskItem->setText(1, info);
            currentTaskItem->setOpen(true);
        } else if (e.kind == TodoMdEntry::Step && currentTaskItem && e.task == currentTask) {
            TodoListItem *step = new TodoListItem(currentTaskItem, e.title, e.task, stepTotal, true);
            step->setOn(e.done);
            ++stepTotal;
            if (e.done)
                ++stepDone;
        }
    }
    if (currentTaskItem && stepTotal > 0)
        currentTaskItem->setText(1, currentTaskItem->text(1) + " " + QString::number(stepDone) + "/" + QString::number(stepTotal));
    if (doneGroup) {
        doneGroup->setText(0, "Done (" + QString::number(doneCount) + ")");
        doneGroup->setOpen(false);
    }
}

void MainWindow::saveTodoDoc()
{
    editor->setEdited(true);
    saveFile();
    refreshTodo();
}

void MainWindow::todoItemClicked(QListViewItem *item)
{
    if (!item || (item->text(2) != "task" && item->text(2) != "step"))
        return;
    TodoListItem *todo = (TodoListItem *)item;
    QString today = todayIsoDate();
    TodoMdDoc doc = TodoMd::parseTodo(editor->text(), today);
    if (todo->isStep)
        TodoMd::setStepDone(&doc, todo->taskIndex, todo->stepIndex, todo->isOn());
    else
        TodoMd::setTaskDone(&doc, todo->taskIndex, todo->isOn(), today);
    editor->setText(TodoMd::serializeTodo(doc));
    saveTodoDoc();
}

void MainWindow::addTodoFromInput()
{
    QString title = todoAdd->text().stripWhiteSpace();
    if (title.left(1) == "+")
        title = title.mid(1).stripWhiteSpace();
    if (title.isEmpty())
        return;
    QString today = todayIsoDate();
    TodoMdDoc doc = TodoMd::parseTodo(editor->text(), today);
    TodoMd::addTask(&doc, title, todoViewMode == 0, today);
    editor->setText(TodoMd::serializeTodo(doc));
    todoAdd->setText("+ Add task");
    saveTodoDoc();
}

void MainWindow::toggleTodoImportant()
{
    QListViewItem *raw = todoList->currentItem();
    if (!raw || raw->text(2) != "task")
        return;
    TodoListItem *item = (TodoListItem *)raw;
    QString today = todayIsoDate();
    TodoMdDoc doc = TodoMd::parseTodo(editor->text(), today);
    TodoMd::toggleImportant(&doc, item->taskIndex);
    editor->setText(TodoMd::serializeTodo(doc));
    saveTodoDoc();
}

void MainWindow::addTodoStep()
{
    QListViewItem *raw = todoList->currentItem();
    if (!raw || (raw->text(2) != "task" && raw->text(2) != "step"))
        return;
    TodoListItem *item = (TodoListItem *)raw;
    bool ok = false;
    QString title = TextPrompt::getText("Step", "Title", "", &ok, this);
    if (!ok || title.stripWhiteSpace().isEmpty())
        return;
    QString today = todayIsoDate();
    TodoMdDoc doc = TodoMd::parseTodo(editor->text(), today);
    TodoMd::addStep(&doc, item->taskIndex, title.stripWhiteSpace());
    editor->setText(TodoMd::serializeTodo(doc));
    saveTodoDoc();
}

void MainWindow::deleteTodoCurrent()
{
    QListViewItem *raw = todoList->currentItem();
    if (!raw || raw->text(2) != "task")
        return;
    TodoListItem *item = (TodoListItem *)raw;
    if (QMessageBox::warning(this, "Delete", "Delete selected task?", QMessageBox::Yes, QMessageBox::No) != QMessageBox::Yes)
        return;
    QString today = todayIsoDate();
    TodoMdDoc doc = TodoMd::parseTodo(editor->text(), today);
    TodoMd::deleteTask(&doc, item->taskIndex);
    editor->setText(TodoMd::serializeTodo(doc));
    saveTodoDoc();
}

void MainWindow::refreshSplit()
{
    QString text = filteredPreviewText(editor->text());
    splitView->setText(MdParser::toRichText(text, 0), currentFile);
}

int MainWindow::editorByteOffset(int line, int col) const
{
    int offset = 0;
    if (line < 0)
        line = 0;
    for (int row = 0; row < line && row < editor->logicalLineCount(); ++row)
        offset += editor->logicalLine(row).utf8().length() + 1;
    if (line < editor->logicalLineCount()) {
        QString prefix = editor->logicalLine(line).left(col);
        offset += prefix.utf8().length();
    }
    return offset;
}

void MainWindow::byteOffsetToCursor(const QString &text, int bytes, int *line, int *col) const
{
    if (bytes < 0)
        bytes = 0;
    QStringList lines = QStringList::split('\n', text, true);
    int consumed = 0;
    for (int row = 0; row < (int)lines.count(); ++row) {
        QString one = lines[row];
        int lineBytes = one.utf8().length();
        if (bytes <= consumed + lineBytes) {
            int best = 0;
            for (int c = 0; c <= (int)one.length(); ++c) {
                int here = one.left(c).utf8().length();
                if (consumed + here <= bytes)
                    best = c;
                else
                    break;
            }
            *line = row;
            *col = best;
            return;
        }
        consumed += lineBytes + 1;
    }
    *line = lines.count() ? lines.count() - 1 : 0;
    *col = lines.count() ? lines[*line].length() : 0;
}

void MainWindow::applyMarkdownAction(int actionId)
{
    int l1, c1, l2, c2;
    bool hasSel = editor->selectionRegion(&l1, &c1, &l2, &c2);
    if (!hasSel) {
        editor->logicalCursor(&l1, &c1);
        l2 = l1;
        c2 = c1;
    }
    int start = editorByteOffset(l1, c1);
    int end = editorByteOffset(l2, c2);
    if (start > end) {
        int t = start;
        start = end;
        end = t;
    }

    MdActionCtx ctx;
    ctx.indentWidth = 2;
    ctx.dateText = QDate::currentDate().toString().latin1();
    ctx.timeText = QTime::currentTime().toString().latin1();
    QCString utf = editor->text().utf8();
    MdActionResult result = mdApplyAction((MdActionId)actionId, std::string(utf.data(), utf.length()), start, end, ctx);
    QString next = QString::fromUtf8(result.text.c_str(), result.text.size());
    editor->setText(next);
    int sl, sc, el, ec;
    byteOffsetToCursor(next, result.selStart, &sl, &sc);
    byteOffsetToCursor(next, result.selEnd, &el, &ec);
    if (result.selStart != result.selEnd)
        editor->selectLogical(sl, sc, el, ec);
    else
        editor->setLogicalCursor(sl, sc);
    editor->setEdited(true);
    touchEditor();
    scheduleAutosave();
    if (splitView && splitView->isVisible())
        refreshSplit();
}

void MainWindow::saveFile()
{
    if (currentFile.isEmpty())
        return;
    if (autosaveTimer)
        autosaveTimer->stop();
    FileUtil::writeUtf8Atomic(currentFile, editor->text());
    editor->setEdited(false);
    updateSaveIndicator();
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
    if (!isEditableFileName(name))
        name += ".md";
    QString path = currentDir + "/" + name;
    if (QFileInfo(path).exists()) {
        QMessageBox::warning(this, "New file", "File exists.");
        return;
    }
    FileUtil::writeUtf8Atomic(path, "# " + name + "\n\n");
    loadDirectory(currentDir);
    openFileInNewWindow(path);
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

void MainWindow::goBack()
{
    if ((int)dirHistory.count() == 0 || dirHistoryIndex <= 0)
        return;
    --dirHistoryIndex;
    loadDirectory(dirHistory[dirHistoryIndex], false);
}

void MainWindow::goForward()
{
    if (dirHistoryIndex < 0 || dirHistoryIndex + 1 >= (int)dirHistory.count())
        return;
    ++dirHistoryIndex;
    loadDirectory(dirHistory[dirHistoryIndex], false);
}

void MainWindow::goUp()
{
    if (stack && stack->visibleWidget() != browser) {
        showBrowser();
        return;
    }
    if (currentDir == "/")
        return;
    QDir d(currentDir);
    d.cdUp();
    QString next = d.absPath();
    loadDirectory(next);
}

QString MainWindow::currentSelectedPath() const
{
    QListViewItem *item = browser->currentItem();
    if (!item)
        return QString::null;
    if (item->text(0) == "..")
        return QString::null;
    if (!item->text(2).isEmpty())
        return item->text(2);
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
    if (!isEditableFileName(name))
        name += ".md";
    QString path = currentDir + "/" + name;
    if (QFileInfo(path).exists()) {
        QMessageBox::warning(this, "Save as", "File exists.");
        return;
    }
    currentFile = path;
    saveFile();
    loadDirectory(currentDir);
    updateCaption();
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
            updateCaption();
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
        editor->insert(text);
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
        editor->logicalCursor(&line, &col);
        editor->insert(before + after);
        editor->setLogicalCursor(line, col + before.length());
    } else {
        replaceSelectionOrInsert(before + sel + after);
    }
    touchEditor();
    scheduleAutosave();
}

void MainWindow::toggleInlineMarkup(const QString &before, const QString &after)
{
    int line1, col1, line2, col2;
    if (editor->selectionRegion(&line1, &col1, &line2, &col2)) {
        QString sel = editor->selectionText();
        if (sel.left(before.length()) == before && sel.right(after.length()) == after
                && sel.length() >= before.length() + after.length()) {
            QString inner = sel.mid(before.length(), sel.length() - before.length() - after.length());
            editor->insert(inner);
            editor->selectLogical(line1, col1, line1, col1 + inner.length());
        } else {
            editor->insert(before + sel + after);
            editor->selectLogical(line1, col1 + before.length(), line1, col1 + before.length() + sel.length());
        }
        touchEditor();
        scheduleAutosave();
        return;
    }

    int line, col;
    editor->logicalCursor(&line, &col);
    QString text = editor->logicalLine(line);
    int beforeLen = before.length();
    int afterLen = after.length();

    if (col >= beforeLen && col + afterLen <= (int)text.length()
            && text.mid(col - beforeLen, beforeLen) == before
            && text.mid(col, afterLen) == after) {
        QString next = text.left(col - beforeLen) + text.mid(col + afterLen);
        replaceCurrentLine(next);
        editor->setLogicalCursor(line, col - beforeLen);
        return;
    }

    wrapSelection(before, after);
}

void MainWindow::setCurrentLineText(int lineNo, const QString &line)
{
    editor->selectLogical(lineNo, 0, lineNo, editor->logicalLine(lineNo).length());
    editor->insert(line);
}

void MainWindow::replaceAllText(const QString &text, int cursorLine)
{
    editor->setText(text);
    if (cursorLine < 0)
        cursorLine = 0;
    if (cursorLine >= editor->logicalLineCount())
        cursorLine = editor->logicalLineCount() - 1;
    editor->setLogicalCursor(cursorLine, 0);
    touchEditor();
    scheduleAutosave();
}

void MainWindow::replaceCurrentLine(const QString &line)
{
    int row, col;
    editor->logicalCursor(&row, &col);
    setCurrentLineText(row, line);
    if (col > (int)line.length())
        col = line.length();
    editor->setLogicalCursor(row, col);
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
    editor->logicalCursor(&row, &col);
    QString line = editor->logicalLine(row);
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
        editor->logicalCursor(&l1, &c1);
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
    if (l2 >= editor->logicalLineCount())
        l2 = editor->logicalLineCount() - 1;
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
        QString line = editor->logicalLine(row);
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
    editor->setLogicalCursor(first, 0);
    touchEditor();
    scheduleAutosave();
}

void MainWindow::transformLineRange(const QString &mode)
{
    int first, last;
    selectedLineRange(&first, &last);
    for (int row = first; row <= last; ++row) {
        QString line = editor->logicalLine(row);
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
    editor->setLogicalCursor(first, 0);
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
    if (autosaveTimer)
        autosaveTimer->stop();
}

void MainWindow::autosaveTick()
{
    scheduleAutosave();
    updateSaveIndicator();
}

void MainWindow::applyFontSize()
{
    QFont f("song", fontSize);
    editor->setFont(f);
    view->setFont(f);
    splitView->setFont(f);
    browser->setFont(QFont("song", 11));
}

void MainWindow::applyTheme()
{
    if (!darkTheme) {
        editor->unsetPalette();
        view->unsetPalette();
        splitView->unsetPalette();
        browser->unsetPalette();
        return;
    }
    QPalette pal(QColor(238, 238, 238), QColor(32, 32, 32));
    editor->setPalette(pal);
    view->setPalette(pal);
    splitView->setPalette(pal);
    browser->setPalette(pal);
}

void MainWindow::wrapBold()
{
    applyMarkdownAction(MdActBold);
}

void MainWindow::wrapItalic()
{
    applyMarkdownAction(MdActItalic);
}

void MainWindow::wrapCode()
{
    applyMarkdownAction(MdActCode);
}

void MainWindow::cycleHeading()
{
    applyMarkdownAction(MdActHeading);
}

void MainWindow::toggleBullet()
{
    applyMarkdownAction(MdActBullet);
}

void MainWindow::toggleNumber()
{
    applyMarkdownAction(MdActNumber);
}

void MainWindow::toggleTaskCurrent()
{
    applyMarkdownAction(MdActTask);
}

void MainWindow::quoteLine()
{
    applyMarkdownAction(MdActQuote);
}

void MainWindow::insertLink()
{
    applyMarkdownAction(MdActLink);
}

void MainWindow::insertImage()
{
    bool ok = false;
    QString path = TextPrompt::getText("Image", "Path", "image.jpg", &ok, this);
    if (!ok || path.isEmpty())
        return;
    QString alt = selectedText();
    if (alt.isNull())
        alt = "image";
    replaceSelectionOrInsert("![" + alt + "](" + path + ")");
    touchEditor();
}

void MainWindow::insertTable()
{
    QString table = "\n| Name | Value |\n| --- | --- |\n|  |  |\n";
    replaceSelectionOrInsert(table);
    touchEditor();
}

void MainWindow::insertCodeBlock()
{
    QString sel = selectedText();
    if (sel.isNull())
        sel = "code";
    replaceSelectionOrInsert("\n```\n" + sel + "\n```\n");
    touchEditor();
}

void MainWindow::insertRule()
{
    applyMarkdownAction(MdActRule);
}

void MainWindow::indentLine()
{
    applyMarkdownAction(MdActIndent);
}

void MainWindow::outdentLine()
{
    applyMarkdownAction(MdActOutdent);
}

void MainWindow::insertDate()
{
    applyMarkdownAction(MdActDate);
}

void MainWindow::insertTime()
{
    applyMarkdownAction(MdActTime);
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
    int lastLine = editor->logicalLineCount() - 1;
    if (lastLine < 0)
        return;
    int lastCol = 0;
    lastCol = editor->logicalLine(lastLine).length();
    editor->selectLogical(0, 0, lastLine, lastCol);
    touchEditor();
}

void MainWindow::duplicateLine()
{
    int first, last;
    selectedLineRange(&first, &last);
    QStringList lines;
    for (int row = first; row <= last; ++row)
        lines.append(editor->logicalLine(row));
    editor->setLogicalCursor(last, editor->logicalLine(last).length());
    editor->insert("\n" + lines.join("\n"));
    touchEditor();
    scheduleAutosave();
}

void MainWindow::smartNewLine()
{
    applyMarkdownAction(MdActEnter);
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
    editor->logicalCursor(&row, &col);
    for (int i = row; i < editor->logicalLineCount(); ++i) {
        QString line = editor->logicalLine(i);
        int from = (i == row) ? col + 1 : 0;
        int p = line.find(needle, from);
        if (p >= 0) {
            editor->setLogicalCursor(i, p);
            editor->selectLogical(i, p, i, p + needle.length());
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
    editor->logicalCursor(&row, &col);
    QString line = editor->logicalLine(row);
    int p = line.find(lastFind, col);
    if (p < 0) {
        findNext();
        editor->logicalCursor(&row, &col);
        line = editor->logicalLine(row);
        p = line.find(lastFind, col);
    }
    if (p >= 0) {
        line.replace(p, lastFind.length(), repl);
        setCurrentLineText(row, line);
        editor->setLogicalCursor(row, p + repl.length());
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
    QString prev = editor->logicalLine(first - 1);
    for (int row = first - 1; row < last; ++row)
        setCurrentLineText(row, editor->logicalLine(row + 1));
    setCurrentLineText(last, prev);
    editor->setLogicalCursor(first - 1, 0);
    touchEditor();
    scheduleAutosave();
}

void MainWindow::moveLineDown()
{
    int first, last;
    selectedLineRange(&first, &last);
    if (last >= editor->logicalLineCount() - 1)
        return;
    QString next = editor->logicalLine(last + 1);
    for (int row = last + 1; row > first; --row)
        setCurrentLineText(row, editor->logicalLine(row - 1));
    setCurrentLineText(first, next);
    editor->setLogicalCursor(first + 1, 0);
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
    editor->logicalCursor(&row, &col);
    QString line = editor->logicalLine(row);
    if (isTodoFile())
        line = TodoTxt::toggleDone(line, QDate::currentDate().toString());
    else
        toggleTaskCurrent();
    if (isTodoFile())
        replaceCurrentLine(line);
    scheduleAutosave();
}

void MainWindow::markSelectedTasksDone()
{
    int first, last;
    selectedLineRange(&first, &last);
    for (int row = first; row <= last; ++row) {
        QString line = editor->logicalLine(row);
        int p = line.find("[ ]");
        if (p >= 0) {
            line.replace(p, 3, "[x]");
        } else if (isTodoFile() && line.left(2) != "x ") {
            line = "x " + QDate::currentDate().toString() + " " + line;
        }
        setCurrentLineText(row, line);
    }
    editor->setLogicalCursor(first, 0);
    touchEditor();
    scheduleAutosave();
}

void MainWindow::markSelectedTasksOpen()
{
    int first, last;
    selectedLineRange(&first, &last);
    for (int row = first; row <= last; ++row) {
        QString line = editor->logicalLine(row);
        int p = line.find("[x]");
        if (p < 0)
            p = line.find("[X]");
        if (p >= 0) {
            line.replace(p, 3, "[ ]");
        } else if (isTodoFile() && line.left(2) == "x ") {
            line = line.mid(2);
            if (line.length() >= 11 && line[4] == '-' && line[7] == '-')
                line = line.mid(11);
        }
        setCurrentLineText(row, line);
    }
    editor->setLogicalCursor(first, 0);
    touchEditor();
    scheduleAutosave();
}

void MainWindow::setTodoPriority(const QString &priority)
{
    int first, last;
    selectedLineRange(&first, &last);
    for (int row = first; row <= last; ++row) {
        QString line = editor->logicalLine(row).stripWhiteSpace();
        if (line.length() >= 4 && line[0] == '(' && line[2] == ')' && line[1] >= 'A' && line[1] <= 'Z')
            line = line.mid(4);
        if (!priority.isEmpty())
            line = "(" + priority + ") " + line;
        setCurrentLineText(row, line);
    }
    editor->setLogicalCursor(first, 0);
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
    editor->logicalCursor(&row, &col);
    QString line = editor->logicalLine(row);
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
