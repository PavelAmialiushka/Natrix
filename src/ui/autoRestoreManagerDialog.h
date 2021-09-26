#ifndef AUTORESTOREMANAGERDIALOG_H
#define AUTORESTOREMANAGERDIALOG_H

#include <QDateTime>
#include <QDialog>

namespace Ui
{
class AutoRestoreManagerDialog;
}

struct ARItem
{
    QString   fileName;
    QDateTime dateTime;
};

class AutoRestoreManagerDialog : public QDialog
{
    Q_OBJECT

    QStringList fileNames_;

public:
    explicit AutoRestoreManagerDialog(QStringList names, QWidget *parent = 0);
    ~AutoRestoreManagerDialog();

    void updateContents();

private:
    void        insertPanel(ARItem left, ARItem right);
    static void startAutoRestoredFile(QString, bool autosaved);

    Ui::AutoRestoreManagerDialog *ui;
};

#endif // AUTORESTOREMANAGERDIALOG_H
