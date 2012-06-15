
#ifndef SUBMIT_JOB_THREAD_H
#define SUBMIT_JOB_THREAD_H

#include <QThread>
#include <QString>
#include <QSet>

class SSHCredentials;

class SubmitJobThread : public QThread
{
   Q_OBJECT
public:
   SubmitJobThread(SSHCredentials *credentials);
   virtual ~SubmitJobThread();
   
   void submitJobs(QSet<int> const& jobNums,
                   QString const& imagePath,
                   int numCpus,
                   QString const& memory,
                   QString const& jobName,
                   QString const& startupScript);

signals:
   void complete();

protected:
   virtual void run();

private:
   SSHCredentials *sshCredentials;

   QSet<int>  jobNums;
   QString    imagePath;
   int        numCpus;
   QString    memory;
   QString    jobName;
   QString    startupScript;
};

#endif /* SUBMIT_JOB_THREAD_H */
