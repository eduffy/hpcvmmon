
#include <QAction>
#include <QTreeWidget>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QString>
#include <QStringList>
#include <QRegExp>
#include <QErrorMessage>
#include <QtDebug>
#include "JobDefinition.h"
#include "LoginDialog.h"
#include "SubmitJobThread.h"
#include "SSHCredentials.h"
#include "SSHCommand.h"
#include "AddSessionDialog.h"
#include "VNCWindow.h"
#include "BusyButton.h"
#include "SessionListWidget.h"
#include "SessionsWindow.moc"

SessionsWindow::SessionsWindow()
   : QMainWindow()
{
   QVBoxLayout *layout = new QVBoxLayout();
   QHBoxLayout *btmbar = new QHBoxLayout();

   wSessionList = new SessionListWidget();
   connect(wSessionList, SIGNAL(updated()),
           this,         SLOT(sessionsUpdated()));
   connect(wSessionList, SIGNAL(viewSession(JobDefinition *)),
           this,         SLOT(viewSession(JobDefinition *)));
   connect(wSessionList, SIGNAL(removeSession(JobDefinition *)),
           this,         SLOT(removeSession(JobDefinition *)));
   connect(wSessionList, SIGNAL(resumeSession(JobDefinition *)),
           this,         SLOT(resumeSession(JobDefinition *)));

   QAction *viewAction = new QAction("View session", this);
   QFont f = viewAction->font();
   f.setBold(true);
   viewAction->setFont(f);
   wSessionList->addAction(viewAction);

   QAction *deleteAction = new QAction("Delete session", this);
   wSessionList->addAction(deleteAction);

   wShowFinished = new QCheckBox("Show &completed jobs");
   wShowFinished->setDisabled(true);
   connect(wShowFinished, SIGNAL(stateChanged(int)), this, SLOT(toggleShowFinished(int)));

   wAddSession  = new BusyButton("&Add Sessions");
   wRefresh     = new BusyButton("&Refresh");

   wAddSession->setDisabled(true);
   connect(wAddSession, SIGNAL(clicked()), this, SLOT(addSession()));

   wRefresh->setDisabled(true);
   connect(wRefresh, SIGNAL(clicked()), this, SLOT(updateSessionList()));

   layout->addWidget(wSessionList);
   layout->addWidget(wShowFinished);
   btmbar->addWidget(wRefresh, 1, Qt::AlignRight);
   btmbar->addWidget(wAddSession, 0, Qt::AlignRight);
   layout->addLayout(btmbar);

   setCentralWidget(new QWidget);
   centralWidget()->setLayout(layout);

   setWindowTitle("VM Manager");
   resize(640, 280);
}

SessionsWindow::~SessionsWindow()
{
}

void SessionsWindow::show()
{
   QMainWindow::show();
   LoginDialog *loginDlg = new LoginDialog(NULL);
   loginDlg->setHost("user.palmetto.clemson.edu");
   loginDlg->setUsername(getenv("USER"));
   connect(loginDlg, SIGNAL(accept(SSHCredentials *)),
           this,     SLOT(authorized(SSHCredentials *)));
   // connect(loginDlg, SIGNAL(reject()), this, SLOT(authenticate()));
   loginDlg->show();
}


void SessionsWindow::viewSession(JobDefinition *job)
{
   qDebug() << job->pbsid;
  
   if(job->status == JobDefinition::RUNNING) {
      int screen = 1;
      while(availScreens.contains(screen)) {
         ++screen;
      }
      availScreens.insert(screen);
      VNCWindow *w = new VNCWindow(sshCredentials,
                                   screen,
                                   job->host,
                                   job->port);
      connect(w,    SIGNAL(closed(int)),
              this, SLOT(vncSessionClosed(int)));
      w->show();
   }
}

void SessionsWindow::removeSession(JobDefinition *job)
{
   qDebug() << "remove " << job;
}

void SessionsWindow::resumeSession(JobDefinition *job)
{
   qDebug() << "resume " << job;
}

void SessionsWindow::authorized(SSHCredentials *_sshCredentials)
{
   sshCredentials = _sshCredentials;
   wSessionList->Update(sshCredentials, wShowFinished->checkState() == Qt::Checked);

   wShowFinished->setDisabled(false);
   wAddSession->setDisabled(false);
   startTimer(60000);  // refresh every 60 seconds
}

void SessionsWindow::updateSessionList()
{
   wRefresh->SetBusyState("Refreshing");
   wSessionList->Update(sshCredentials, wShowFinished->checkState() == Qt::Checked);
}

void SessionsWindow::sessionsUpdated()
{
   wRefresh->StopBusyState();
}

void SessionsWindow::addSession()
{
   sessionDlg = new AddSessionDialog(NULL);
   connect(sessionDlg,
           SIGNAL(jobs_submitted(QSet<int> const&,
                                 QString const&,
                                 int,
                                 QString const&,
                                 QString const&,
                                 QString const&)),
           this,
           SLOT(submitJobs(QSet<int> const&,
                           QString const&,
                           int,
                           QString const&,
                           QString const&,
                           QString const&)));

   sessionDlg->show();
}

void SessionsWindow::timerEvent(QTimerEvent *)
{
   updateSessionList();
}


void SessionsWindow::toggleShowFinished(int state)
{
   updateSessionList();
}

void SessionsWindow::vncSessionClosed(int screen)
{
   availScreens.remove(screen);
}

void SessionsWindow::submitJobs(QSet<int> const& jobNums,
                                QString const& imagePath,
                                int numCpus,
                                QString const& memory,
                                QString const& jobName,
                                QString const& startupScript)
{
   jobThread = new SubmitJobThread(sshCredentials);
   connect(jobThread, SIGNAL(complete()),
           this,      SLOT(submitComplete()));
   jobThread->submitJobs(jobNums,
                         imagePath,
                         numCpus,
                         memory,
                         jobName,
                         startupScript);
   wAddSession->SetBusyState("Submitting");
}

void SessionsWindow::submitComplete()
{
   wAddSession->StopBusyState();
   updateSessionList();
}

