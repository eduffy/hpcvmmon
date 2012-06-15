
/* Based on
 * http://qt-project.org/wiki/Busy_Indicator_for_QML
 */

#include <QTimeLine>
#include <QIcon>
#include <QPixmap>
#include <QPainterPath>
#include <QPainter>
#include "BusyButton.moc"

BusyButton::BusyButton(QString const& title, QWidget *parent)
   : QPushButton(title, parent)
   , origTitle(title)
{
   timeline = new QTimeLine(1000, this);
   timeline->setCurveShape(QTimeLine::LinearCurve);
   timeline->setLoopCount(0);
   timeline->setFrameRange(0, 36);
   connect(timeline, SIGNAL(frameChanged(int)), this, SLOT(spin(int)));
}

BusyButton::~BusyButton()
{
}


void BusyButton::spin(int value)
{
   QColor bgColor(177, 210, 143, 70);
   QColor fgColor(119, 183, 83, 255);
   qreal  innerRadius(5.0);
   qreal  outerRadius(7.0);

   // Set up a convenient path
   QPainterPath path;
   path.setFillRule(Qt::OddEvenFill);
   path.addEllipse(QPointF(outerRadius, outerRadius), outerRadius, outerRadius);
   path.addEllipse(QPointF(outerRadius, outerRadius), innerRadius, innerRadius);
 
   qreal diameter = 2 * outerRadius;
   QPixmap pixmap(diameter, diameter);
   pixmap.fill(Qt::transparent);
   QPainter p(&pixmap);
   p.setTransform(QTransform()
                     .translate(outerRadius, outerRadius)
                     .rotate(10 * value)
                     .translate(-outerRadius, -outerRadius));
 
   // Draw the ring background
   p.setPen(Qt::NoPen);
   p.setBrush(bgColor);
   p.setRenderHint(QPainter::Antialiasing);
   p.drawPath(path);
 
   // Draw the ring foreground
   QConicalGradient gradient(QPointF(outerRadius, outerRadius), 0.0);
   gradient.setColorAt(0.00, Qt::transparent);
   gradient.setColorAt(0.05, fgColor);
   gradient.setColorAt(0.80, Qt::transparent);
   p.setBrush(gradient);
   p.drawPath(path);
   p.end();

   setIcon(QIcon(pixmap));
   setIconSize(QSize(16, 16));
}

void BusyButton::SetBusyState(QString const& busyTitle)
{
   timeline->start();
   setText(busyTitle);
   setDisabled(true);
}

void BusyButton::StopBusyState()
{
   setIcon(QIcon());
   timeline->stop();
   setText(origTitle);
   setDisabled(false);
}

