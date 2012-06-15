
#include <QCloseEvent>
#include "SSHPortConnection.h"
#include "vncview.h"
#include "VNCWindow.moc"

VNCWindow::VNCWindow(SSHCredentials *credentials,
                     int screen_,
                     QString const& host,
                     int port,
                     QWidget *parent)
   : QMainWindow(parent)
   , ssh(credentials)
   , screen(screen_)
{
   ssh.ForwardPort(5900+screen, host, port);
   QUrl url = QString("vnc://localhost:%1").arg(screen);
   qDebug() << url;
   vnc = new VncView(this, url);

   setCentralWidget(vnc);
   setWindowTitle(QString("VNC Connection [%1:%2]")
      .arg(host)
      .arg(port - 5900));
}

VNCWindow::~VNCWindow()
{
}

void VNCWindow::show()
{
   QMainWindow::show();
   ssh.start();
   while(!ssh.isInitialized()) {
      usleep(100);
   }
   vnc->start();
}

void VNCWindow::closeEvent(QCloseEvent *event)
{
   qDebug() << "closed";
   vnc->startQuitting();
   while(vnc->status() != RemoteView::Disconnected) {
      usleep(100);
   }

   ssh.stop();
   event->accept();
   emit closed(screen);
}

