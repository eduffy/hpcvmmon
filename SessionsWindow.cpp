
#include <QTreeWidget>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QString>
#include <QStringList>
#include <QRegExp>
#include <QErrorMessage>
#include <QtDebug>
#include "LoginDialog.h"
#include "SubmitJobThread.h"
#include "SSHCredentials.h"
#include "SSHCommand.h"
#include "AddSessionDialog.h"
#include "VNCWindow.h"
#include "BusyButton.h"
#include "SessionsWindow.moc"

JobDefinition::JobDefinition(QString const& id)
   : pbsid(id)
   , name("")
   , ncpus(0)
   , jobNumber(0)
   , host("")
   , port(0)
   , status(QUEUED)
   , walltime("00:00:00")
   , cpupercent(0)
   , memMb(0.0)
{ }

QString JobDefinition::StatusStr[] = {
   QString("Queued"),
   QString("Held"),
   QString("Running"),
   QString("Finished"),
};

void JobDefinition::update(QMap<QString, QString> const& jobSpec)
{
   name = jobSpec["Job_Name"];
   if(jobSpec["job_state"] == "H") {
      status = HELD;
   } else if(jobSpec["job_state"] == "R") {
      status = RUNNING;
   } else if(jobSpec["job_state"] == "F") {
      status = FINISHED;
   }
   if(status != QUEUED) {
      QString execHost = jobSpec["exec_host"];
      QRegExp rxHost("[a-z0-9]+/");
      rxHost.indexIn(execHost);
      host = rxHost.cap();
      host.chop(1);

      QRegExp rxCore("/[0-9]+");
      rxCore.indexIn(execHost);
      port = 5901 + rxCore.cap().mid(1).toInt();

      ncpus = 1;
      QRegExp rxNCpus("\\*[0-9]+");
      rxNCpus.indexIn(execHost);
      if(!rxNCpus.cap().isEmpty()) {
         ncpus = rxNCpus.cap().mid(1).toInt();
      }

      /* A job may be running according to the queue, but still executing 
       * the prologue, so the `resources_used` attributes are not yet set
       */
      if(jobSpec.contains("resources_used.walltime")) {
         QStringList sl = jobSpec["resources_used.walltime"].split(":");
         walltime = QString("%1h %2m").arg(sl[0]).arg(sl[1]);
      }
      if(jobSpec.contains("resources_used.cpupercent")) {
         cpupercent = jobSpec["resources_used.cpupercent"].toInt();
      }
      if(jobSpec.contains("resources_used.mem")) {
         QString memstr = jobSpec["resources_used.mem"];
         memstr.chop(2);  /* remove the kb suffix */
         memMb = memstr.toInt() / 1024.;
      }
   }
   jobNumber = -1;
   QRegExp rxJobNum("QEMU_VNC_JOB_NUMBER=([0-9]+)");
   rxJobNum.indexIn(jobSpec["Variable_List"]);
   if(!rxJobNum.cap(1).isEmpty()) {
      jobNumber = rxJobNum.cap(1).toInt();
   }
}

QDebug operator<<(QDebug debug, const JobDefinition &job)
{
   debug << "\n\n" << job.pbsid;
   debug << "\n\tname = " << job.name;
   debug << "\n\tncpus = " << job.ncpus;
   debug << "\n\tjob no. = " << job.jobNumber;
   debug << "\n\thost = " << job.host;
   debug << "\n\tport = " << job.port;
   debug << "\n\tstatus =" << JobDefinition::StatusStr[job.status];
   debug << "\n\twalltime = " << job.walltime;
   debug << "\n\tcpu = " << job.cpupercent << "%";
   debug << "\n\tmem = " << job.memMb << "mb";
   return debug;
}


SessionsWindow::SessionsWindow()
   : QMainWindow()
{
   QVBoxLayout *layout = new QVBoxLayout();
   QHBoxLayout *btmbar = new QHBoxLayout();

   wSessionList = new QTreeWidget();
   QStringList headerLabels;
   headerLabels << "Job Name"
                << "Job No."  // QString::fromUtf8("Job \u2116")
                << "Status"
                << "Run Time"
                << "%CPU"
                << "Memory";
   wSessionList->setHeaderLabels(headerLabels);
   connect(wSessionList, SIGNAL(doubleClicked(const QModelIndex &)), this, SLOT(doubleClickSession()));

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

void SessionsWindow::authorized(SSHCredentials *credentials)
{
   sshCredentials = credentials;
   updateSessionList();

   wShowFinished->setDisabled(false);
   wAddSession->setDisabled(false);
   startTimer(60000);  // refresh every 60 seconds
}

void SessionsWindow::updateSessionList()
{
   wRefresh->SetBusyState("Refreshing");
   SSHCommand *sshCmd = new SSHCommand(sshCredentials);
   connect(sshCmd, SIGNAL(completed(QString)), this, SLOT(queueUpdated(QString)));
   connect(sshCmd, SIGNAL(failed(QString)),    this, SLOT(commandFailed(QString)));

   QString cmd(QString("qselect -u %1 -x | xargs --no-run-if-empty qstat -fx")
                 .arg(sshCredentials->getUsername()));
   sshCmd->Execute(cmd);
}

void SessionsWindow::queueUpdated(QString qstat)
{
   sessions.clear();

   QStringList lines = qstat.split('\n');

   JobDefinition *job = NULL;
   QMap<QString, QString> jobSpec, jobVars;

   QRegExp rxJobId("[0-9]+\\.pbs01");
   QRegExp rxKey("[A-Za-z_\\.]+");
   QString key, value;
   for(QStringList::const_iterator line = lines.begin();
       line != lines.end();
       line++)
   {
      if(line->isEmpty()) { /* empty */ }
      else if(line->startsWith("Job Id:")) {
         if(job != NULL) {
            job->update(jobSpec);
            if(job->jobNumber > -1 && (wShowFinished->checkState() == Qt::Checked || job->status != JobDefinition::FINISHED)) {
               sessions.append(*job);
            }
            delete job;
            job = NULL;
            jobSpec.clear();
            jobVars.clear();
         }
         rxJobId.indexIn(*line);
         job = new JobDefinition(rxJobId.cap());
      }
      else if(line->startsWith("    ")) {  /* keys start with 4 spaces */
         if(key == "Variable_List") {
            QStringList vars = jobSpec[key].split(",");
            for(QStringList::const_iterator i = vars.begin();
                i != vars.end(); 
                i++)
            {
               int eq = i->indexOf('=');
               jobVars.insert(i->left(eq), i->mid(eq + 1));
            }
         }
         rxKey.indexIn(*line);
         key = rxKey.cap(0);
         value = line->mid(line->indexOf('=') + 2);
         jobSpec.insert(key, value);
      }
      else if(line->at(0) == '\t') {
         /* append to the previous key */
         jobSpec[key].append(line->mid(1));
      }
   }
   if(job) {
      job->update(jobSpec);
      if(job->jobNumber > -1 && (wShowFinished->checkState() == Qt::Checked || job->status != JobDefinition::FINISHED)) {
         sessions.append(*job);
      }
   }

   qDebug() << sessions;
   RefreshSessionDisplay();
}

void SessionsWindow::commandFailed(QString error)
{
   QErrorMessage msg;
   msg.showMessage(error);
   msg.exec();
}

void SessionsWindow::RefreshSessionDisplay()
{
   wSessionList->clear();
   QList<JobDefinition>::iterator p;
   for(p = sessions.begin(); p != sessions.end(); ++p) {
      QTreeWidgetItem *item = new QTreeWidgetItem(wSessionList);
      item->setText(0, p->name);
      item->setText(1, QString("%1").arg(p->jobNumber));
      item->setText(2, JobDefinition::StatusStr[p->status]);
      item->setText(3, p->walltime);
      item->setText(4, QString("%1").arg(p->cpupercent));
      item->setText(5, QString("%1mb").arg(p->memMb));
      item->setData(0, Qt::UserRole, qVariantFromValue(&(*p)));
   }
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

void SessionsWindow::doubleClickSession()
{
   QTreeWidgetItem *item = wSessionList->currentItem();
   const QVariant &itemData = item->data(0, Qt::UserRole);
   JobDefinition *job = itemData.value<JobDefinition *>();
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
