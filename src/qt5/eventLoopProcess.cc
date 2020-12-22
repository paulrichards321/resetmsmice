#include "eventLoopProcess.h"
#include <QDebug>

int EventLoopProcess::startFriendlyExec(QString& rootCmd, QStringList& strArgs)
{
  running = false;
  procExitCode = 0;
  procExitStatus = 0;
  eventLoop = new QEventLoop(this);
  connect(this, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(eventProcessFinished(int, QProcess::ExitStatus)));
  running = true;
  start(rootCmd, strArgs);
  eventLoop->exec();
  running = false;
  delete eventLoop;
  eventLoop = NULL;
  return procExitCode;
}


void EventLoopProcess::readFromStdin(QString& text)
{
  if (running)
  {
    QByteArray charArray = text.toUtf8();
    write(charArray);
  }
}


void EventLoopProcess::eventProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
  if (running && eventLoop)
  {
    eventLoop->quit();
  }
  procExitCode = exitCode;
  procExitStatus = exitStatus;
}


void EventLoopProcess::killEventLoop()
{
  if (running && eventLoop)
  {
    eventLoop->quit();
  }
}


EventLoopProcess::EventLoopProcess(QObject *parent) : QProcess(parent)
{
  procExitCode = 0;
  procExitStatus = 0;
  eventLoop = NULL;
  running = false;
}

