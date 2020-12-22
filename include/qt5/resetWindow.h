#ifndef RESETWINDOW_H
#define RESETWINDOW_H

#include <QtGlobal>
#include <QMainWindow>
#include "terminalView.h"
#include "ui_resetWindow.h"
#include "execSave.h"

class ResetWindow : public QWidget, private Ui_ResetWindow
{
  Q_OBJECT
protected:
  bool resized;
  bool bufferSet;
  ExecSave execSave;
  TerminalView* terminalText;
  QString endOfLine;
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
protected slots:
  void readFromStdout(QString&);
  void on_enableBootButton_clicked();
  void on_disableBootButton_clicked();
//  void on_enableResumeButton_clicked(const QPoint &pos);
//  void on_disableResumeButton_clicked(const QPoint &pos);
  void on_resetNowButton_clicked();
};

#endif
