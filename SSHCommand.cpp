
#include <QtDebug>
#include "SSHCredentials.h"
#include "SSHCommand.moc"

extern "C" int ssh2_exec(const char *hostip,
                         const char *username,
                         const char *password, 
                         const char *command,
                         char **result,
                         char **errmsg);


SSHCommand::SSHCommand(SSHCredentials *credentials)
   : sshCredentials(credentials)
{
}

SSHCommand::~SSHCommand()
{
}

void SSHCommand::Execute(QString const& _command)
{
   command = _command;
   start();
}

void SSHCommand::run()
{
   char *result, *errmsg;

   qDebug("[ssh-exec] %s", qPrintable(command));
   Q_ASSERT(sshCredentials && sshCredentials->isAuthenticated());
   int rc = ssh2_exec(qPrintable(sshCredentials->getHostIPAsString()),
                      qPrintable(sshCredentials->getUsername()),
                      qPrintable(sshCredentials->getPassword()),
                      qPrintable(command),
                      &result, &errmsg);

   if(rc != 0) {
      emit failed(QString::fromAscii(errmsg));
      return;
   }

   emit completed(QString::fromAscii(result));
}

QString SSHCommand::ExecuteNow(QString const& _command)
{
   char *result, *errmsg;

   qDebug("[ssh-exec] %s", qPrintable(_command));
   Q_ASSERT(sshCredentials && sshCredentials->isAuthenticated());
   int rc = ssh2_exec(qPrintable(sshCredentials->getHostIPAsString()),
                      qPrintable(sshCredentials->getUsername()),
                      qPrintable(sshCredentials->getPassword()),
                      qPrintable(_command),
                      &result, &errmsg);

   if(rc != 0) {
      qDebug("[ssh-exec-error] %s", errmsg);
   }

   return QString::fromAscii(result);
}
