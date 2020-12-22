#include "terminalView.h"
#include <QtMath>
#include <QFontDatabase>
#include <QTextStream>
#include <QPen>
#include <QStyle>
#include <QDebug>

TerminalView::TerminalView(QGraphicsScene *scene, QWidget *parent) : QGraphicsView(scene, parent)
{
  QPalette pal = palette();
  QTextStream ts(&endOfLine);
  ts << endl;

  fixedFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
  QFontMetrics fontMetrics(fixedFont);

  cursorWidth = fontMetrics.maxWidth();
  cursorHeight = fontMetrics.ascent();
  charMarginWidth = 0;
  charMarginHeight = fontMetrics.descent();
  fixedCharWidth = cursorWidth + charMarginWidth;
  fixedCharHeight = cursorHeight + charMarginHeight;

  maxCharCount = 0;
  lineCount = 0;
  xStart = 0;
  yStart = 0;
  basePxWidth = 0;
  basePxHeight = 0;

  scrollBarExtent = style()->pixelMetric(QStyle::PM_ScrollBarExtent);  
  setupViewport(parent);
  setScene(scene);
  setInteractive(true);
  setDragMode(QGraphicsView::NoDrag);
  setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
  setCacheMode(QGraphicsView::CacheNone);
 
  horizScroll = horizontalScrollBar();
  vertScroll = verticalScrollBar();
  connect((QObject*)horizScroll, SIGNAL(valueChanged(int)), this, SLOT(horizScrollChanged(int)));
  connect((QObject*)vertScroll, SIGNAL(valueChanged(int)), this, SLOT(vertScrollChanged(int)));
  QBrush brush(Qt::white); 
  setBackgroundBrush(brush);
  setFocusPolicy(Qt::StrongFocus);
  setFocus();
  text = "";
}


QString TerminalView::getEndOfLine()
{
  return endOfLine;
}


void TerminalView::horizScrollChanged(int value)
{
  xStart = value;
}


void TerminalView::vertScrollChanged(int value)
{
  yStart = value;
}


void TerminalView::resizeEvent(QResizeEvent *event) 
{
  QWidget::resizeEvent(event);
  updateTextSceneRect();
}


void TerminalView::keyPressEvent(QKeyEvent *event)
{
  if (captureKeyEvent && event)
  {
    QString text = event->text();
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
    {
      appendPlainText(endOfLine);
    }
    emit readFromStdin(text);
  }
}


void TerminalView::setCaptureKeyEvent(bool newCaptureKeyEvent)
{
  captureKeyEvent = newCaptureKeyEvent;
}



void TerminalView::updateTextSceneRect()
{
  qreal xMax = width() - 2;
  qreal yMax = height() - 2;
  if (basePxWidth > xMax) xMax = basePxWidth;
  if (basePxHeight > yMax) yMax = basePxHeight;
  QRectF rect(0, 0, xMax, yMax);
  qreal xMin = xStart;
  qreal yMin = yStart;
  xMax = width() - 2;
  yMax = height() - 2;
  if (basePxWidth > xMax)
  {
    xMax -= (scrollBarExtent + 2);
  }
  if (basePxHeight > yMax)
  {
    yMax -= (scrollBarExtent + 2);
  }
  QRectF rect2(xMin, yMin, xMax, yMax);
  setSceneRect(rect);
  ensureVisible(rect2, 0, 0);
  QList<QRectF> all;
  all.append(rect2);
  updateScene(all);
}


void TerminalView::calcTextSceneRect()
{
  int textLen = text.length();
  int i = 0;
  lineCount = 0;
  basePxWidth = 0;
  basePxHeight = 0;
  maxCharCount = 0;
  subStrs.clear();
  while (i < textLen)
  {
    int next = text.indexOf(endOfLine, i);
    if (next == -1) 
    {
      next = textLen;
    }
    lineCount++;
    
    subStrs.push_back(text.mid(i, next - i));
    int charCount = next - i;
    if (charCount > maxCharCount) maxCharCount = charCount;
    i = next+endOfLine.length();
  }  
  if (lineCount > 0 && maxCharCount > 0)
  {
    basePxHeight = (lineCount+1) * fixedCharHeight;
    basePxWidth = (maxCharCount+1) * fixedCharWidth;
  }
}


void TerminalView::drawForeground(QPainter *painter, const QRectF &destRect)
{
  QPen pen = painter->pen();
  QColor blackish(0, 0, 0); 
  pen.setColor(blackish);
  painter->setPen(pen);
  painter->setFont(fixedFont);
  int x= qRound(destRect.x());
  if (x < 0) x = 0;
  int y= qRound(destRect.y());
  if (y < 0) y = 0;
  int width= qCeil(destRect.width());
  int height= qCeil(destRect.height());
 
  int startLine = qFloor((qreal) y / fixedCharHeight);
  int startChar = qFloor((qreal) x / fixedCharWidth);
  int endLine = qCeil((qreal) (y + height) / fixedCharHeight);
  int endChar = qCeil((qreal) (x + width) / fixedCharWidth);
  y = startLine * fixedCharHeight;
  x = startChar * fixedCharWidth;
  if (endLine > lineCount)
  {
    endLine = lineCount;
  }
  if (startLine < 0)
  {
    startLine = 0;
  }
  height = (endLine - startLine) * fixedCharHeight;
  width = (endChar - startChar) * fixedCharWidth;
  int endCharB = 0;
  for (int line = startLine; line < endLine; line++)
  {
    QString subStr = subStrs[line];
    int subStrLen = subStr.length();
    if (subStrLen > startChar)
    {
      endCharB = subStrLen;
      if (endCharB > endChar)
      {
        endCharB = endChar;
      }
      subStr = subStr.mid(startChar, endChar);
      painter->drawText(x, y, width, height, 0, subStr);
    }
    y += fixedCharHeight; 
  }
  if (endLine > 0 && endChar > 0) 
  {
    QColor darkGrey(128, 128, 128);
    int a = (endCharB * fixedCharWidth) + 1;
    int b = ((endLine-1) * fixedCharHeight) + 1;
    painter->fillRect(a, b, cursorWidth, cursorHeight, darkGrey);
  }
}



void TerminalView::appendPlainText(QString &newText)
{
  text.append(newText);
  calcTextSceneRect();
  updateTextSceneRect();
}


void TerminalView::readFromStdout(QString &newText)
{
  appendPlainText(newText);
}


void TerminalView::setPlainText(QString &newText)
{
  text = newText;
  calcTextSceneRect();
  updateTextSceneRect();
}


void TerminalView::clear()
{
  text.clear();
  calcTextSceneRect();
  updateTextSceneRect();
}


TerminalView::~TerminalView()
{
}

