#ifndef SCENE_H
#define SCENE_H

#include <QList>
#include <QMultiMap>

#include "adorner.h"
#include "grip.h"
#include "node.h"
#include "primitives.h"
#include "sceneProperties.h"
#include "worldSystem.h"

#include "element.h"
#include "lineInfo.h"

#include <tuple>

#include <QtPrintSupport/QPrinter>

class QGraphicsScene;
class QGraphicsTextItem;
class QPainter;

namespace geometry
{

struct SceneImpl;
class Manipulator;
class Document;
class ITextEditor;

class ObjectToDraw;

class Label;
struct LabelInfo;
typedef QSharedPointer<Label> PLabel;

class Object;
class ObjectInfo;
struct ConnectionData;
typedef QSharedPointer<Object>       PObject;
typedef QSharedPointer<ObjectToDraw> PObjectToDraw;

class Marker;
typedef QSharedPointer<Marker> PMarker;

struct Neighbourhood;
typedef QSharedPointer<Neighbourhood> PNeighbourhood;

struct CanvasInfo;
class CanvasRectangle;
typedef QSharedPointer<CanvasRectangle> PCanvasRectangle;

enum HoverState
{
    NoHover = 0, // нет особого статуса
    Hover   = 1, // мышка висит над

    Selected = 0x04, // элемент выделен

    // специальные маски
    Newby        = 0x08, // элемент только что добавлен / элемент собирается быть удаленным
    ToBeSelected = 0x10, // элемент выделен + наведена мышка / наведена мышка чтобы выделить
    ToModify     = 0x20, // элемент будет изменён

    HasContour = Hover | Newby | ToBeSelected | ToModify,
};

class Scene : public QObject
{
    Q_OBJECT

    friend class Manipulator;

    SceneImpl *  pimpl;
    Manipulator *manipulator_;
    Document *   document_;

public:
    Scene(int, bool empty = false);
    ~Scene();

    void swap(Scene *);

public:
    PWorldSystem worldSystem() const;
    void         setWorldSystem(PWorldSystem);

    Manipulator *   manipulator() const;
    SceneProperties getPropertiesFromScene();

    QList<int> textAngles() const;

signals:
    void viewRectangleChanged();
    void update();
    void onSetCursor(QString);
    void onSetTextEdit(ITextEditor *);
    void replaceBy(Scene *);

    // означает, что набор свойств изменился
    void updatePropertiesAfterSceneChange();

    void updateUI();

public:
    void setCursor(QString);
    void setTextEdit(ITextEditor *);

public:
    Document *document() const;
    void      clear();

    // тип плоскости сцены
    int         planeType() const;
    QStringList dumpContents() const;

    // счетчик модификаций
    int  modifyCounter() const;
    void modify(int delta);

    // добавление и удаление объектов сцены
    void          attach(PObject);
    void          detach(PObject);
    bool          contains(PObject) const;
    QSet<PObject> objects() const;
    void          recalcCaches() const;
    void          recalcDeepCaches() const;
    void          clearOldGItemsCache() const;

    // добавление
    void attachLabel(PLabel);
    void detachLabel(PLabel);
    void updateLabel(PLabel);

    bool         contains(PLabel) const;
    QSet<PLabel> labels() const;

    void invalidateObject(PObject);

    // работа с маркерами
    void           attachMarker(PMarker marker);
    void           detachMarker(PMarker marker);
    QList<PMarker> markersOfFollower(PLabel follower) const;
    QList<PMarker> markersOfLeader(PObjectToSelect object) const;
    QList<PMarker> markers() const;

    // работа с грипами
    void attachGrip(PGrip);
    void detachGrip(PGrip);

    QByteArray stringCopySelection();

    // масштаб
    double scale() const;
    double epsilon() const;

    QGraphicsScene *graphicsScene() const;
    void            preparePrinter(QPrinter *printer, int canvasNo = 0);
    void            render(QPainter *, QPrinter * = 0, int canvasNo = 0);

    // прямоугольник, который необходимо видеть
    // устанавлиается по командам пользователя
    void    setRecomendedWindow(point2d lt, point2d rb);
    point2d recomendedLeftTop() const;
    point2d recomendedRightBottom() const;

    // прямоугольник, который виден пользователю
    // устанавливается окном с учетом фактических
    // пропорций экрана.
    // Гарантированно    recomendedWindow < visibleWindow
    point2d visibleLeftTop() const;
    point2d visibleRightBottom() const;

    // обновление вида экрана, вызывается когда виджет сообщает
    // сцене, какие координаты видны пользователю
    void updateVisibleWindow(point2d lb, point2d rb, double scale);

    int              bestVisibleCanvas() const;
    int              canvasCount() const;
    PCanvasRectangle canvas(int index) const;

    // set selected window
    void setWindowToCanvasRectangle(int index = 0);
    void setWindowToModelExtents();

    // курсор
    void    setCursorPoint();
    void    setCursorPoint(point2d);
    bool    noCursorPoint() const;
    point2d cursorPoint() const;

    // полная работа по ховерам
    void setHoverObjects(QSet<PObjectToSelect> objects, HoverState state, HoverState mask);

    // hover objects
    void setHoverObjects(QMap<PObjectToSelect, HoverState> const &ho);

    // получить элементы, для которых установлен определенный флаг
    QSet<PObjectToSelect> getHoverObjects(HoverState state) const;

    bool                         isSelected(PObjectToSelect) const;
    QSet<PObjectToSelect> const &selectedObjects() const;
    void                         addSelectedObjects(QSet<PObjectToSelect>);
    void                         removeSelectedObjects(QSet<PObjectToSelect>);
    void                         clearSelection();
    bool                         hasUnselected() const;

    LineInfo defaultLineInfo() const;
    void     setDefaultLineInfo(LineInfo);

    TextInfo defaultTextInfo() const;
    void     setDefaultTextInfo(TextInfo);

    ElementInfo defaultElementInfo() const;
    void        setDefaultElementInfo(ElementInfo);

    // adrners
    void            clearAdorners();
    void            pushAdorner(Adorner *);
    void            pushAdorner(PAdorner);
    QList<PAdorner> getAdorners() const;

    // grips
    void         clearGrips();
    void         pushGrip(PGrip);
    void         removeGrip(PGrip old);
    QList<PGrip> getGrips() const;

    // работа с нодами
    ConnectionData findConnectedNode(PNode, bool force = false) const;
    PNeighbourhood getObjectsRectangle(point2d a, point2d b) const;

    ObjectInfo analizeCloseObject(PObjectToSelect sel, point2d b, bool preferNodes = true) const;
    void       analizeCloseObject(PNeighbourhood  nei,
                                  PObjectToSelect sel,
                                  point2d         b,
                                  bool            preferNodes = true) const;

    PNeighbourhood getClosestObjects(point2d a, point2d b) const;
    PNeighbourhood getClosestObjectsEx(point2d a, double radius) const;

    PNeighbourhood getClosestObjects(point2d pt) const;
};

} // namespace geometry

#endif // SCENE_H
