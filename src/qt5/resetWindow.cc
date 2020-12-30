#include <QtGlobal>
#include <QDebug>
#include <QTextStream>
#include <QInputDialog>
#include <sys/stat.h>
#include <unistd.h>
#include <libusb-1.0/libusb.h>
#include "resetWindow.h"

static const QString aboutText = 
    "This package fixes scroll wheel issues with certain Wireless Microsoft mice in X.org or OS X (includes KDE, Gnome, and Mac OS X applications), where the vertical wheel scrolls abnormally fast. Only needed if you dual boot between Microsoft Windows and in Mac OS X or a linux distribution.\n\n"
    "Known to fix the vertical scroll wheel issue with the following models (and others related):\n"
    "    Microsoft Wireless Mouse 1000\n"
    "    Microsoft Wireless Optical Desktop 3000\n"
    "    Microsoft Wireless Mobile Mouse 3500\n"
    "    Microsoft Wireless Mobile Mouse 4000\n"
    "    Microsoft Comfort Mouse 4500\n"
    "    Microsoft Wireless Mouse 5000\n"
    "    Microsoft Sculpt Mouse\n\n"
    "This program basically just resets a setting in the mouse through usb communications and then exits.";

ResetWindow::ResetWindow(QWidget *parent) : QWidget(parent)
{ 
  resized = false; 
  bufferSet = false;
  create();
}

void ResetWindow::closeEvent(QCloseEvent *event)
{
  execSave.terminate();
  event->accept();
}


void ResetWindow::setStatusText(QString& text)
{
  statusBootLabel->setText(text);
}


void ResetWindow::getStatusText()
{
  char selfLocation[256];
  QString strStatus = "Reset any Microsoft mice on bootup is currently: ";
  QString enableBootPath, output;
  QString errMsgStr;
  QTextStream errMsg(&errMsgStr); 
  QString scriptDir;

  if (readlink("/proc/self/exe", selfLocation, 255) > 0) {
    struct stat info;
    char *lastDirPos = strrchr(selfLocation, '/');
    if (lastDirPos != NULL) {
      lastDirPos++;
      *lastDirPos = 0;
      scriptDir = selfLocation;
    } else {
      lastDirPos = strrchr(selfLocation, '\\');
      if (lastDirPos != NULL) {
        lastDirPos++;
        *lastDirPos = 0;
        scriptDir = selfLocation;
      } 
    }
    if (scriptDir.length() == 0) {
      enableBootPath = "/usr/sbin/resetmsmice-enable-boot";
    } else {
      enableBootPath = scriptDir;
      enableBootPath += "resetmsmice-enable-boot";
    }
    QByteArray enableBootPathArr = enableBootPath.toUtf8();
    if (stat(enableBootPathArr.data(), &info) != 0) {
      enableBootPath = "";
    }
  }
  if (enableBootPath.length() == 0 && execSave.findIt("resetmsmice-enable-boot", enableBootPath) == false) {
    errMsg << endl << "Error: Cannot find resetmsmice-enable-boot script. Please make sure this script is installed in your PATH." << endl;
    qCritical() << errMsgStr;
    createTerminalView();
    appendTerminalText(errMsgStr);
    strStatus += "UNKNOWN ";
    setStatusText(strStatus);
    return;
  }
  enableBootPath += " --status";
  int exitCode = execSave.run(enableBootPath, &output, 0);
  output = output.trimmed();
  if (exitCode == 0) {
    strStatus += output;
    strStatus += " ";
  } else {
    strStatus += "UNKNOWN ";
    errMsg << endl << "Error getting status: " << strerror(errno) << endl;
    qCritical() << errMsgStr;
    createTerminalView();
    appendTerminalText(errMsgStr);
  }
  setStatusText(strStatus);
}    


bool ResetWindow::createTerminalView()
{
  if (resized && bufferSet) { 
    terminalText->grabKeyboard();
    return true; 
  } else if (resized) {
    return false;
  }
  resized = true;

  int newWidth = width() - 20;
  resize(width(), height()+250);
  QWidget * terminalWidget = new QWidget();
  terminalWidget->resize(newWidth, 210);
  QGraphicsScene* terminalScene = new QGraphicsScene(terminalWidget);
  terminalText = new TerminalView(terminalScene, terminalWidget);
  execSave.setStdinObj(terminalText);
  execSave.setStdoutObj(terminalText);
  terminalText->resize(newWidth, 230);
  grid->addWidget(terminalText, 6, 0, 1, 5);
  terminalText->show();
  terminalText->setFocus();
  endOfLine = terminalText->getEndOfLine();
  bufferSet = true;
 
  return true;
}

void ResetWindow::clearTerminal()
{
  if (bufferSet) {
    terminalText->clear();
  }
}

void ResetWindow::setTerminalText(QString& text)
{
  if (bufferSet) {
    terminalText->setPlainText(text);
  }
}

void ResetWindow::readFromStdout(QString& text)
{
  if (bufferSet) {
    terminalText->appendPlainText(text);
  }
}


void ResetWindow::appendTerminalText(QString& text)
{
  if (bufferSet) {
    terminalText->appendPlainText(text);
  }
}

void ResetWindow::enableBoot(bool enable)
{
  QString sudoCmd, enableBootPath, output;
  QString errMsgStr;
  QTextStream errMsg(&errMsgStr);
  char selfLocation[256];

  createTerminalView();
  clearTerminal();

  if (execSave.findIt("sudo", sudoCmd) == false) { 
     errMsg << "Error: Cannot find sudo binary in PATH. Please make sure policy kit is installed." << endl;
     qCritical() << errMsgStr;
     setTerminalText(errMsgStr);
     return;
  }
    
  if (readlink("/proc/self/exe", selfLocation, 255) > 0) {
    struct stat info;
    if (strstr(selfLocation, "/local/") == NULL) {
      enableBootPath = "/usr/sbin/resetmsmice-enable-boot";
    } else {
      enableBootPath = "/usr/local/sbin/resetmsmice-enable-boot";
    }
    QByteArray enableBootPathArr = enableBootPath.toUtf8();
    if (stat(enableBootPathArr.data(), &info) != 0) {
      enableBootPath = "";
    }
  }
  if (enableBootPath.length() == 0 && execSave.findIt("resetmsmice-enable-boot", enableBootPath) == false) {
    errMsg << "Error: Cannot find resetmsmice-enable-boot script. Please make sure this script is installed in your PATH." << endl;
    qCritical() << errMsgStr;
    setTerminalText(errMsgStr);
    return;
  }

  output.clear();
  
  sudoCmd += " -S ";
  sudoCmd += enableBootPath;
  if (enable == true) {
    sudoCmd += " --enable --quiet";
  } else {
    sudoCmd += " --disable";
  }
  appendTerminalText(sudoCmd);
  appendTerminalText(endOfLine);
  terminalText->setCaptureKeyEvent(true);
  int exitCode = execSave.run(sudoCmd, &output, ExecSave::UseStdout | ExecSave::UseStdin);
  terminalText->setCaptureKeyEvent(false);
  if (exitCode == 0) {
    QString done = "Done.";
    appendTerminalText(done);
    getStatusText();
  } else {
    errMsg << endl << "Error running: " << sudoCmd << endl;
    errMsg << "Error message: " << strerror(errno) << endl;
    appendTerminalText(errMsgStr);
  }
  return;
}


void ResetWindow::on_enableBootButton_clicked()
{
  if (execSave.isRunning() == false) {
    enableBoot(true);
  }
}


void ResetWindow::on_disableBootButton_clicked()
{
  if (execSave.isRunning() == false) {
    enableBoot(false);
  }
}


void ResetWindow::on_resetNowButton_clicked()
{
  if (execSave.isRunning() == false) {
    resetNow();
  }
}


void ResetWindow::resetNow()
{
  char selfLocation[256];
  QString resetmsmiceCmd, output;
  QString errMsgStr;
  QTextStream errMsg(&errMsgStr);
  QString sudoCmd;

  createTerminalView();
  clearTerminal();
  
  if (readlink("/proc/self/exe", selfLocation, 255) > 0) {
    struct stat info;
    if (strstr(selfLocation, "/local/") == NULL) {
      resetmsmiceCmd = "/usr/bin/resetmsmice";
    } else {
      resetmsmiceCmd = "/usr/local/bin/resetmsmice";
    }
    QByteArray resetmsmiceCmdArr = resetmsmiceCmd.toUtf8();
    if (stat(resetmsmiceCmdArr.data(), &info) != 0) {
      resetmsmiceCmd = "";
    }
  }
  if (resetmsmiceCmd.length() == 0 && execSave.findIt("resetmsmice", resetmsmiceCmd) == false) {
    errMsg << "Error: Cannot find resetmsmice binary in PATH." << endl;
    qCritical() << errMsgStr;
    setTerminalText(errMsgStr);
    return;
  }
  output.clear();
	if (execSave.findIt("sudo", sudoCmd) == false) {
    errMsg << "Error: Cannot find sudo binary in PATH. Please make sure policy kit is installed." << endl;
    qCritical() << errMsgStr;
    appendTerminalText(errMsgStr);
    return;
  }

  sudoCmd += " -S ";
  sudoCmd += resetmsmiceCmd;

  appendTerminalText(sudoCmd);
  appendTerminalText(endOfLine);
  terminalText->setCaptureKeyEvent(true);
  execSave.run(sudoCmd, &output, ExecSave::UseStdout | ExecSave::UseStdin);
  terminalText->setCaptureKeyEvent(false);
}


void ResetWindow::create()
{
  grid = new QGridLayout(this);
  about = new QLabel(aboutText);
  about->setWordWrap(true);
  statusBootLabel = new QLabel("");
  enableBootButton = new QPushButton("Enable");  
  disableBootButton = new QPushButton("Disable");
  resetNowButton = new QPushButton("Reset Now");
  labelAuth = new QLabel("You may need to provide authentication to reset the mouse or change boot settings.");

  grid->addWidget(about, 0, 0, 1, 5, Qt::AlignLeft);
  grid->addWidget(statusBootLabel, 1, 0, 1, 3, Qt::AlignLeft);
  grid->addWidget(enableBootButton, 1, 3, 1, 1, Qt::AlignRight);
  grid->addWidget(disableBootButton, 1, 4, 1, 1, Qt::AlignLeft);
  grid->addWidget(labelAuth, 3, 0, 1, 5, Qt::AlignLeft);
  grid->addWidget(resetNowButton, 5, 0, 1, 1, Qt::AlignLeft);
  connect(enableBootButton, SIGNAL(clicked()), this, SLOT(on_enableBootButton_clicked()));
  connect(disableBootButton, SIGNAL(clicked()), this, SLOT(on_disableBootButton_clicked()));
  connect(resetNowButton, SIGNAL(clicked()), this, SLOT(on_resetNowButton_clicked()));
  getStatusText();
  terminalText = NULL;
}

