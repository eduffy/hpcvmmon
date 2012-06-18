
#ifndef JOB_DEFINITION_H
#define JOB_DEFINITION_H

#include <QMetaType>
#include <QString>
#include <QMap>

struct JobDefinition {
   JobDefinition(QString const& id);
   void update(QMap<QString, QString> const& jobSpec);

   enum Status { QUEUED, HELD, RUNNING, FINISHED, };
   static QString StatusStr[];

   QString  pbsid;
   QString  name;
   int      ncpus;
   int      jobNumber;
   QString  host;
   int      port;
   Status   status;
   QString  walltime;
   int      cpupercent;
   double   memMb;
};
Q_DECLARE_METATYPE(JobDefinition *);


QDebug operator<<(QDebug debug, const JobDefinition &job);

#endif /* JOB_DEFINITION_H */
