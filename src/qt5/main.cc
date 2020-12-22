#include "resetWindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
  QApplication app(argc, argv);
  ResetWindow resetWindow;
  resetWindow.show();
  return app.exec();
}
