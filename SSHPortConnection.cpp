
#include <QtDebug>
#include "SSHCredentials.h"
#include "SSHPortConnection.moc"

extern "C" int ssh2_forward_port(const char *hostip,
                                 const char *username,
                                 const char *password,
                                 int listenport,
                                 const char *desthost,
                                 int destport,
                                 int *running,
                                 int *finished,
                                 char **errmsg);

SSHPortConnection::SSHPortConnection(SSHCredentials *credentials)
   : sshCredentials(credentials)
{
}

SSHPortConnection::~SSHPortConnection()
{
}

void SSHPortConnection::ForwardPort(int _localPort,
                                    QString const& _remoteHost,
                                    int _remotePort)
{
   localPort  = _localPort;
   remoteHost = _remoteHost;
   remotePort = _remotePort;
}
                             
void SSHPortConnection::run()
{
   char *errmsg = NULL;

   initialized = 0; // will be set to 1 when accepting connection
   finished = 0;    // set this to 1 to terminate the main loop

   int rc = ssh2_forward_port(qPrintable(sshCredentials->getHostIPAsString()),
                              qPrintable(sshCredentials->getUsername()),
                              qPrintable(sshCredentials->getPassword()),
                              localPort,
                              qPrintable(remoteHost),
                              remotePort,
                              &initialized,
                              &finished,
                              &errmsg);

   if(rc != 0) {
      emit failure(errmsg);
      qDebug() << "[SSH Port Error] " << errmsg;
      free(errmsg);
      finished = 1;
   }
}

void SSHPortConnection::stop()
{
   finished = 1;
   wait(1000);
   quit();
}
