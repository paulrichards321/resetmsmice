#ifndef __EXEC_SAVE
#define __EXEC_SAVE

#include <QtGlobal>
#include <QKeyEvent>
#include <QProcess>
#include <QString>
#include "eventLoopProcess.h"

class ExecSave : public QObject {
  Q_OBJECT
protected:
  EventLoopProcess *process;
  bool running;
  bool quitOnNewline;
  int flags;
  QObject *stdinObj;
  QObject *stdoutObj;
  QString *output;
public:
  ExecSave();
  ~ExecSave();
  void setStdinObj(QObject*);
  void setStdoutObj(QObject*);
  int run(QString& exe, QString* output, int flags);
  bool findIt(const char* exeName, QString& location);
  bool isRunning() { return running; }
  bool isDone() { return (!running); }
  static const int UseStdin;
  static const int UseStdout;
signals:
  void readFromStdout(QString&);
public slots:
  void readyReadStandardOutput();
  void readFromStdin(QString&);
};

#endif
