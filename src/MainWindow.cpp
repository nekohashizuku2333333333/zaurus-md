#include "MainWindow.h"

#include "FileUtil.h"
#include "MdParser.h"
#include "MdView.h"

#include <qdir.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qlistview.h>
#include <qmultilineedit.h>
#include <qpushbutton.h>
#include <qstatusbar.h>
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
    modeButton = new QPushButton("View", top);
    connect(modeButton, SIGNAL(clicked()), this, SLOT(showView()));
    QPushButton *save = new QPushButton("Save", top);
    connect(save, SIGNAL(clicked()), this, SLOT(saveFile()));

    stack = new QWidgetStack(root);
    browser = new QListView(stack);
    browser->addColumn("Name");
    browser->addColumn("Modified");
    connect(browser, SIGNAL(doubleClicked(QListViewItem *)), this, SLOT(openSelected(QListViewItem *)));

    editor = new QMultiLineEdit(stack);
    view = new MdView(stack);
    connect(view, SIGNAL(toggleTask(int)), this, SLOT(toggleTask(int)));

    stack->addWidget(browser, 0);
    stack->addWidget(editor, 1);
    stack->addWidget(view, 2);

    QHBox *bar = new QHBox(root);
    QPushButton *task = new QPushButton("[ ]", bar);
    connect(task, SIGNAL(clicked()), this, SLOT(insertTask()));
    QPushButton *edit = new QPushButton("Edit", bar);
    connect(edit, SIGNAL(clicked()), this, SLOT(showEditor()));
    QPushButton *preview = new QPushButton("View", bar);
    connect(preview, SIGNAL(clicked()), this, SLOT(showView()));

    stack->raiseWidget(browser);
}

void MainWindow::loadDirectory(const QString &path)
{
    currentDir = path;
    browser->clear();
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
