#ifndef RESETWINDOW_H
#define RESETWINDOW_H

#include <QtGlobal>
#include <QMainWindow>
#include <QWindow>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include "terminalView.h"
#include "execSave.h"

class ResetWindow : public QWidget
{
  Q_OBJECT
protected:
  bool resized;
  bool bufferSet;
  ExecSave execSave;
  TerminalView* terminalText;
  QString endOfLine;
  QGridLayout * grid;
  QLabel * statusBootLabel;  
  QPushButton * enableBootButton;
  QPushButton * disableBootButton;
  QPushButton * resetNowButton;
  QLabel * labelAuth;
  QLabel * about;
//#ifndef Q_OS_MAC
  QIcon mainIcon;
//#endif
public:
  explicit ResetWindow(QWidget *parent = nullptr);
  
  void create();
  void resetNow();
  bool createTerminalView();
  void clearTerminal(); 
  void getStatusText();
  void setTerminalText(QString&);
  void appendTerminalText(QString&);
  void setStatusText(QString&);
  void enableBoot(bool);
protected:
  void closeEvent(QCloseEvent *event);
protected slots:
  void readFromStdout(QString&);
  void on_enableBootButton_clicked();
  void on_disableBootButton_clicked();
  void on_resetNowButton_clicked();
};

#endif
