#include "MainWindow.h"

#include <qpe/qpeapplication.h>

int main(int argc, char **argv)
{
    QPEApplication app(argc, argv);
    MainWindow mw;
    app.showMainWidget(&mw);
    return app.exec();
}

