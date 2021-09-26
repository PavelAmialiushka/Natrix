#ifndef CANVASPROPERTIESDIALOG_H
#define CANVASPROPERTIESDIALOG_H

#include <QDialog>

#include <geometry/scene.h>
using namespace geometry;

namespace Ui
{
class CanvasPropertiesDialog;
}

class CanvasPropertiesDialog : public QDialog
{
    Q_OBJECT

    bool lock_;

public:
    explicit CanvasPropertiesDialog(Scene *scene, QWidget *parent = nullptr);
    ~CanvasPropertiesDialog();

    void       updateCurrentCanvas();
    void       updateCanvasList();
    CanvasInfo getCurrentInfo(PCanvasRectangle canvas);

    void registerCommand(Command cmd);

private slots:
    void on_canvasNo_currentRowChanged(int currentRow);
    void on_portraitOrientation_clicked();
    void on_landscapeOrientation_clicked();
    void on_pageSize_currentIndexChanged(int index);

    void on_canvasNo_itemSelectionChanged();

    void on_addButton_clicked();

    void on_delButton_clicked();

private:
    Scene *                     scene_;
    Ui::CanvasPropertiesDialog *ui;
};

#endif // CANVASPROPERTIESDIALOG_H
