
#include <QGridLayout>
#include <QTextEdit>
#include <QLineEdit>
#include <QSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QString>
#include <QStringList>
#include <QRegExp>
#include <QtDebug>
#include "AddSessionDialog.moc"


AddSessionDialog::AddSessionDialog(QWidget *parent)
   : QDialog(parent)
{
   QLabel       *wLabel;
   QGridLayout  *wGrid;
   QDialogButtonBox *dlgBox;
   QPushButton  *wAddSession;
   QPushButton  *wCancel;

   wGrid = new QGridLayout();

   wLabel = new QLabel("VM Image:");
   wGrid->addWidget(wLabel, 0, 0);

   wImagePath = new QLineEdit();
   wGrid->addWidget(wImagePath, 0, 1, 1, 3);

   wLabel = new QLabel("No. CPUs:");
   wGrid->addWidget(wLabel, 1, 0);

   wNumCPUs = new QSpinBox();
   wNumCPUs->setRange(1, 24);
   wNumCPUs->setSingleStep(1);
   wNumCPUs->setAlignment(Qt::AlignRight);
   wGrid->addWidget(wNumCPUs, 1, 1);

   wLabel = new QLabel("Memory:");
   wGrid->addWidget(wLabel, 1, 2);

   wMemory = new QSpinBox();
   wMemory->setRange(2048, 1048576);
   wMemory->setSingleStep(128);
   wMemory->setSuffix("mb");
   wMemory->setAlignment(Qt::AlignRight);
   wGrid->addWidget(wMemory, 1, 3);

   wLabel = new QLabel("Job Name:");
   wGrid->addWidget(wLabel, 2, 0);

   wJobName = new QLineEdit();
   wGrid->addWidget(wJobName, 2, 1);

   wLabel = new QLabel("Job Range:");
   wGrid->addWidget(wLabel, 2, 2);

   wJobRange = new QLineEdit();
   wGrid->addWidget(wJobRange, 2, 3);

   wLabel = new QLabel("Job Script:");
   wGrid->addWidget(wLabel, 3, 0);

   wStartupScript = new QTextEdit();
   wStartupScript->setCurrentFont(QFont("Courier"));
   wStartupScript->setLineWrapMode(QTextEdit::NoWrap);
   wGrid->addWidget(wStartupScript, 4, 0, 1, 4);

   dlgBox = new QDialogButtonBox();
   wGrid->addWidget(dlgBox, 5, 0, 1, 4);
   wAddSession = new QPushButton("&Add");
   dlgBox->addButton(wAddSession, QDialogButtonBox::AcceptRole);
   connect(wAddSession, SIGNAL(clicked()), this, SLOT(submitJobs()));

   wCancel = new QPushButton("&Cancel");
   dlgBox->addButton(wCancel, QDialogButtonBox::RejectRole);

   setLayout(wGrid);
   setWindowTitle("Add session");
   resize(580, 410);
}

AddSessionDialog::~AddSessionDialog()
{
}

void AddSessionDialog::submitJobs()
{
   QSet<int> jobIds;
   foreach(const QString &r, wJobRange->text().split(',')) {
      QStringList range = r.split('-');
      if(range.size() == 2) {
         int low = range.front().toInt();
         int high = range.back().toInt();
         for(int jobId = low; jobId <= high; jobId++) {
            jobIds.insert(jobId);
         }
      }
      else if(range.size() == 1) {
         jobIds.insert(range.front().toInt());
      }
   }

   emit jobs_submitted(jobIds,
                       wImagePath->text(),
                       wNumCPUs->text().toInt(),
                       wMemory->text(),
                       wJobName->text(),
                       wStartupScript->toPlainText());
   accept();
}
