#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <qmainwindow.h>
#include <qstring.h>

class QListView;
class QListViewItem;
class QMultiLineEdit;
class QPushButton;
class QWidgetStack;
class MdView;

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

private:
    void buildUi();
    void loadDirectory(const QString &path);
    void openFile(const QString &path);
    void refreshView();

    QString notesDir;
    QString currentDir;
    QString currentFile;
    QListView *browser;
    QWidgetStack *stack;
    QMultiLineEdit *editor;
    MdView *view;
    QPushButton *modeButton;
};

#endif

