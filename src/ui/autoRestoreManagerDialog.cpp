#include "autoRestoreManagerDialog.h"
#include "ui_autoRestoreManagerDialog.h"

#include "geometry/document.h"

#include <QDateTimeEdit>
#include <QFileInfo>
#include <QGroupBox>
#include <QProcess>
#include <QPushButton>

using namespace geometry;

AutoRestoreManagerDialog::AutoRestoreManagerDialog(QStringList names, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AutoRestoreManagerDialog)
    , fileNames_(names)
{
    ui->setupUi(this);
    // parent->hide();

    updateContents();
}

AutoRestoreManagerDialog::~AutoRestoreManagerDialog()
{
    delete ui;
}

void AutoRestoreManagerDialog::updateContents()
{
    foreach(QString tempName, fileNames_)
    {
        tempName = tempName.trimmed();
        if(tempName.isEmpty())
            continue;

        QString origName = Document::preLoadDocumentOriginalName(tempName);
        ARItem  orig{origName, QFileInfo(origName).lastModified()};
        ARItem  temp{tempName, QFileInfo(tempName).lastModified()};

        insertPanel(orig, temp);
    }
}

void AutoRestoreManagerDialog::insertPanel(ARItem orig, ARItem temp)
{
    auto verticalLayout = dynamic_cast<QVBoxLayout *>(layout());
    if(!verticalLayout)
        return;

    auto groupBox   = new QGroupBox(this);
    auto gridLayout = new QGridLayout(groupBox);
    gridLayout->setColumnStretch(1, 1);

    auto label_1 = new QLabel("Местоположение", groupBox);
    gridLayout->addWidget(label_1, 0, 0, 1, 1);

    bool noOrigFile = orig.fileName.isEmpty();
    auto fileName   = !noOrigFile ? orig.fileName : "(файл не был сохранён)";
    auto label_2    = new QLabel(fileName, groupBox);
    gridLayout->addWidget(label_2, 0, 1, 1, 2);

    auto label_3 = new QLabel("Автосохранённая копия", groupBox);
    gridLayout->addWidget(label_3, 1, 0, 1, 1);

    auto label_4 = new QLabel("Последнее сохранение", groupBox);
    gridLayout->addWidget(label_4, 2, 0, 1, 1);

    // autosaved

    auto openTempButton = new QPushButton("Открыть", groupBox);
    gridLayout->addWidget(openTempButton, 1, 2, 1, 1);
    connect(openTempButton, &QPushButton::pressed, [temp]() {
        startAutoRestoredFile(temp.fileName, 1);
    });

    auto tempFileDateTime = new QDateTimeEdit(groupBox);
    tempFileDateTime->setDateTime(temp.dateTime);
    gridLayout->addWidget(tempFileDateTime, 1, 1, 1, 1);

    // original

    auto openOrigButton = new QPushButton("Открыть", groupBox);
    if(noOrigFile)
        openOrigButton->setEnabled(false);

    gridLayout->addWidget(openOrigButton, 2, 2, 1, 1);
    connect(openOrigButton, &QPushButton::pressed, [orig]() {
        startAutoRestoredFile(orig.fileName, 0);
    });

    auto origFileDateTime = new QDateTimeEdit(groupBox);
    if(noOrigFile)
        origFileDateTime->setEnabled(false);

    origFileDateTime->setDateTime(orig.dateTime);
    gridLayout->addWidget(origFileDateTime, 2, 1, 1, 1);

    verticalLayout->insertWidget(verticalLayout->count() - 2, groupBox);
}

void AutoRestoreManagerDialog::startAutoRestoredFile(QString file, bool autosaved)
{
    QStringList args;
    if(autosaved)
        args << "--load-autosave";
    args << file;

    auto process = new QProcess;
    process->setProcessEnvironment(QProcessEnvironment::systemEnvironment());
    auto exeFile = QCoreApplication::arguments().first();
    process->start(exeFile, args);
}
