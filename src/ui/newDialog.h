#ifndef NEWDIALOG_H
#define NEWDIALOG_H

#include <QDialog>

namespace Ui
{
class NewDialog;
}

class NewDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NewDialog(QWidget *parent = 0);
    ~NewDialog();

public slots:
    virtual void done(int result);

private:
    Ui::NewDialog *ui;
};

#endif // NEWDIALOG_H
