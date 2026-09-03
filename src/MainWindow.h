#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <qmainwindow.h>
#include <qstring.h>

class QListView;
class QListViewItem;
class MdEdit;
class QPushButton;
class QWidgetStack;
class MdView;
class QButton;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = 0, const char *name = 0);

private slots:
    void openSelected(QListViewItem *item);
    void showBrowser();
    void showEditor();
    void showView();
    void saveFile();
    void toggleTask(int lineNumber);
    void insertTask();
    void newFile();
    void goUp();
    void wrapBold();
    void wrapItalic();
    void wrapCode();
    void cycleHeading();
    void toggleBullet();
    void toggleNumber();
    void toggleTaskCurrent();
    void quoteLine();
    void insertLink();
    void insertRule();
    void indentLine();
    void outdentLine();
    void insertDate();
    void insertTime();
    void undoEdit();
    void redoEdit();
    void saveAsFile();
    void renameFile();
    void deleteFile();
    void findText();
    void replaceText();
    void moveLineUp();
    void moveLineDown();
    void moveDoneTasksToEnd();
    void clearDoneTasks();
    void todoToggleDone();
    void todoPriorityA();
    void todoPriorityB();
    void todoPriorityC();
    void todoProject();
    void todoContext();
    void todoDue();

private:
    void buildUi();
    QPushButton *makeButton(QWidget *parent, const char *text, const char *slot);
    void loadDirectory(const QString &path);
    void openFile(const QString &path);
    void refreshView();
    QString selectedText() const;
    void replaceSelectionOrInsert(const QString &text);
    void wrapSelection(const QString &before, const QString &after);
    void replaceCurrentLine(const QString &line);
    void setCurrentLineText(int lineNo, const QString &line);
    void applyLinePrefix(const QString &prefix, bool numbered);
    void replaceAllText(const QString &text, int cursorLine);
    QString currentSelectedPath() const;
    bool isTodoFile() const;
    void setTodoPriority(const QString &priority);
    void appendToCurrentLine(const QString &text);
    void touchEditor();

    QString notesDir;
    QString currentDir;
    QString currentFile;
    QListView *browser;
    QWidgetStack *stack;
    MdEdit *editor;
    MdView *view;
    QPushButton *modeButton;
};

#endif
