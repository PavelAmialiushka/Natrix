#ifndef CHECKPOINTDIALOG_H
#define CHECKPOINTDIALOG_H

#include <QDialog>

#include <geometry/scene.h>
#include <geometry/textLabel.h>

namespace Ui
{
class CheckPointsDialog;
}

using geometry::Scene;

class CheckPointsDialog : public QDialog
{
    Q_OBJECT

    Scene *scene_;

public:
    explicit CheckPointsDialog(Scene *scene, QWidget *parent = 0);
    ~CheckPointsDialog();

private slots:
    void on_closeButton_clicked();

private:
    QMap<int, QString> analyzeLabels();
    QList<int>         searchLabels();

private:
    Ui::CheckPointsDialog *ui;
};

#endif // CHECKPOINTDIALOG_H
