
#include <iostream>
#include <QHostInfo>
#include "SSHCredentials.moc"

extern "C" int ssh2_auth(const char *hostip,
                         const char *username,
                         const char *password, 
                         char **errmsg);

SSHCredentials::SSHCredentials()
   : authenticated(false)
{
}

SSHCredentials::~SSHCredentials()
{
}


void SSHCredentials::Authenticate(QString const& _hostname,
                                  QString const& _username,
                                  QString const& _password)
{
   authenticated = false;
   hostname = _hostname;
   username = _username;
   password = _password;
   start();
}

void SSHCredentials::run()
{
   char *errmsg;
   std::cerr << "Authenticating "
             << qPrintable(username) << "@"
             << qPrintable(hostname) << "..." << std::endl;
   QHostInfo hostInfo = QHostInfo::fromName(hostname);
   if(hostInfo.addresses().isEmpty()) {
      emit denied(QString("Cannot resolve host `%1'.").arg(hostname));
      return;
   }

   hostip = hostInfo.addresses().first();
   int rc = ssh2_auth(qPrintable(hostip.toString()),
                      qPrintable(username),
                      qPrintable(password),
                      &errmsg);
   if(rc != 0) {
      emit denied(QString::fromAscii(errmsg));
      return;
   }

   authenticated = true;
   emit authorized(this);
}

bool SSHCredentials::isAuthenticated() const
{
   return authenticated;
}

QString SSHCredentials::getHostIPAsString() const
{
   return hostip.toString();
}

QString const& SSHCredentials::getHostname() const
{
   return hostname;
}

QString const& SSHCredentials::getUsername() const
{
   return username;
}

QString const& SSHCredentials::getPassword() const
{
   return password;
}

