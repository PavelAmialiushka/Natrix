#include "sceneWidget.h"
#include "sceneWidgetDrawer.h"

#include <math.h>

#include <QApplication>
#include <QClipboard>
#include <QDebug>
#include <QGraphicsTextItem>
#include <QMap>
#include <QMimeData>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QTimer>
#include <QtGlobal>

#include <windows.h>

using namespace geometry;

SceneWidget::SceneWidget(QWidget *parent)
    : QGraphicsView(parent)
    , doc_(0)
    , scene_(0)
    , text_(0)
    , editMode_(0)
    , panMode_(0)
    , scale_(1.0)
{
    simpleDebugMode_ = QApplication::queryKeyboardModifiers() & Qt::ControlModifier;
}

void SceneWidget::replaceScene()
{
    setScene(doc_->scene());
}

void SceneWidget::setDocument(Document *doc)
{
    doc_ = doc;
    connect(doc_, SIGNAL(replaceScene()), SLOT(replaceScene()));

    setScene(doc->scene());
}

void SceneWidget::setScene(Scene *scene)
{
    if(!scene_)
    {
        // чтобы получать события движения мыши
        viewport()->setMouseTracking(true);

        // настройки
        setRenderHint(QPainter::Antialiasing, false);
        setDragMode(QGraphicsView::NoDrag);
        setOptimizationFlags(QGraphicsView::DontSavePainterState);
        setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
        setResizeAnchor(QGraphicsView::AnchorUnderMouse);
        //        setViewport(new QGLWidget);

        // скролл-бары
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

        // чтобы получать сообщения от клавиатуры
        setFocusPolicy(Qt::StrongFocus);

        // устанавливаем шрифт
        QFont font("Tahoma", 12);
        font.setKerning(0);
        setFont(font);

        ////////////////////////////////////////

        scale_ = scene->scale();

        // запускаем таймер на обновление
        auto utimer = new QTimer(this);
        utimer->start(100);
        connect(utimer, SIGNAL(timeout()), this, SLOT(timerUpdate()));
    }
    else if(scene_)
    {
        // во второй раз и последующие
        scene_->disconnect(this);
        scene_->manipulator()->disconnect(this);
    }

    scene_ = scene;
    connect(scene_, SIGNAL(update()), SLOT(updateViewport()));
    connect(scene_, SIGNAL(viewRectangleChanged()), SLOT(viewRectangleChanged()));

    connect(scene_, SIGNAL(onSetTextEdit(ITextEditor *)), SLOT(setTextEdit(ITextEditor *)));

    connect(scene_, SIGNAL(onSetCursor(QString)), SLOT(setCursor(QString)));
    connect(scene_->manipulator(), SIGNAL(toolChanged()), SLOT(toolChanged()));

    QGraphicsView::setScene(scene_->graphicsScene());

    emit updateHelper(scene_->manipulator()->helperText());

    updateSceneVisibleWindow();
}

void SceneWidget::updateViewport()
{
    viewport()->update();
}

bool SceneWidget::focusNextPrevChild(bool)
{
    return false;
}

void SceneWidget::focusOutEvent(QFocusEvent *event)
{
}

void SceneWidget::toolChanged()
{
    emit updateHelper(scene_->manipulator()->helperText());
}

geometry::Scene *SceneWidget::scene() const
{
    return scene_;
}

void SceneWidget::onCopy()
{
    auto *mime = new QMimeData;

    auto data = scene_->stringCopySelection();
    if(data.isEmpty())
        return;

    mime->setData("application/sktx-copyclip", data);

    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setMimeData(mime);
}

void SceneWidget::onCut()
{
    onCopy();

    scene_->manipulator()->deleteSelected();
}

void SceneWidget::onPaste()
{
    QClipboard *clipboard = QApplication::clipboard();
    auto        mime      = clipboard->mimeData();
    if(mime->hasFormat("application/sktx-copyclip"))
    {
        QByteArray data = mime->data("application/sktx-copyclip");
        scene_->manipulator()->paste(data);
    }
}

void SceneWidget::wheelEvent(QWheelEvent *evt)
{
    // изменение масштаба происходит так, чтобы
    // точка, на которую указывает курсор не изменилась

    rescale(evt->angleDelta().y() > 0 ? 1 : -1, evt->position().toPoint());
}

void SceneWidget::mouseDoubleClickEvent(QMouseEvent *evt)
{
    if(evt->button() == Qt::MiddleButton || evt->button() == Qt::RightButton)
    {
        scene_->setWindowToModelExtents();
    }
    else
    {
        auto p = mapToScene(evt->pos());
        if(editMode_ && text_ && text_->eventInItem(p.x(), p.y()))
        {
            QGraphicsView::mouseDoubleClickEvent(evt);
            evt->ignore();
        }
        else if(evt->button() == Qt::LeftButton)
        {
            scene_->manipulator()->debugLog("SceneWidget::mouseDoubleClickEvent");

            QPointF point   = mapToScene(evt->pos());
            dragStartPoint_ = point;
            dragStartTime_.start();
            dragging_ = false;

            point2d pt(point.x(), point.y());

            scene_->manipulator()->setModifiers(evt->modifiers() & Qt::SHIFT,
                                                evt->modifiers() & Qt::CTRL,
                                                evt->modifiers() & Qt::ALT);
            scene_->manipulator()->doubleClick(pt);
        }
    }

    emit updateHelper(scene_->manipulator()->helperText());
}

const double maximumScale = 10;
const double minimumScale = 0.2;

void SceneWidget::viewRectangleChanged()
{
    if(!scene_)
        return;
    // rect
    auto lt = scene_->recomendedLeftTop();
    auto rb = scene_->recomendedRightBottom();

    // устанавливаем масштаб
    auto   newSize = rb - lt;
    QSizeF oldSize = mapToScene(rect()).boundingRect().size();

    double ms = qMin(oldSize.width() / newSize.x, oldSize.height() / newSize.y) / 1.025;
    double ts = qMin(qMax(ms * scale_, minimumScale), maximumScale);
    scale_    = ts;
    applyScaleAndPan();

    // смещаем центр
    QRectF  modelRect(QPointF(lt.x, lt.y), QPointF(rb.x, rb.y));
    QPointF modelNewCenter = modelRect.center();
    QPoint  viewOldCenter  = this->rect().center();

    QPointF modelOldCenter = mapToScene(viewOldCenter);
    QPointF delta          = modelOldCenter - modelNewCenter;
    centerPoint_ -= delta;

    updateSceneVisibleWindow();
}

void SceneWidget::rescale(int factor, QPoint zoomCenter)
{
    // находим эту точку на модели
    QPointF zoomCenterF = mapToScene(zoomCenter);

    // масштабируем
    double ms = pow(1.25, factor > 0 ? 1 : -1) * scale_;
    double ts = qMin(qMax(ms, minimumScale), maximumScale);
    scale_    = ts;

    // предварительно применяем масштаб
    applyScaleAndPan();

    QPointF zoomCenterDelta = mapToScene(zoomCenter) - zoomCenterF;
    centerPoint_ -= zoomCenterDelta;

    // теперь уже обновляем по-серьезному
    updateSceneVisibleWindow();
}

void SceneWidget::startPan(QMouseEvent *evt)
{
    pan_point_ = evt->pos();
}

void SceneWidget::makePan(QMouseEvent *evt)
{
    QPoint  newPoint = evt->pos();
    QPointF delta    = mapToScene(newPoint) - mapToScene(pan_point_);

    centerPoint_ -= delta;
    pan_point_ = newPoint;

    updateSceneVisibleWindow();
}

// предварительное применение масштаба
void SceneWidget::applyScaleAndPan()
{
    // устанавливаем масштаб
    resetTransform();
    scale(scale_, scale_);

    // устанавливаем центральную точку
    setSceneRect(centerPoint_.x(), centerPoint_.y(), 1, 1);
}

void SceneWidget::updateSceneVisibleWindow()
{
    QRectF r = mapToScene(rect()).boundingRect();

    // сообщаем модели, что границы видимой области изменились
    scene_->updateVisibleWindow(point2d(r.left(), r.top()), point2d(r.right(), r.bottom()), scale_);
    applyScaleAndPan();

    viewport()->update();
}

void SceneWidget::updateSceneScale()
{
    QTimer::singleShot(50, [&]() { scene_->setWindowToCanvasRectangle(); });
}

void SceneWidget::resizeEvent(QResizeEvent *evt)
{
    viewport()->resize(maximumViewportSize());
    QGraphicsView::resizeEvent(evt);

    updateSceneVisibleWindow();
}

void SceneWidget::mousePressEvent(QMouseEvent *evt)
{
    if(evt->button() == Qt::MiddleButton || evt->button() == Qt::RightButton)
    {
        panMode_ = 1;
        startPan(evt);
    }
    else
    {
        bool lookForMouseMovements = true;
#ifndef NDEBUG
        lookForMouseMovements = !(evt->modifiers() & Qt::AltModifier);
        if(simpleDebugMode_)
            lookForMouseMovements = false;
#endif

        auto p = mapToScene(evt->pos());
        if(editMode_ && text_ && text_->eventInItem(p.x(), p.y()))
        {
            QGraphicsView::mousePressEvent(evt);
            evt->ignore();
        }
        else if(evt->button() == Qt::LeftButton)
        {
            scene_->manipulator()->debugLog("SceneWidget::mousePressEvent");

            QPointF point   = mapToScene(evt->pos());
            dragStartPoint_ = point;
            dragStartTime_.start();
            dragging_ = false;

            point2d pt(point.x(), point.y());

            scene_->manipulator()->setModifiers(evt->modifiers() & Qt::SHIFT,
                                                evt->modifiers() & Qt::CTRL,
                                                evt->modifiers() & Qt::ALT);
            if(lookForMouseMovements)
                scene_->manipulator()->move(pt);
        }
    }

    emit updateHelper(scene_->manipulator()->helperText());
}

void SceneWidget::mouseMoveEvent(QMouseEvent *evt)
{
    if(panMode_)
    {
        makePan(evt);
    }
    else
    {
        bool lookForMouseMovements = true;
#ifndef NDEBUG
        lookForMouseMovements = !(evt->modifiers() & Qt::AltModifier);
        if(simpleDebugMode_)
            lookForMouseMovements = false;
#endif
        auto p = mapToScene(evt->pos());
        if(editMode_ && text_ && text_->eventInItem(p.x(), p.y()))
        {
            QGraphicsView::mouseMoveEvent(evt);
            evt->ignore();
        }
        else
        {
            scene_->manipulator()->debugLog("SceneWidget::mouseMoveEvent");

            QPointF point = mapToScene(evt->pos());
            point2d pt(point.x(), point.y());

            bool acceptDrag =
                (point - dragStartPoint_).manhattanLength() > 20 || dragStartTime_.elapsed() > 200;
            bool isDrag = dragging_ || acceptDrag;

            scene_->manipulator()->setModifiers(evt->modifiers() & Qt::SHIFT,
                                                evt->modifiers() & Qt::CTRL,
                                                evt->modifiers() & Qt::ALT);
            if((evt->buttons() & Qt::LeftButton) && isDrag)
            {
                if(dragging_)
                {
                    scene_->manipulator()->drag(pt);
                }
                else
                {
                    dragging_ = true;
                    pt        = point2d(dragStartPoint_.x(), dragStartPoint_.y());
                    scene_->manipulator()->dragStart(pt);
                }
            }
            else if(lookForMouseMovements)
            {
                scene_->manipulator()->move(pt);
            }
        }
    }
}

void SceneWidget::mouseReleaseEvent(QMouseEvent *evt)
{
    if(evt->button() == Qt::MiddleButton || evt->button() == Qt::RightButton)
    {
        panMode_ = 0;
    }
    else
    {
        auto p = mapToScene(evt->pos());
        if(editMode_ && text_ && text_->eventInItem(p.x(), p.y()))
        {
            QGraphicsView::mouseReleaseEvent(evt);
            evt->ignore();
        }
        else if(evt->button() == Qt::LeftButton)
        {
            scene_->manipulator()->debugLog("SceneWidget::mouseReleaseEvent");

            QPointF point = mapToScene(evt->pos());
            point2d pt(point.x(), point.y());

            if(dragging_)
            {
                scene_->manipulator()->drop(pt);
                dragging_ = false;
            }
            else
            {
                scene_->manipulator()->click(pt);
            }
        }
    }
}

void SceneWidget::leaveEvent(QEvent *)
{
    scene_->manipulator()->moveOut();
}

void SceneWidget::keyPressEvent(QKeyEvent *evt)
{
#ifndef NDEBUG
    static QByteArray iddqd  = QByteArray("iddqd").toUpper();
    static QByteArray killme = QByteArray("65536").toUpper();
    static QByteArray buffer;

    buffer.push_back(evt->key());
    if(buffer.size() > 10)
        buffer = buffer.right(buffer.size() - 1);

    if(buffer.endsWith(iddqd))
    {
        simpleDebugMode_ = true;
        scene_->manipulator()->setSimpleDebugMode(true);
    }
    else if(buffer.endsWith(killme))
    {
        throw 0;
    }
#endif

    if(evt->modifiers() & Qt::ControlModifier)
    {
        switch(evt->key())
        {
        case 0x425: // Cyrillic BraceLeft
        case Qt::Key_BracketLeft:
        case Qt::Key_BraceLeft:
            if(editMode_)
                text_->changeRotation(-1);
            else
                scene_->manipulator()->changeFlanges(false);
            return;
        case 0x42a: // Cyrillic BraceRight
        case Qt::Key_BracketRight:
        case Qt::Key_BraceRight:
            if(editMode_)
                text_->changeRotation(+1);
            else
                scene_->manipulator()->changeFlanges(true);
            return;
        case Qt::Key_Asterisk:
            scene_->manipulator()->changeSize(0, true);
            return;
        case Qt::Key_Minus:
            scene_->manipulator()->changeSize(-1);
            return;
        case Qt::Key_Equal:
        case Qt::Key_Plus:
            scene_->manipulator()->changeSize(+1);
            return;

        case Qt::Key_1:
            scene_->manipulator()->changeSize(-4, true);
            return;
        case Qt::Key_2:
            scene_->manipulator()->changeSize(-3, true);
            return;
        case Qt::Key_3:
            scene_->manipulator()->changeSize(-2, true);
            return;
        case Qt::Key_4:
            scene_->manipulator()->changeSize(-1, true);
            return;
        case Qt::Key_5:
            scene_->manipulator()->changeSize(0, true);
            return;
        case Qt::Key_6:
            scene_->manipulator()->changeSize(1, true);
            return;
        case Qt::Key_7:
            scene_->manipulator()->changeSize(2, true);
            return;
        case Qt::Key_8:
            scene_->manipulator()->changeSize(3, true);
            return;
        case Qt::Key_9:
            scene_->manipulator()->changeSize(4, true);
            return;
        case Qt::Key_0:
            scene_->manipulator()->changeSize(5, true);
            return;
        }
    }

    keyModifiers_ = evt->key();
    if(evt->key() == Qt::Key_Tab)
    {
        scene_->manipulator()->togglePlane();
        scene_->manipulator()->moveToTheSamePoint();
        emit updateHelper(scene_->manipulator()->helperText());

        return;
    }
    else if(evt->key() == Qt::Key_Escape)
    {
        scene_->manipulator()->cancel();
        return;
    }
    else if(!editMode_ && (evt->key() == Qt::Key_Plus || evt->key() == Qt::Key_Equal))
    {
        QRect r = rect();
        rescale(+1, (r.bottomLeft() + r.topRight()) / 2);
        return;
    }
    else if(!editMode_ && evt->key() == Qt::Key_Minus)
    {
        QRect r = rect();
        rescale(-1, (r.bottomLeft() + r.topRight()) / 2);
        return;
    }
    else if(evt->key() == Qt::Key_Shift || evt->key() == Qt::Key_Alt ||
            evt->key() == Qt::Key_Control)
    {
        scene_->manipulator()->setModifiers(
            evt->modifiers() & Qt::SHIFT, evt->modifiers() & Qt::CTRL, evt->modifiers() & Qt::ALT);
        scene_->manipulator()->moveToTheSamePoint();
    }

    emit updateHelper(scene_->manipulator()->helperText());
    return QGraphicsView::keyPressEvent(evt);
}

void SceneWidget::keyReleaseEvent(QKeyEvent *evt)
{
    if(editMode_)
        return QGraphicsView::keyReleaseEvent(evt);

    if(evt->key() == Qt::Key_Shift || evt->key() == Qt::Key_Alt || evt->key() == Qt::Key_Control)
    {
        scene_->manipulator()->setModifiers(
            evt->modifiers() & Qt::SHIFT, evt->modifiers() & Qt::CTRL, evt->modifiers() & Qt::ALT);
        scene_->manipulator()->moveToTheSamePoint();
    }
}

void SceneWidget::dropWaitingShift()
{
    keyModifiers_ = 0;
}

void SceneWidget::undoEvent()
{
    scene_->manipulator()->undo();
    emit updateHelper(scene_->manipulator()->helperText());
}

void SceneWidget::redoEvent()
{
    scene_->manipulator()->redo();
    emit updateHelper(scene_->manipulator()->helperText());
}

void SceneWidget::onDelete()
{
    scene_->manipulator()->deleteSelected();
}

void SceneWidget::onSelectAll()
{
    scene_->manipulator()->selectAll();
}

void SceneWidget::onSelectNone()
{
    scene_->manipulator()->selectNone();
}

void SceneWidget::paintEvent(QPaintEvent *evt)
{
    // таким чудным образом отбрасываем события от Widget
    // и обрабатываем только события от viewport()
    if(evt->rect() == rect())
        return;

    scene_->recalcCaches();
    QGraphicsView::paintEvent(evt);
}

void SceneWidget::setCursor(QString name)
{
    QCursor cursor = QCursor(Qt::ArrowCursor);
    if(name == "size")
        cursor = QCursor(Qt::SizeAllCursor);
    else if(name == "move")
        cursor = QCursor(Qt::OpenHandCursor);
    else if(name == "moving")
        cursor = QCursor(Qt::ClosedHandCursor);
    else if(name == "stop")
        cursor = QCursor(Qt::ForbiddenCursor);
    else if(name == "hand")
        cursor = QCursor(Qt::ClosedHandCursor);
    else if(name == "text")
        cursor = QCursor(Qt::IBeamCursor);
    else if(name == "") // arrow
    {
        QPixmap pix(":/cursor-arrow.png");
        cursor = QCursor(pix, 5, 5);
    }
    else if(name == "+")
    {
        QPixmap pix(":/cursor-plus.png");
        cursor = QCursor(pix, 5, 5);
    }
    else if(name == "-")
    {
        QPixmap pix(":/cursor-minus.png");
        cursor = QCursor(pix, 5, 5);
    }
    else if(name == "+-")
    {
        QPixmap pix(":/cursor-plusminus.png");
        cursor = QCursor(pix, 5, 5);
    }
    else if(name == "text-new")
    {
        QPixmap pix(":/cursor-text-new.png");
        cursor = QCursor(pix, 12, 12);
    }
    else if(name == "erase")
    {
        QPixmap pix(":/cursor-erase.png");
        cursor = QCursor(pix, 2, 29);
    }
    else if(name == "match")
    {
        QPixmap pix(":/cursor-match.png");
        cursor = QCursor(pix, 2, 2);
    }
    else if(name == "match-empty")
    {
        QPixmap pix(":/cursor-match-empty.png");
        cursor = QCursor(pix, 2, 2);
    }

    QWidget::setCursor(cursor);
}

bool SceneWidget::event(QEvent *evt)
{
    if(editMode_)
    {
        switch(evt->type())
        {
        case QEvent::ShortcutOverride:
        case QEvent::KeyPress:
        case QEvent::KeyRelease: {
            QKeyEvent *ke = (QKeyEvent *)evt;

            // чтоб таб воспринимался как нажатая кнопка а не кака спец символ
            if(ke->key() == Qt::Key_Tab)
            {
                if(ke->type() == QEvent::KeyPress)
                    keyPressEvent(ke);
                if(ke->type() == QEvent::KeyRelease)
                    keyReleaseEvent(ke);
            }
            // выполняем вызов события
            else
                QGraphicsView::event(evt);

            text_->checkChanged();

            bool ctrl_pressed = ke->modifiers() & Qt::ControlModifier;
            switch(ke->key())
            {
            case Qt::Key_Return:
            case Qt::Key_Enter:
                // если без контрола, то конец редактирования
                if(!ctrl_pressed)
                    break;
                [[fallthrough]];
            case Qt::Key_Escape:
                textReturnPressed();
            }

            // событие не должно попасть дальше
            evt->accept();
            return true;
        }
        case QEvent::MouseButtonRelease: {
            auto r = QGraphicsView::event(evt);
            text_->checkFocus();
            return r;
        }
        default:;
        }
    }

    if(evt->type() == QEvent::KeyPress)
    {
        QKeyEvent *ke = (QKeyEvent *)evt;
        if(ke->key() == Qt::Key_Tab)
        {
            keyPressEvent(ke);
            ke->accept();
            return true;
        }
    }

    if(evt->type() == QEvent::KeyPress)
    {
        return QWidget::event(evt);
    }

    return QWidget::event(evt);
}

void SceneWidget::textReturnPressed()
{
    editMode_ = false;
    releaseKeyboard();
    text_->editFinished();
}

void SceneWidget::timerUpdate()
{
    if(scene_)
        scene_->recalcDeepCaches();
}

void SceneWidget::setTextEdit(ITextEditor *tx)
{
    if(editMode_ && text_ == tx)
    {
        setFocus();
        grabKeyboard();
        return;
    }

    text_     = tx;
    editMode_ = tx;

    if(editMode_)
    {
        setFocus();
        grabKeyboard();

        // изображаем клик на тексте, чтобы установить курсор
        // в нужное положение
        QPointF     sceneClick = text_->clickedPoint().toQPoint();
        QMouseEvent event(QEvent::MouseButtonPress,
                          mapFromScene(sceneClick),
                          Qt::LeftButton,
                          Qt::NoButton,
                          Qt::NoModifier);
        mousePressEvent(&event);
    }
    else
    {
        releaseKeyboard();

        // восстанавливаем видимость курсора
        QMouseEvent event(QEvent::MouseMove,
                          mapFromGlobal(QCursor::pos()),
                          Qt::NoButton,
                          Qt::NoButton,
                          Qt::NoModifier);
        mouseMoveEvent(&event);
    }
}
