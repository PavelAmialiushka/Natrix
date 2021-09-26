#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QActionGroup>
#include <QButtonGroup>
#include <QMainWindow>
#include <QtPrintSupport/QPrinter>
#include <geometry/sceneProperties.h>

#include "geometry/document.h"
#include "geometry/toolInfo.h"
#include "sceneWidget.h"

namespace Ui
{
class MainWindow;
}

using std::function;

using namespace geometry;

class QSplitter;

class ActionMaker : public QObject
{
    Q_OBJECT

    function<void()> ptr;

public:
    ActionMaker(function<void()> ptr, QObject *parent);
    ~ActionMaker();

public slots:
    void toggled(bool checked);
};

/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////

class WidgetBoxTreeWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void setupSplitter();

    void showMaximized();

public slots:
    void toggle640x480();
    void toggle800x600();

    void onNewDocument();
    void onOpenDocument();
    void onSaveDocument();
    void onSaveAsDocument();
    void onOpenMRUDocument();

    void onPrintPreview();
    void onPrint();
    void onCanvasProperties();
    void onExport();
    bool printPage(QPrinter *);
    void onFindText();
    void onCheckPoints();

    void toggledSelect(bool);
    void toggledMove(bool);
    void toggledLine(bool);
    void toggledElement(bool);
    void toggledErase(bool);
    void updatePlane();
    void selectPlane();
    void updateHelper(QString);
    void toolChanged();

    void onCheckUpdate();
    void onAbout();
    void onAboutQt();

    void updateMRU();

    void onTogglePropertyTab();
    void onToggleToolsTab();

    void updateUIEvent();
    void closeEvent(QCloseEvent *);

    void startNewDocument();
    void startNewDocument(int);
    int  readCommandLine();
    bool analyzeAutoSavedData();

private:
    void addToolSet(class WidgetBoxTreeWidget *, geometry::ToolSet);
    bool openDocument();
    bool openDocument(QString, bool *busy = 0, bool autoSave = false);
    bool openAutoSaveDocument(QString openAs);
    bool saveDocument();
    bool saveAsDocument();
    bool saveCurrentDocument();
    bool saveDocument(QString);
    bool saveIfModified();
    void connectSignals();
    void setupTreeWidgetLater();
    void setupTreeWidget();
    void setupPropertyWidget();

    void updateTitle();
    void analyzeRegistration();
    void updateMRU(QString name);

private:
    QActionGroup *       instrumentGroup;
    QActionGroup *       planeGroup;
    Ui::MainWindow *     ui;
    QSplitter *          splitter;
    WidgetBoxTreeWidget *treeWidget_;
    geometry::Scene *    scene_;

    QMap<geometry::PToolInfo, class InstrumentButton *> mapTool2Button;

    QStringList mostRecentlyUsed_;
};

#endif // MAINWINDOW_H
