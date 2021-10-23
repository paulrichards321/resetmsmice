#ifndef __EVENTLOOPPROCESS
#define __EVENTLOOPPROCESS 
 
#include <QtGlobal>
#include <QEventLoop>
#include <QProcess>
#include <QKeyEvent>
#include <QString>

class EventLoopProcess : public QProcess
{
  Q_OBJECT
public:
  EventLoopProcess(QObject* parent);
  int startFriendlyExec(QString&, QStringList&);
  void killEventLoop(void);
  void readFromStdin(QString& text);
public slots:
  void eventProcessFinished(int, QProcess::ExitStatus); 
protected:
  QEventLoop *eventLoop;
  int procExitCode;
  int procExitStatus;
  bool running;
};

#endif
