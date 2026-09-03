#include "MainWindow.h"

#include <qpe/qpeapplication.h>
#include <qfont.h>

int main(int argc, char **argv)
{
    QPEApplication app(argc, argv);
    app.setFont(QFont("song", 12), true);
    MainWindow mw;
    if (argc > 1)
        mw.openInitialFile(QString::fromLocal8Bit(argv[1]));
    app.showMainWidget(&mw);
    return app.exec();
}
