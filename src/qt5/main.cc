#include "resetWindow.h"
#include <QApplication>
#include <QDebug>

int main(int argc, char *argv[])
{
  QApplication app(argc, argv);
  ResetWindow resetWindow;
//  resetWindow.show();
  qCritical() << "Running app exec!";
  int status = app.exec();
  qCritical() << "App status:" << status << ". Finished.";
  return status;
}
