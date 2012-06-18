
#include <QStringList>
#include <QRegExp>
#include <QtDebug>
#include "JobDefinition.h"

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

