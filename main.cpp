#include <QApplication>

#include "window.h"

int main (int argc, char *argv[])
{
    QApplication app(argc, argv);
    Window w;
    w.loadImage("examples/donkey.png");
    w.show();
    return app.exec();
}

