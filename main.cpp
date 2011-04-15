#include <QApplication>

#include "window.h"
#include "patchmatchwindow.h"

int main (int argc, char *argv[])
{
    QApplication app(argc, argv);

    Window w;
    PatchMatchWindow pmw;

    if (argc == 2) {
        w.loadImage(argv[1]);
        w.show();
    } else if (argc == 3) {
        pmw.loadDst(argv[1]);
        pmw.loadSrc(argv[2]);
        pmw.show();
    }

    return app.exec();
}

