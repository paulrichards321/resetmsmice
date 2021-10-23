#include <QtGlobal>
#include <QDebug>
#include <QTextStream>
#include <QInputDialog>
#include <QIcon>
#include <sys/stat.h>
#include <unistd.h>
#include "resetWindow.h"

#ifdef Q_OS_MAC  
#include <CoreServices/CoreServices.h>
#endif

static const QString aboutText = 
    "This package fixes scroll wheel issues with certain Wireless Microsoft mice in Linux, where the vertical wheel scrolls abnormally fast. Only needed if you dual boot between Microsoft Windows or a linux distribution.\n\n"
    "Known to fix the vertical scroll wheel issue with the following models (and others related):\n"
    "    Microsoft Wireless Mouse 1000\n"
    "    Microsoft Wireless Optical Desktop 3000\n"
    "    Microsoft Wireless Mobile Mouse 3500\n"
    "    Microsoft Wireless Mobile Mouse 4000\n"
    "    Microsoft Comfort Mouse 4500\n"
    "    Microsoft Wireless Mouse 5000\n"
    "    Microsoft Sculpt Mouse\n\n"
    "This program basically just resets a setting in the mouse through usb communications and then exits.";

ResetWindow::ResetWindow(QWidget *parent) : QWidget(parent, Qt::Window)
{ 
  resized = false; 
  bufferSet = false;
  create();
  //show();
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
  QString strStatus = "Reset any Microsoft mice on bootup is currently: ";
  QString enableBootPath, output;
  QString errMsgStr;
  QTextStream errMsg(&errMsgStr); 
  QString scriptDir;
  struct stat info;

  #ifdef Q_OS_MAC
  const char* filePath = NULL; 
  CFURLRef appUrlRef = CFBundleCopyResourceURL(CFBundleGetMainBundle(), CFSTR("resetmsmice-enable-boot"), NULL, NULL);
  if (appUrlRef) {
    CFStringRef filePathRef = CFURLCopyPath(appUrlRef);
    filePath = CFStringGetCStringPtr(filePathRef, kCFStringEncodingUTF8);
    enableBootPath = filePath;
    CFRelease(filePathRef);
    CFRelease(appUrlRef);
  }
  if (filePath == NULL) {
    filePath = "/usr/local/sbin/resetmsmice-enable-boot";
    enableBootPath = filePath;
  }
  
  #else // Linux

  char selfLocation[256];

  if (readlink("/proc/self/exe", selfLocation, 255) > 0) {
    if (strstr(selfLocation, "/local/") == NULL) {
      enableBootPath = "/usr/sbin/resetmsmice-enable-boot";
    } else {
      enableBootPath = "/usr/local/sbin/resetmsmice-enable-boot";
    }
  }

  #endif

  QByteArray enableBootPathArr(enableBootPath.toUtf8());
  if (stat(enableBootPathArr.data(), &info) != 0) {
    enableBootPath = "";
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
  qCritical() << "Running: " << enableBootPath;
  int exitCode = execSave.run(enableBootPath, &output, 0);
  output = output.trimmed();
  if (exitCode == 0) {
    qCritical() << "status:" << output;
    strStatus += output;
    strStatus += " ";
  } else {
    strStatus += "UNKNOWN ";
    errMsg << endl << "Error getting status: " << strerror(errno) << endl;
    qCritical() << errMsgStr;
    createTerminalView();
    appendTerminalText(errMsgStr);
  }
  qCritical() << "Setting status text...";
  setStatusText(strStatus);
  qCritical() << "Set status text!";
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
  QString scriptDir;
  struct stat info;

  createTerminalView();
  clearTerminal();

  if (execSave.findIt("sudo", sudoCmd) == false) { 
     errMsg << "Error: Cannot find sudo binary in PATH. Please make sure policy kit is installed." << endl;
     qCritical() << errMsgStr;
     setTerminalText(errMsgStr);
     return;
  }
    
  #ifdef Q_OS_MAC
  CFURLRef appUrlRef = CFBundleCopyResourceURL(CFBundleGetMainBundle(), CFSTR("resetmsmice-enable-boot"), NULL, NULL);
  CFStringRef filePathRef = CFURLCopyPath(appUrlRef);
  const char* filePath = CFStringGetCStringPtr(filePathRef, kCFStringEncodingUTF8);
  if (filePath == NULL) {
    filePath = "/usr/local/sbin/resetmsmice-enable-boot";
  }
  enableBootPath = filePath;
  CFRelease(filePathRef);
  CFRelease(appUrlRef);
  
  #else // Linux

  char selfLocation[256];

  if (readlink("/proc/self/exe", selfLocation, 255) > 0) {
    if (strstr(selfLocation, "/local/") == NULL) {
      enableBootPath = "/usr/sbin/resetmsmice-enable-boot";
    } else {
      enableBootPath = "/usr/local/sbin/resetmsmice-enable-boot";
    }
  }

  #endif
  
  QByteArray enableBootPathArr(enableBootPath.toUtf8());
  if (stat(enableBootPathArr.data(), &info) != 0) {
    enableBootPath = "";
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
  QString resetmsmiceCmd, output, exeDir;
  QString errMsgStr;
  QTextStream errMsg(&errMsgStr);
  QString sudoCmd;
  struct stat info;

  createTerminalView();
  clearTerminal();
  
  #ifdef Q_OS_MAC
  CFURLRef appUrlRef = CFBundleCopyResourceURL(CFBundleGetMainBundle(), CFSTR("resetmsmice"), NULL, NULL);
  CFStringRef filePathRef = CFURLCopyPath(appUrlRef);
  const char* filePath = CFStringGetCStringPtr(filePathRef, kCFStringEncodingUTF8);
  if (filePath == NULL) {
    filePath = "/usr/local/bin/resetmsmice";
  }
  resetmsmiceCmd = filePath;
  CFRelease(filePathRef);
  CFRelease(appUrlRef);
  
  #else // Linux

  char selfLocation[256];

  if (readlink("/proc/self/exe", selfLocation, 255) > 0) {
    char *lastDirPos = strrchr(selfLocation, '/');
    if (lastDirPos != NULL) {
      lastDirPos++;
      *lastDirPos = 0;
      exeDir = selfLocation;
    } else {
      lastDirPos = strrchr(selfLocation, '\\');
      if (lastDirPos != NULL) {
        lastDirPos++;
        *lastDirPos = 0;
        exeDir = selfLocation;
      } 
    }
    if (exeDir.length() == 0) {
      resetmsmiceCmd = "/usr/local/bin/resetmsmice";
    } else {
      resetmsmiceCmd = exeDir;
      resetmsmiceCmd += "resetmsmice";
    }
  }
  #endif
  
  QByteArray resetmsmiceCmdArr(resetmsmiceCmd.toUtf8());
  if (stat(resetmsmiceCmdArr.data(), &info) != 0) {
    resetmsmiceCmd = "";
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
  QString iconPath;  
  setWindowModality(Qt::ApplicationModal);
  setWindowTitle("resetmsmice-gui");
#ifdef Q_OS_MAC
  CFURLRef appUrlRef = CFBundleCopyResourceURL(CFBundleGetMainBundle(), CFSTR("resetmsmice.icns"), NULL, NULL);
  if (appUrlRef) {
    CFStringRef filePathRef = CFURLCopyPath(appUrlRef);
    const char * filePath = CFStringGetCStringPtr(filePathRef, kCFStringEncodingUTF8);
    iconPath = filePath;
    CFRelease(filePathRef);
    CFRelease(appUrlRef);
    mainIcon = QIcon(iconPath);
    if (mainIcon.isNull()) {
      qCritical() << "Failed to load main from app bundle!";
    } else {
      setWindowIcon(mainIcon);
      qCritical() << "Successfully loaded main icon from path.";
    }
  }
#else
  iconPath = "/usr/share/icons/hicolor/scalable/apps/resetmsmice.svg"; 

  mainIcon = QIcon(iconPath);
  if (mainIcon.isNull()) {
    qCritical() << "Failed to load main icon from path  '/usr/share/icons/hicolor/scalable/apps/resetmsmice.svg'!";
  } else {
    setWindowIcon(mainIcon);
    qCritical() << "Successfully loaded main icon from path.";
  }
#endif
  /*
  QWidget * mainWidget = new QWidget(this);
  setCentralWidget(mainWidget);
  */
  grid = new QGridLayout(this);
  setLayout(grid);
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
  show();
  //mainWidget->show();
  getStatusText();
  terminalText = NULL;
}

