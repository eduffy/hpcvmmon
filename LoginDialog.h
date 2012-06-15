
#ifndef LOGIN_DIALOG_H
#define LOGIN_DIALOG_H

#include <QDialog>

class QLineEdit;
class QLabel;
class BusyButton;
class SSHCredentials;

class LoginDialog : public QDialog
{
   Q_OBJECT

public:
   LoginDialog(QWidget *parent);

   void setHost(QString const& host);
   void setUsername(QString const& username);
   void setPassword(QString const& password);

protected slots:
   void authenticate();
   void authorized(SSHCredentials *credentials);
   void denied(QString error);

signals:
   void accept(); // Do not implement
   void accept(SSHCredentials *credentials);
   void reject();
   
private:
   QLineEdit   *wHost;
   QLineEdit   *wUsername;
   QLineEdit   *wPassword;
   BusyButton  *wConnect;
};

#endif  /* LOGIN_DIALOG_H */
