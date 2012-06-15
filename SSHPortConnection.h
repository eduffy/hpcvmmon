
#ifndef SSH_PORT_CONNECTION_H
#define SSH_PORT_CONNECTION_H

#include <QThread>
#include <QString>

class SSHCredentials;

class SSHPortConnection : public QThread
{
   Q_OBJECT
public:
   SSHPortConnection(SSHCredentials *credentials);
   virtual ~SSHPortConnection();
   void ForwardPort(int localPort, QString const& remoteHost, int remotePort);

   bool isInitialized() { return initialized == 1; }
   void stop();
protected:
   void run();

private:
   SSHCredentials *sshCredentials;

   int      localPort;
   QString  remoteHost;
   int      remotePort;

   // status flags
   int      initialized;
   int      finished;
};

#endif /* SSH_SESSION_H */
