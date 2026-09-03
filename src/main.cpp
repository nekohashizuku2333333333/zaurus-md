#include "MainWindow.h"

#include <qpe/qpeapplication.h>
#include <qfont.h>

int main(int argc, char **argv)
{
    QPEApplication app(argc, argv);
    app.setFont(QFont("song", 12), true);
    MainWindow mw;
    app.showMainWidget(&mw);
    return app.exec();
}
