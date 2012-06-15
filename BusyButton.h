
#ifndef BUSY_BUTTON_H
#define BUSY_BUTTON_H

#include <QPushButton>
#include <QString>

class QTimeLine;

class BusyButton : public QPushButton
{
   Q_OBJECT
public:
   BusyButton(QString const& title, QWidget *parent = NULL);
   virtual ~BusyButton();

   void SetBusyState(QString const& busyTitle);
   void StopBusyState();

protected slots:
   void spin(int);

private:
    QString    origTitle;
    QTimeLine *timeline;
};


#endif  /* BUSY_BUTTON_H */
