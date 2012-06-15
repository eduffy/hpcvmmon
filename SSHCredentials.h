
#ifndef SSH_CREDENTIALS_H
#define SSH_CREDENTIALS_H

#include <QThread>
#include <QString>
#include <QHostAddress>

class SSHCredentials : public QThread
{
   Q_OBJECT
public:
   SSHCredentials();
   virtual ~SSHCredentials();

   void Authenticate(QString const& _hostname,
                     QString const& _username,
                     QString const& _password);

   bool isAuthenticated() const;
   QString getHostIPAsString() const;
   QString const& getHostname() const;
   QString const& getUsername() const;
   QString const& getPassword() const;

protected:
   void run();

signals:
   void authorized(SSHCredentials *credentials);
   void denied(QString error);

private:
   bool         authenticated;
   QHostAddress hostip;
   QString      hostname;
   QString      username;
   QString      password;
};

#endif /* SSH_CREDENTIALS_H */
