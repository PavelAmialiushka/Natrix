#ifndef EXPORTDIALOG_H
#define EXPORTDIALOG_H

#include <QDialog>

#include "geometry/scene.h"
using namespace geometry;

namespace Ui
{
class ExportDialog;
}

class ExportDialog : public QDialog
{
    Q_OBJECT

    Scene *scene_;

public:
    explicit ExportDialog(Scene *scene, QWidget *parent = 0);
    ~ExportDialog();

    int getCurrentDPI();

public slots:
    void imageSizeChanged(QString);
    void exportToClipboard();
    void exportToFile();
    void exportToPDF();

private:
    QImage *makeAnImage();

private:
    Ui::ExportDialog *ui;
};

#endif // EXPORTDIALOG_H
