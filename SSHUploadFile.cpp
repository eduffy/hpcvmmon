
#include <QErrorMessage>
#include <QHostInfo>
#include <QtDebug>
#include "SSHCredentials.h"
#include "SSHUploadFile.moc"

extern "C" int ssh2_scp_write(const char *hostip,
                              const char *username,
                              const char *password,
                              const char *loclfile,
                              const char *scppath,
                              char **errmsg);


SSHUploadFile::SSHUploadFile(SSHCredentials *credentials)
   : sshCredentials(credentials)
{
}

SSHUploadFile::~SSHUploadFile()
{
}


void SSHUploadFile::Transfer(QString const& localFile,
                             QString const& remotePath)
{
}

bool SSHUploadFile::TransferNow(QString const& localFile,
                                QString const& remotePath)
{
   char *errmsg = NULL;

   qDebug() << QString("[SCP] %1 -> %2@%3:%4")
                  .arg(localFile)
                  .arg(sshCredentials->getUsername())
                  .arg(sshCredentials->getHostname())
                  .arg(remotePath);
            
   int rc = ssh2_scp_write(qPrintable(sshCredentials->getHostIPAsString()),
                           qPrintable(sshCredentials->getUsername()),
                           qPrintable(sshCredentials->getPassword()),
                           qPrintable(localFile),
                           qPrintable(remotePath),
                           &errmsg);

   if(rc != 0) {
      qDebug() << "[SCP Error] " << errmsg;
      free(errmsg);
      return false;
   }
   return true;
}

