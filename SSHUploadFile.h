
#ifndef SSH_UPLOAD_FILE_H
#define SSH_UPLOAD_FILE_H

#include <QObject>
#include <QString>

class SSHCredentials;

class SSHUploadFile : public QObject
{
   Q_OBJECT
public:
   SSHUploadFile(SSHCredentials *credentials);
   virtual ~SSHUploadFile();

   void Transfer(QString const& localFile,
                 QString const& remotePath);
   bool TransferNow(QString const& localFile,
                    QString const& remotePath);

private:
   SSHCredentials *sshCredentials;
};

#endif /* SSH_UPLOAD_FILE_H */
