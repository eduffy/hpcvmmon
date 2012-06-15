
#include <libssh2.h>
#include <QApplication>
#include "SessionsWindow.h"

int main(int argc, char *argv[])
{
   QApplication app(argc, argv);
   SessionsWindow w;
   libssh2_init(0);
   w.show();
   return app.exec();
}
