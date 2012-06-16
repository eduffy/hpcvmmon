
#ifndef VNC_WINDOW_H
#define VNC_WINDOW_H

#include <QMainWindow>
#include "SSHPortConnection.h"

class QCloseEvent;
class SSHCredentials;
class VncView;

class VNCWindow : public QMainWindow
{
   Q_OBJECT
public:
   VNCWindow(SSHCredentials *credentials,
             int screen,
             QString const& host,
             int port,
             QWidget *parent = NULL);
   virtual ~VNCWindow();

   virtual void show();

protected:
   virtual void closeEvent(QCloseEvent *ev);

signals:
   void closed(int screen);

private slots:
   void portFailure(QString const& errmsg);

private:
   SSHPortConnection  ssh;
   VncView           *vnc;
   int                screen;
};

#endif /* VNC_WINDOW_H */
