
#ifndef SESSION_LIST_WIDGET_H
#define SESSION_LIST_WIDGET_H

#include <QTreeWidget>
#include <QList>

struct JobDefinition;
class SSHCredentials;
class QContextMenuEvent;

class SessionListWidget : public QTreeWidget
{
   Q_OBJECT

public:
   SessionListWidget(QWidget *parent = NULL);
   virtual ~SessionListWidget();

   void Update(SSHCredentials *sshCredentials, bool showFinished);

protected slots:
   void parseQstat(QString const& qstat);
   void queueUpdated(QString const& qstat);
   void commandFailed(QString const& error);
   void doubleClickSession();

signals:
   void updated();
   void viewSession(JobDefinition *job);
   void removeSession(JobDefinition *job);
   void resumeSession(JobDefinition *job);

protected:
   void contextMenuEvent(QContextMenuEvent *event);

private:
   QList<JobDefinition>  sessions;
};

#endif  /* SESSION_LIST_WIDGET_H */
