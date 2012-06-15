
#ifndef SSH_COMMAND_H
#define SSH_COMMAND_H

#include <QThread>
#include <QString>

class SSHCredentials;

class SSHCommand : public QThread
{
   Q_OBJECT
public:
   SSHCommand(SSHCredentials *credentials);
   virtual ~SSHCommand();
   void Execute(QString const& _command);
   QString ExecuteNow(QString const& _command);

signals:
   void completed(QString);
   void failed(QString);

protected:
   void run();

private:
   SSHCredentials *sshCredentials;
   QString         command;
};

#endif /* SSH_COMMAND_H */
