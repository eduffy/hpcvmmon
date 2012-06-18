
#ifndef SESSIONS_WINDOW_H
#define SESSIONS_WINDOW_H

#include <QApplication>
#include <QMainWindow>
#include <QString>
#include <QList>
#include <QMap>
#include <QSet>


class QCheckBox;
class QPushButton;
class QTimerEvent;
struct JobDefinition;
class SessionListWidget;
class LoginDialog;
class AddSessionDialog;
class SubmitJobThread;
class SSHCredentials;
class SSHCommand;
class BusyButton;


class SessionsWindow : public QMainWindow
{
   Q_OBJECT
public:
   SessionsWindow();
   virtual ~SessionsWindow();

   virtual void show();

public slots:
   void authorized(SSHCredentials *credentials);
   void commandFailed(QString const& error);
   void updateSessionList();
   void toggleShowFinished(int state);
   void vncSessionClosed(int screen);

   void addSession();
   void submitJobs(QSet<int> const& jobNums,
                   QString const& imagePath,
                   int numCpus,
                   QString const& memory,
                   QString const& jobName,
                   QString const& startupScript);
   void submitComplete();

   // slots from the session list
   void sessionsUpdated();
   void viewSession(JobDefinition *job);
   void removeSession(JobDefinition *job);
   void resumeSession(JobDefinition *job);
   void releaseSession(JobDefinition *job);


protected:
   virtual void timerEvent(QTimerEvent *);

private:
   SSHCredentials     *sshCredentials;
   AddSessionDialog   *sessionDlg;
   SubmitJobThread    *jobThread;

   SessionListWidget  *wSessionList;
   QCheckBox          *wShowFinished;
   BusyButton         *wAddSession;
   BusyButton         *wRefresh;

   QSet<int>           availScreens;
};

#endif /* SESSIONS_WINDOW_H */
