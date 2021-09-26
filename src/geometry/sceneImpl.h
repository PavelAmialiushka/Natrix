#ifndef SCENEIMPL_H
#define SCENEIMPL_H

#include "canvasRectangle.h"
#include "scene.h"
#include "straightLineAdorner.h"

#include <QHash>
#include <QMap>
#include <QSet>

#include <tuple>

#include "graphics.h"
#include "graphicsScene.h"

namespace geometry
{

static point2d INCORRECT_POINT = point2d(0x7FFFFF, 0x7FFFFF);

////////////////////////////////////////////////////////////////////////
struct SceneImpl
{
    PWorldSystem worldSystem;
    int          wsType;

    int modifyCounter;

    // указатель на сцену, включающую данный класс
    class Scene *scene;

    GraphicsScene gscene;

    // перечень всех объектов
    QSet<PObject>          objects;
    QList<PObjectToSelect> invalidatedObjects;
    QList<PObjectToSelect> invalidatedObjectsDeep;

    // метки
    QSet<PLabel> labels;

    // выделенные объекты
    QSet<PObjectToSelect> selectedObjects;
    // индикация объектов под мышкой и проч.
    QHash<PObjectToSelect, HoverState> hoverObjects;

    // TODO удалить
    QList<QSet<PObjectToSelect>>   old_gitems_generations;
    QHash<PObjectToSelect, GItems> old_gitems_cache;

    QHash<PObjectToDraw, GItemsList>        gitems;
    QHash<PObjectToSelect, GItemsList>      gcontur;
    QHash<QGraphicsItem *, PObjectToSelect> gitemToObject;

    // список всех адорнеров
    QList<PAdorner> adorners;

    // список всех грипов
    QList<PGrip> grips;

    // ускорение поиска нодов
    QMultiHash<point3d, PNode> nodesCache;

    // работа с маркерами
    QSet<PMarker>                        markers;
    QMultiHash<PLabel, PMarker>          markerFollower;
    QMultiHash<PObjectToSelect, PMarker> markerLeaders;
    QHash<QString, PMarker>              markerSerialCache;

    // масштаб
    double  scale;
    double  epsilon() const;
    point2d centerPoint;

    // значения по умолчанию
    LineInfo    defaultLineInfo;
    TextInfo    defaultTextInfo;
    ElementInfo defaultElementInfo;

    // точка, где сейчас находится курсор
    point2d             cursorPoint;
    QGraphicsItemGroup *cursorItems;
    void                updateCursorItems();

    // поле, в которое выводится все изображение
    QList<PCanvasRectangle> canvases;

    point2d visibleLt_;
    point2d visibleRb_;

    point2d recomendedLt_;
    point2d recomendedRb_;

    // ускорение поиска пересечений
    struct Cross
    {
        QPainterPath path;
    };
    struct ObjectCrossData
    {
        QSet<PObject>  lower;
        QSet<PObject>  upper;
        QList<Cross>   crosses;
        QList<point2d> segments;
    };

    QGraphicsScene                 crossScene;
    QMap<PObject, PGItem>          crossObject2Item;
    QMap<QGraphicsItem *, PObject> crossItem2Object;

    QSet<PObject>                  invalidatedCrossObjects;
    QMap<PObject, ObjectCrossData> objectCrossData;

public:
    QList<Cross> getCrosses(PObject object) const;
    void         makeSelected(PObjectToSelect, HoverState set, bool selected);
    void         updateSelection(QSet<PObjectToSelect>);

private:
    void recalcCrosses();

public:
    // StraightLine адорнеры
    QList<PObject>               invalidatedAdornerObjects;
    QMultiMap<PAdorner, PObject> mapAdorner2object;
    QMap<PObject, PAdorner>      mapObject2adorner;

private:
    void removeObjectAdorner(PObject object);
    void recalculateLineAdorners();

    void removeObjectCrossBase(PObject object);

public:
    SceneImpl(int type);
    QStringList dumpContents() const;

    void addCanvas(PCanvasRectangle);
    void initCanvas(Scene *scene, bool empty);
    void recalcCaches();
    void recalcDeepCaches();
    void clearOldGItemsCache();

    PNode          getPartnerNode(PNode node) const;
    PNode          getSecondNode(PNode node) const;
    ConnectionData findConnectedNode(PNode node, bool force = false) const;

    void           analizeCloseObject(PNeighbourhood  nei,
                                      PObjectToSelect sel,
                                      point2d         b,
                                      bool            preferNodes = true,
                                      bool            nolimit     = false) const;
    PNeighbourhood getClosestObjects(point2d a, point2d b, double radius = 0.0) const;
    PNeighbourhood getObjectsRectangle(point2d a, point2d b) const;

    void onAttachObject(PObject object);
    void onDetachObject(PObject object);

    void attachAdorner(PAdorner adorner);
    void detachAdorner(PAdorner adorner);

    void attachGrip(PGrip grip);
    void detachGrip(PGrip grip);

    QSet<PObject> updateNeighboursGraphicsItem(PObject);
    void          detachGraphicsItem(PObjectToDraw object);
    bool          updateGraphicsItem(PObjectToDraw object, int level);

private:
    void updateGraphicsItem(PObjectToDraw object, GItems &items);
};

} // namespace geometry

#endif // SCENEIMPL_H
