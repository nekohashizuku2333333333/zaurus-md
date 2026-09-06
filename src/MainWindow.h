#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <qmainwindow.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qvaluelist.h>

class QListView;
class QListViewItem;
class MdEdit;
class QToolButton;
class QWidgetStack;
class MdView;
class QButton;
class QTimer;
class QLabel;
class QLineEdit;
class QCloseEvent;
class QResizeEvent;
class QKeyEvent;
class DocLnk;
class FileSelector;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = 0, const char *name = 0, WFlags flags = 0);
    void openInitialFile(const QString &path);

public slots:
    void setDocument(const QString &path);

protected:
    void closeEvent(QCloseEvent *event);
    void resizeEvent(QResizeEvent *event);
    void keyPressEvent(QKeyEvent *event);

private slots:
    void openSelected(QListViewItem *item);
    void showBrowser();
    void showEditor();
    void showView();
    void showSplit();
    void showTodo();
    void addTodoFromInput();
    void toggleTodoImportant();
    void toggleTodoMyDay();
    void setTodoDue();
    void moveTodoUp();
    void moveTodoDown();
    void editTodoCurrent();
    void demoteTodoCurrent();
    void promoteTodoCurrent();
    void deleteTodoCurrent();
    void addTodoStep();
    void todoItemClicked(QListViewItem *item);
    void saveFile();
    void toggleTask(int lineNumber);
    void insertTask();
    void newFile();
    void goBack();
    void goForward();
    void goUp();
    void showRoot();
    void showDocumentLibrary();
    void openDocumentLibraryFile(const DocLnk &doc);
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
    void nextToolPage();
    void tool0();
    void tool1();
    void tool2();
    void tool3();
    void tool4();
    void tool5();
    void tool6();
    void tool7();
    void tool8();
    void tool9();

private:
    void buildUi();
    QToolButton *makeButton(QWidget *parent, const char *text, const char *slot);
    QToolButton *makeTopButton(QWidget *parent, const char *text, const char *icon, const char *slot);
    void rebuildToolBar();
    void setToolLabel(int index, const char *text);
    void setToolButton(int index, const char *icon, const char *text);
    void runTool(int index);
    void layoutToolButtons();
    void layoutTopBars();
    bool confirmSaveIfNeeded();
    void updateCaption();
    void updateSaveIndicator();
    void loadDirectory(const QString &path, bool remember = true);
    void addLocationItem(const QString &name, const QString &path);
    void addStorageLocations();
    void openFile(const QString &path);
    void openFileInNewWindow(const QString &path);
    bool isEditableFileName(const QString &name) const;
    void refreshView();
    void refreshSplit();
    void refreshTodo();
    void saveTodoDoc();
    bool isMarkdownTodoFile() const;
    void applyMarkdownAction(int actionId);
    int editorByteOffset(int line, int col) const;
    void byteOffsetToCursor(const QString &text, int bytes, int *line, int *col) const;
    QString selectedText() const;
    void replaceSelectionOrInsert(const QString &text);
    void wrapSelection(const QString &before, const QString &after);
    void toggleInlineMarkup(const QString &before, const QString &after);
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
    QStringList dirHistory;
    int dirHistoryIndex;
    QListView *browser;
    FileSelector *documentSelector;
    QWidgetStack *stack;
    QWidget *splitPane;
    QWidget *todoPane;
    MdEdit *editor;
    MdView *view;
    MdView *splitView;
    QListView *todoList;
    QLineEdit *todoAdd;
    QToolButton *modeButton;
    QToolButton *todoButton;
    QToolButton *splitButton;
    QLabel *saveIndicator;
    QWidget *fileBar;
    QWidget *docBar;
    QWidget *toolBar;
    QToolButton *toolButtons[10];
    QTimer *autosaveTimer;
    QString lastFind;
    QString todoFilter;
    QValueList<int> previewLineMap;
    bool darkTheme;
    bool hideDone;
    int todoViewMode;
    int toolPage;
    int fontSize;
};

#endif
