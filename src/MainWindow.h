#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <qmainwindow.h>
#include <qstring.h>
#include <qvaluelist.h>

class QListView;
class QListViewItem;
class MdEdit;
class QPushButton;
class QWidgetStack;
class MdView;
class QButton;
class QTimer;

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
    void insertImage();
    void insertTable();
    void insertCodeBlock();
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
    void newFolder();
    void findText();
    void replaceText();
    void replaceOne();
    void findNext();
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
    void todoSortPriority();
    void toggleHideDone();
    void filterTodoProject();
    void filterTodoContext();
    void clearTodoFilter();
    void markSelectedTasksDone();
    void markSelectedTasksOpen();
    void copyText();
    void cutText();
    void pasteText();
    void selectAllText();
    void duplicateLine();
    void fontBigger();
    void fontSmaller();
    void toggleTheme();
    void autosaveTick();
    void smartNewLine();

private:
    void buildUi();
    QPushButton *makeButton(QWidget *parent, const char *text, const char *slot);
    QPushButton *makeTopButton(QWidget *parent, const char *text, const char *slot);
    void loadDirectory(const QString &path);
    void openFile(const QString &path);
    void refreshView();
    QString selectedText() const;
    void replaceSelectionOrInsert(const QString &text);
    void wrapSelection(const QString &before, const QString &after);
    void replaceCurrentLine(const QString &line);
    void setCurrentLineText(int lineNo, const QString &line);
    void applyLinePrefix(const QString &prefix, bool numbered);
    bool selectedLineRange(int *firstLine, int *lastLine) const;
    void applyLineRangePrefix(const QString &prefix, bool numbered);
    void transformLineRange(const QString &mode);
    QString continuationForLine(const QString &line) const;
    void replaceAllText(const QString &text, int cursorLine);
    QString currentSelectedPath() const;
    bool isTodoFile() const;
    void setTodoPriority(const QString &priority);
    void appendToCurrentLine(const QString &text);
    void applyFontSize();
    void applyTheme();
    void scheduleAutosave();
    QString stripTodoPriority(const QString &line) const;
    int todoPriorityRank(const QString &line) const;
    QString withoutDoneLines(const QString &text) const;
    QString filteredPreviewText(const QString &text);
    bool lineMatchesTodoFilter(const QString &line) const;
    void touchEditor();

    QString notesDir;
    QString currentDir;
    QString currentFile;
    QListView *browser;
    QWidgetStack *stack;
    MdEdit *editor;
    MdView *view;
    QPushButton *modeButton;
    QWidget *fileBar;
    QWidget *docBar;
    QTimer *autosaveTimer;
    QString lastFind;
    QString todoFilter;
    QValueList<int> previewLineMap;
    bool darkTheme;
    bool hideDone;
    int fontSize;
};

#endif
