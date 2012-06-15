
#include <QTemporaryFile>
#include <QTextStream>
#include <QDateTime>
#include <QtDebug>
#include "SSHCredentials.h"
#include "SSHCommand.h"
#include "SSHUploadFile.h"
#include "SubmitJobThread.moc"

static QString qsubSubmitTempl =
"#!/bin/bash -x\n"
"\n"
"#PBS -l select=1:ncpus=%1:mem=%2b,walltime=24:00:00\n"
"#PBS -N %3\n"
"#PBS -j oe\n"
"workdir=/local_scratch/${USER}_${PBS_JOBID}\n"
"mkdir -p ${workdir}\n"
"echo \"SET JOB_NUMBER=${QEMU_VNC_JOB_NUMBER}\" > ${workdir}/RUNSCRIPT.BAT\n"
"for i in $(seq 2 9); do\n"
"  echo \"SET JOB_NUMBER_Z${i}=$(printf %'0'${i}'d' ${QEMU_VNC_JOB_NUMBER})\" >> ${workdir}/RUNSCRIPT.BAT\n"
"done\n"
"echo \"SET JOB_HOST=$(hostname)\" >> ${workdir}/RUNSCRIPT.BAT\n"
"sed -e '1,/^@/d' $0 >> ${workdir}/RUNSCRIPT.BAT\n"
"sed -e 's/$/\\r/' -i ${workdir}/RUNSCRIPT.BAT\n"
"PATH=\"/home/joehsen/qemu-kvm-0.14.1/x86_64-softmmu:${PATH}\"\n"
"export TMPDIR=/local_scratch\n"
"VNC_DISPLAY=$((1+$(qstat -n ${PBS_JOBID} | egrep -o 'node..../[0-9]+' | awk -F/ '{print $2}')))\n"
"\n"
"qemu-system-x86_64 \\\n"
"   -localtime -hda /scratch/eduffy/win7.img \\\n"
"   -m %2 -smp %1,cores=%1,sockets=1 \\\n"
"   -usb -usbdevice tablet \\\n"
"   -net nic -net user \\\n"
"   -snapshot -k en-us -vnc :${VNC_DISPLAY} \\\n"
"   -fsdev local,id=scratch,path=/scratch/eduffy,security_model=passthrough \\\n"
"   -tftp ${workdir}\n"
"exit $?\n";

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
