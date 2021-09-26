#include "newDialog.h"
#include "ui_newDialog.h"

#include "geometry/document.h"

#include <QButtonGroup>
#include <QSettings>

using namespace geometry;

NewDialog::NewDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::NewDialog)
{
    ui->setupUi(this);

    QButtonGroup *group = new QButtonGroup;
    group->addButton(ui->radioIso);
    group->addButton(ui->radioDim);
    group->addButton(ui->radioPlane);

    int def = QSettings().value("plane_type", 1).toInt();

    switch(def)
    {
    case 1:
        ui->radioIso->setChecked(1);
        break;
    case 2:
        ui->radioDim->setChecked(1);
        break;
    case 3:
        ui->radioPlane->setChecked(1);
    }
}

NewDialog::~NewDialog()
{
    delete ui;
}

void NewDialog::done(int result)
{
    if(result)
    {
        if(ui->radioIso->isChecked())
            result = DocumentIsometric;
        if(ui->radioDim->isChecked())
            result = DocumentDimetric;
        if(ui->radioPlane->isChecked())
            result = DocumentPlane;
        QSettings().setValue("plane_type", result);
    }

    QDialog::done(result);
}
