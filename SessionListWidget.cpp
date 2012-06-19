
#include <QMenu>
#include <QFont>
#include <QContextMenuEvent>
#include <QErrorMessage>
#include <QtDebug>
#include "JobDefinition.h"
#include "SSHCredentials.h"
#include "SSHCommand.h"
#include "SessionListWidget.moc"

// void updated();
// void viewSession(QString const& pbsId);
// void removeSession(QString const& pbsId);
// void resumeSession(QString const& pbsId);

SessionListWidget::SessionListWidget(QWidget *parent)
   : QTreeWidget(parent)
{
   QStringList headerLabels;
   headerLabels << "Job Name"
                << "Job No."  // QString::fromUtf8("Job \u2116")
                << "Status"
                << "Run Time"
                << "%CPU"
                << "Memory";
   setHeaderLabels(headerLabels);


   connect(this, SIGNAL(doubleClicked(const QModelIndex &)), this, SLOT(doubleClickSession()));

}

SessionListWidget::~SessionListWidget()
{
}


void SessionListWidget::Update(SSHCredentials *sshCredentials, bool showFinished)
{
   QString cmd;
   SSHCommand *sshCmd = new SSHCommand(sshCredentials);
   connect(sshCmd, SIGNAL(completed(QString)), this, SLOT(queueUpdated(QString)));
   connect(sshCmd, SIGNAL(failed(QString)),    this, SLOT(commandFailed(QString)));

   if(showFinished) {
      cmd = QString("qselect -u %1 -x | xargs --no-run-if-empty qstat -fx");
   }
   else {
      cmd = QString("qselect -u %1 | xargs --no-run-if-empty qstat -f");
   }
   cmd = cmd.arg(sshCredentials->getUsername());
   sshCmd->Execute(cmd);
}

void SessionListWidget::parseQstat(QString const& qstat)
{
   QStringList lines = qstat.split('\n');

   JobDefinition *job = NULL;
   QMap<QString, QString> jobSpec, jobVars;

   QRegExp rxJobId("[0-9]+\\.pbs01");
   QRegExp rxKey("[A-Za-z_\\.]+");
   QString key(""), value("");
   for(QStringList::const_iterator line = lines.begin();
       line != lines.end();
       line++)
   {
      if(line->isEmpty()) { /* empty */ }
      else if(line->startsWith("Job Id:")) {
         if(job != NULL) {
            job->update(jobSpec);
            sessions.append(*job);
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
            QStringList vars(jobSpec[key].split(","));
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
      sessions.append(*job);
   }

   qDebug() << sessions;
}

void SessionListWidget::queueUpdated(QString const& qstat)
{
   sessions.clear();
   parseQstat(qstat);
   
   clear();
   QList<JobDefinition>::iterator p;
   for(p = sessions.begin(); p != sessions.end(); ++p) {
      QTreeWidgetItem *item = new QTreeWidgetItem(this);
      item->setText(0, p->name);
      item->setText(1, QString("%1").arg(p->jobNumber));
      item->setText(2, JobDefinition::StatusStr[p->status]);
      item->setText(3, p->walltime);
      item->setText(4, QString("%1").arg(p->cpupercent));
      item->setText(5, QString("%1mb").arg(p->memMb));
      item->setData(0, Qt::UserRole, qVariantFromValue(&(*p)));
   }

   emit updated();
}

void SessionListWidget::commandFailed(QString const& error)
{
   QErrorMessage msg;
   msg.showMessage(error);
   msg.exec();
}

void SessionListWidget::doubleClickSession()
{
   const QVariant &itemData = currentItem()->data(0, Qt::UserRole);
   JobDefinition *job = itemData.value<JobDefinition *>();
   qDebug() << *job;
   if(job->status == JobDefinition::RUNNING) {
      emit viewSession(job);
   }
}

void SessionListWidget::contextMenuEvent(QContextMenuEvent *event)
{
   QModelIndex index = indexAt(event->pos());
   if (index.isValid()) {
      JobDefinition &job = sessions[index.row()];
      qDebug() << job;

      QMenu *menu = new QMenu(this);
      QAction *action;
      switch(job.status) {
         case JobDefinition::QUEUED: {
            action = new QAction("Delete session", this);
            action->setData(qVariantFromValue(&job));
            connect(action, SIGNAL(triggered()), this, SLOT(removeSessionSlot()));
            menu->addAction(action);

            break;
         }

         case JobDefinition::HELD: {
            action = new QAction("Resume session", this);
            action->setData(qVariantFromValue(&job));
            connect(action, SIGNAL(triggered()), this, SLOT(releaseSessionSlot()));
            menu->addAction(action);

            action = new QAction("Delete session", this);
            action->setData(qVariantFromValue(&job));
            connect(action, SIGNAL(triggered()), this, SLOT(removeSessionSlot()));
            menu->addAction(action);

            break;
         }

         case JobDefinition::RUNNING: {
            action = new QAction("View session", this);
            QFont f = action->font();
            f.setBold(true);
            action->setFont(f);
            action->setData(qVariantFromValue(&job));
            connect(action, SIGNAL(triggered()), this, SLOT(viewSessionSlot()));
            menu->addAction(action);

            action = new QAction("Delete session", this);
            action->setData(qVariantFromValue(&job));
            connect(action, SIGNAL(triggered()), this, SLOT(removeSessionSlot()));
            menu->addAction(action);

            break;
         }

         case JobDefinition::FINISHED: {
            menu->addAction("Resume session [TODO]");
            break;
         }
      }

      menu->exec(QCursor::pos());
   }
}

void SessionListWidget::viewSessionSlot()
{
   QAction *action = qobject_cast<QAction *>(sender());
   JobDefinition *job = action->data().value<JobDefinition *>();
   emit viewSession(job);
}

void SessionListWidget::removeSessionSlot()
{
   QAction *action = qobject_cast<QAction *>(sender());
   JobDefinition *job = action->data().value<JobDefinition *>();
   emit removeSession(job);
}

void SessionListWidget::resumeSessionSlot()
{
   QAction *action = qobject_cast<QAction *>(sender());
   JobDefinition *job = action->data().value<JobDefinition *>();
   emit resumeSession(job);
}

void SessionListWidget::releaseSessionSlot()
{
   QAction *action = qobject_cast<QAction *>(sender());
   JobDefinition *job = action->data().value<JobDefinition *>();
   emit releaseSession(job);
}

