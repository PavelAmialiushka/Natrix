#ifndef FINDDIALOG_H
#define FINDDIALOG_H

#include <QDialog>

#include <geometry/scene.h>
#include <geometry/textLabel.h>

namespace Ui
{
class FindDialog;
}

using geometry::Scene;

class FindDialog : public QDialog
{
    Q_OBJECT

    Scene *scene_;

public:
    explicit FindDialog(Scene *scene, QWidget *parent = 0);
    ~FindDialog();

private slots:
    void on_textEdit_textChanged();
    void on_findNextButton_clicked();
    void on_excactMatch_toggled(bool checked);

    void on_closeButton_clicked();

private:
    bool                        eventFilter(QObject *, QEvent *);
    QList<geometry::PTextLabel> searchLabels();

private:
    Ui::FindDialog *ui;
};

#endif // FINDDIALOG_H
