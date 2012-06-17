
#include <iostream>
#include <QLabel>
#include <QLineEdit>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QErrorMessage>
#include "BusyButton.h"
#include "SSHCredentials.h"
#include "LoginDialog.moc"

LoginDialog::LoginDialog(QWidget *parent)
   : QDialog(parent)
{
   QFormLayout *layout = new QFormLayout(this);
   layout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

   wHost = new QLineEdit();
   wHost->resize(150, wHost->height());
   layout->addRow(tr("&Host: "), wHost);

   wUsername = new QLineEdit();
   layout->addRow(tr("&Username: "), wUsername);

   wPassword = new QLineEdit();
   wPassword->setEchoMode(QLineEdit::Password);
   layout->addRow(tr("&Password: "), wPassword);

   QDialogButtonBox *bbox = new QDialogButtonBox();
   layout->addRow(bbox);

   wConnect = new BusyButton("&Connect", this);
   wConnect->setDefault(true);
   connect(wConnect, SIGNAL(clicked()), this, SLOT(authenticate()));
   bbox->addButton(wConnect, QDialogButtonBox::ActionRole);

   setWindowTitle(tr("Connect to scheduler"));
   setFixedWidth(sizeHint().width() * 1.4);
   setModal(true);
   setLayout(layout);
}

void LoginDialog::authenticate()
{
   SSHCredentials *cred = new SSHCredentials();
   connect(cred, SIGNAL(authorized(SSHCredentials *)),
           this, SLOT(authorized(SSHCredentials *)));
   connect(cred, SIGNAL(denied(QString)),
           this, SLOT(denied(QString)));
   cred->Authenticate(wHost->text(), wUsername->text(), wPassword->text());
   wConnect->SetBusyState("Connecting");
}

void LoginDialog::authorized(SSHCredentials *credentials)
{
   hide();
   emit accept(credentials);
}

void LoginDialog::denied(QString error)
{
   QErrorMessage msg(this);
   msg.showMessage(error);
   msg.exec();
   wConnect->StopBusyState();
}

void LoginDialog::setHost(QString const& host)
{
   wHost->setText(host);
}

void LoginDialog::setUsername(QString const& username)
{
   wUsername->setText(username);
}

void LoginDialog::setPassword(QString const& password)
{
   wPassword->setText(password);
}


