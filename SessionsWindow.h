
#ifndef SESSIONS_WINDOW_H
#define SESSIONS_WINDOW_H

#include <QApplication>
#include <QMainWindow>
#include <QMetaType>
#include <QString>
#include <QList>
#include <QMap>
#include <QSet>

class QTreeWidget;
class QCheckBox;
class QPushButton;
class QTimerEvent;
class LoginDialog;
class AddSessionDialog;
class SubmitJobThread;
class SSHCredentials;
class SSHCommand;
class BusyButton;

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


class SessionsWindow : public QMainWindow
{
   Q_OBJECT
public:
   SessionsWindow();
   virtual ~SessionsWindow();

   virtual void show();
   void RefreshSessionDisplay();

public slots:
   void authorized(SSHCredentials *credentials);
   void queueUpdated(QString);
   void commandFailed(QString);
   void addSession();
   void updateSessionList();
   void doubleClickSession();
   void toggleShowFinished(int state);
   void vncSessionClosed(int screen);
   void submitJobs(QSet<int> const& jobNums,
                   QString const& imagePath,
                   int numCpus,
                   QString const& memory,
                   QString const& jobName,
                   QString const& startupScript);
   void submitComplete();

protected:
   virtual void timerEvent(QTimerEvent *);

private:
   SSHCredentials   *sshCredentials;
   AddSessionDialog *sessionDlg;
   SubmitJobThread  *jobThread;

   QTreeWidget      *wSessionList;
   QCheckBox        *wShowFinished;
   BusyButton       *wAddSession;
   BusyButton       *wRefresh;

   QList<JobDefinition> sessions;
   QSet<int>            availScreens;
};

#endif /* SESSIONS_WINDOW_H */
