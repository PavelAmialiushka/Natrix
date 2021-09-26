#ifndef GRAPHICSCENE_H
#define GRAPHICSCENE_H

#include <QGraphicsView>
#include <QLineEdit>
#include <QTime>
#include <QWidget>

#include "geometry/document.h"
#include "geometry/textInfo.h"
#include "geometry/toolInfo.h"

using geometry::Document;
using geometry::ITextEditor;
using geometry::PToolInfo;
using geometry::Scene;

class SceneWidget : public QGraphicsView
{
    Q_OBJECT

    geometry::Document *doc_;
    geometry::Scene *   scene_;

    ITextEditor *text_;
    bool         editMode_;

    int keyModifiers_;

    // pan
    QPoint pan_point_;
    bool   panMode_;

    QPointF centerPoint_;
    double  scale_;

    bool simpleDebugMode_;

    // dragging
    QTime   dragStartTime_;
    QPointF dragStartPoint_;
    bool    dragging_;

public:
    explicit SceneWidget(QWidget *parent = 0);

    void             setDocument(Document *doc);
    geometry::Scene *scene() const;

    void updateSceneScale();

signals:
    void updateHelper(QString);
public slots:
    void onCopy();
    void onCut();
    void onPaste();
    void onDelete();
    void undoEvent();
    void redoEvent();
    void onSelectAll();
    void onSelectNone();
    bool event(QEvent *evt);
    void paintEvent(QPaintEvent *);
    void mouseDoubleClickEvent(QMouseEvent *);
    void mouseReleaseEvent(QMouseEvent *);
    void mousePressEvent(QMouseEvent *);
    void mouseMoveEvent(QMouseEvent *);
    void leaveEvent(QEvent *);
    void wheelEvent(QWheelEvent *);
    void keyPressEvent(QKeyEvent *);
    void keyReleaseEvent(QKeyEvent *);
    void resizeEvent(QResizeEvent *);
    void updateViewport();
    void textReturnPressed();

    void timerUpdate();

    void viewRectangleChanged();
    void dropWaitingShift();
    void toolChanged();
    void replaceScene();
    void setCursor(QString);
    void setTextEdit(ITextEditor *);

private:
    void setScene(Scene *);
    void rescale(int factor, QPoint zoom_center);
    void updateSceneVisibleWindow();
    void resizeToWindow(QRect);

    void startPan(QMouseEvent *evt);
    void makePan(QMouseEvent *evt);
    void applyScaleAndPan();

    bool focusNextPrevChild(bool);
    void focusOutEvent(QFocusEvent *event);
};

#endif // GRAPHICSCENE_H
