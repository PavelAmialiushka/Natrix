#include "aboutDialog.h"
#include "ui_aboutDialog.h"

AboutDialog::AboutDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AboutDialog)
{
    ui->setupUi(this);

    ui->labelAppName->setText(
        ui->labelAppName->text().arg(QApplication::instance()->applicationVersion()));

    connect(ui->pushClose, SIGNAL(clicked()), SLOT(accept()));

    ui->labelURL->setText(QString("<a href=\"http://%1\">%1</a>")
                              .arg(QApplication::instance()->organizationDomain()));
}

AboutDialog::~AboutDialog()
{
    delete ui;
}
