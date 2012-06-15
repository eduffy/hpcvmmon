
#ifndef ADD_SESSION_WINDOW_H
#define ADD_SESSION_WINDOW_H

#include <QDialog>

class QLineEdit;
class QTextEdit;
class QSpinBox;
class SSHCommand;

class AddSessionDialog : public QDialog
{
   Q_OBJECT
public:
   AddSessionDialog(QWidget *parent);
   virtual ~AddSessionDialog();

public slots:
   void submitJobs();

signals:
   void jobs_submitted(QSet<int> const& jobNums,
                       QString const& imagePath,
                       int numCpus,
                       QString const& memory,
                       QString const& jobName,
                       QString const& startupScript);

private:
   QLineEdit  *wImagePath;
   QSpinBox   *wNumCPUs;
   QSpinBox   *wMemory;
   QLineEdit  *wJobName;
   QLineEdit  *wJobRange;
   QTextEdit  *wStartupScript;

   SSHCommand *ssh;
};

#endif /* ADD_SESSION_WINDOW_H */
