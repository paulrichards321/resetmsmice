#include <QtGlobal>
#include <QString>
#include <QDebug>
#include <unistd.h>
#include <sys/stat.h>
#ifdef __APPLE__
#include <sys/wait.h>
#else
#include <wait.h>
#endif
#include "execSave.h"
#include "eventLoopProcess.h"

const int ExecSave::UseStdin = 1;
const int ExecSave::UseStdout = 2;

ExecSave::ExecSave()
{
  running = false;
  process = NULL;
  stdinObj = NULL;
  stdoutObj = NULL;
}


ExecSave::~ExecSave()
{
  if (running && process)
  {
    process->closeReadChannel(QProcess::StandardOutput);
    process->closeReadChannel(QProcess::StandardError);
    process->closeWriteChannel();
    process->close();
    process->killEventLoop();
  }
}


void ExecSave::setStdinObj(QObject *newStdinObj)
{
  stdinObj = newStdinObj;
  connect(stdinObj, SIGNAL(readFromStdin(QString&)), this, SLOT(readFromStdin(QString&)));
  connect(stdinObj, SIGNAL(ctrlcFromStdin()), this, SLOT(ctrlcFromStdin()));
}
  

void ExecSave::setStdoutObj(QObject *newStdoutObj)
{
  stdoutObj = newStdoutObj;
  connect(this, SIGNAL(readFromStdout(QString&)), stdoutObj, SLOT(readFromStdout(QString&)));
}


int ExecSave::run(QString& exe, QString* newOutput, int newFlags)
{
  QStringList strArgs;
  QString rootCmd;
  int start, end, size;
  int count = 0;
 
  running = false;
  output = newOutput;
  flags = newFlags;
  if (newOutput)
  {
    newOutput->clear();
  }
  
  for (start = 0, count = 0; start < exe.length(); count++) {
    end = exe.indexOf(' ', start);
    if (end == -1) {
      size = exe.length() - start;
    } else {
      size = end - start;
    }
    if (count == 0) {
      rootCmd = exe.mid(start, size);
    } else { 
      strArgs << exe.mid(start, size);
    }
    start += size + 1;
  }
  process = new EventLoopProcess(this);
  process->setProcessChannelMode(QProcess::MergedChannels);
  connect(process, SIGNAL(readyReadStandardOutput()), this, SLOT(readyReadStandardOutput()));
  running = true;
  int exitCode = process->startFriendlyExec(rootCmd, strArgs);
  running = false;
  delete process;
  return exitCode;
} 


void ExecSave::readyReadStandardOutput()
{
  QString readStr = process->readAllStandardOutput();
  if (output)
  {
    output->append(readStr);
  }
  if (flags & ExecSave::UseStdout)
  {
    emit readFromStdout(readStr);
  }
}
 

void ExecSave::readFromStdin(QString& text)
{
  if ((flags & ExecSave::UseStdin) && running && process)
  {
    process->readFromStdin(text);
  }
}


void ExecSave::terminate()
{
  if (running && process)
  {
    qWarning() << "Killing current process!";
    process->closeReadChannel(QProcess::StandardOutput);
    process->closeReadChannel(QProcess::StandardError);
    process->closeWriteChannel();
    process->close();
    process->killEventLoop();
  }
}


void ExecSave::ctrlcFromStdin()
{
  terminate();
}


bool ExecSave::findIt(const char *exe_name, QString& location)
{
  QString which = "which ";
  const char *paths[] = { "/usr/local/sbin/", "/usr/local/bin/", "/usr/sbin/",
                    "/usr/bin/", "/sbin/", "/bin/" };
  int i;
  struct stat info;

  which += exe_name;
  location.clear();
  int exitCode = run(which, &location, 0);
  location = location.trimmed();
  if (exitCode == 0) {
    return true;
  }

  for (i = 0; i < 6; i++) {
    location = paths[i];
    location += exe_name;
    QByteArray locationArr = location.toUtf8();
    if (stat(locationArr.data(), &info) == 0) {
      return true;
    }
  }
  return false;
}

