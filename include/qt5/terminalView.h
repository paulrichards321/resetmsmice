#ifndef __TERMINALVIEW
#define __TERMINALVIEW
#include <QtGlobal>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QVector>
#include <QWidget>
#include <QPaintEvent>
#include <QSize>
#include <QFont>
#include <QRect>
#include <QKeyEvent>
#include <QPainter>

class TerminalView : public QGraphicsView
{
  Q_OBJECT
public:
  explicit TerminalView(QGraphicsScene *scene, QWidget *parent);
  ~TerminalView();
  void drawForeground(QPainter*, const QRectF&) override;
  void resizeEvent(QResizeEvent *event) override;
  void keyPressEvent(QKeyEvent *event) override;
  void appendPlainText(QString &newText);
  void setPlainText(QString &newText);
  void clear();
  void updateTextSceneRect();
  void calcTextSceneRect();
  void setCaptureKeyEvent(bool);
  signals:
  void readFromStdin(QString&);
public slots:
  void horizScrollChanged(int value);
  void vertScrollChanged(int value);
  void readFromStdout(QString&);
  QString getEndOfLine();
protected:
  QFont fixedFont;
  QString text;
  QString endOfLine;
  QVector<QString> subStrs;
  QScrollBar *horizScroll, *vertScroll;
  int scrollBarExtent;
  int cursorWidth, cursorHeight;
  int charMarginWidth, charMarginHeight;
  int fixedCharWidth, fixedCharHeight;
  int maxCharCount;
  int lineCount;
  int xStart, yStart;
  int basePxWidth, basePxHeight;
  bool captureKeyEvent;
};
#endif
