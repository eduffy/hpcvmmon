
#include <QTemporaryFile>
#include <QTextStream>
#include <QDateTime>
#include <QtDebug>
#include "SSHCredentials.h"
#include "SSHCommand.h"
#include "SSHUploadFile.h"
#include "SubmitJobThread.moc"

SubmitJobThread::SubmitJobThread(SSHCredentials *credentials)
   : sshCredentials(credentials)
{
}

SubmitJobThread::~SubmitJobThread()
{
}
   
void SubmitJobThread::submitJobs(QSet<int> const& _jobNums,
                                 QString const& _imagePath,
                                 int _numCpus,
                                 QString const& _memory,
                                 QString const& _jobName,
                                 QString const& _startupScript)
{
   jobNums       = _jobNums;
   imagePath     = _imagePath;
   numCpus       = _numCpus;
   memory        = _memory;
   jobName       = _jobName;
   startupScript = _startupScript;

   QThread::start();
}

void SubmitJobThread::run()
{
   QTemporaryFile f;
   f.setAutoRemove(false);
   if(f.open()) {
      QFile submitFile(":/QemuSubmitTemplate.pbs");
      if(!submitFile.open(QIODevice::ReadOnly | QIODevice::Text))
         return;
      QString qsubSubmitTempl = submitFile.readAll();
      qDebug() << qsubSubmitTempl;

      memory.chop(1);  // removes the `b` .. qemu wants 2g not 2gb
      QTextStream strm(&f);
      qDebug() << f.fileName();
      strm << qsubSubmitTempl.arg(numCpus).arg(memory).arg(jobName) << endl
           << "@" << endl
           << startupScript << endl;
      f.close();

      qint64 clock = QDateTime::currentDateTime().toTime_t();
      QString remoteJobFile(QString("/tmp/%1_%2_%3.pbs")
         .arg(sshCredentials->getUsername())
         .arg(jobName)
         .arg(clock));

      SSHUploadFile uf(sshCredentials);
      // we're already in a thread, so use the blocking versions of SSHUploadFile and SSHCommand
      if(uf.TransferNow(f.fileName(), remoteJobFile)) {
         SSHCommand ssh(sshCredentials);
         foreach(int jobnum, jobNums) {
            QString cmd(QString("qsub -v QEMU_VNC_JOB_NUMBER=%1 %2").arg(jobnum).arg(remoteJobFile));
            ssh.ExecuteNow(cmd);
         }
      }
   }
   emit complete();
}
