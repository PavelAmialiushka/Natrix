#include "sceneImpl.h"

#include "global.h"

#include "element.h"
#include "joiners.h"
#include "line.h"
#include "neighbourhood.h"
#include "path.h"
#include "sceneProcedures.h"

#include "marker.h"
#include "pointProcedures.h"
#include "utilites.h"

#include <QSet>

#include <QDebug>
#include <QElapsedTimer>

#include <functional>
using std::bind;
using namespace std::placeholders;

#include "elementFactory.h"
#include "glueObject.h"
#include "graphics.h"
#include "manipulator.h"
#include "markLabels.h"
#include "performanceLogger.h"

//#define OLD_GITEMS_CACHE 1
//#define RECALC_CACHE_PERFORMANCE_LOG 1

namespace geometry
{

SceneImpl::SceneImpl(int type)
    : worldSystem(wsByName(type))
    , wsType(type)
    , gscene(worldSystem)
    , scale(1.0)
    , cursorPoint()
    , cursorItems(0)
    , defaultElementInfo("ValveElement")
{
#ifdef OLD_GITEMS_CACHE
    old_gitems_generations << QSet<PObjectToSelect>();
#endif
    modifyCounter = 0;
}

QStringList SceneImpl::dumpContents() const
{
    QStringList result;
    foreach(PObject object, objects)
    {
        QStringList nodes;
        foreach(PNode node, object->nodes())
            if(node->jointType() == 0)
            {
                auto pt = node->globalPoint();
                nodes << QString("%1,%2,%3")
                             .arg(pt.x, 0, 'f', 0)
                             .arg(pt.y, 0, 'f', 0)
                             .arg(pt.z, 0, 'f', 0);
            }

        result << QString("%1 %2").arg(object->typeName()).arg(nodes.join(" "));
    }
    return result;
}

void SceneImpl::addCanvas(PCanvasRectangle canvas0)
{
    CanvasInfo info = canvas0.dynamicCast<CanvasRectangle>()->info();

    auto by_index = [](PCanvasRectangle lhs, PCanvasRectangle rhs) {
        return lhs->info().index < rhs->info().index;
    };

    if(info.index == -1)
    {
        info.index = canvases.empty() ? 1 : 1 + canvases.last()->info().index;
        canvas0->setInfo(info);

        canvases << canvas0;
    }
    else
    {
        canvases << canvas0;

        // сортировка по возрастанию индекса
        std::sort(canvases.begin(), canvases.end(), by_index);
    }
}

void SceneImpl::initCanvas(Scene *scene, bool empty)
{
    if(!canvases.empty())
        return;

    PLabel canvas0 = CanvasRectangle::create(scene, CanvasInfo::def());
    addCanvas(canvas0.dynamicCast<CanvasRectangle>());

    point2d size = canvas0->rightBottom() - canvas0->leftTop();
    gscene.setSceneRect(0, 0, size.x, size.y);

    if(!empty)
    {
        labels << canvas0;
        updateGraphicsItem(canvas0, 1);
    }
}

QList<SceneImpl::Cross> SceneImpl::getCrosses(PObject object) const
{
    return objectCrossData[object].crosses;
}

static bool objectIsLower(PWorldSystem pw, PObject o1, PObject o2)
{
    int cnt1 = o1.dynamicCast<Joiner>() ? 1 : o1->nodeCount();
    int cnt2 = o2.dynamicCast<Joiner>() ? 1 : o2->nodeCount();
    switch(cnt1 * 10 + cnt2)
    {
    case 21: // если первый элемент "шире" второго,
    case 31: // то меняем аргументы местами, чтобы
    case 32: // не дублировать код
    case 41:
    case 42:
    case 43:
        return !objectIsLower(pw, o2, o1);

    case 11: {
        point2d s1 = o1->localPoint(0);
        point2d s2 = o2->localPoint(0);
        return s1.z < s2.z;
    }
    case 12: {
        // линия или элемент. определяем ближайшую
        // точку этого элемента и смотрим, находится ли она
        // выше точки джойнера
        point2d s1 = o1->localPoint(0);
        point2d closestPoint =
            o1->globalPoint(0).project_to_line(o2->globalPoint(0), o2->globalPoint(1)) >> *pw;
        return s1.z < closestPoint.z;
    }
    case 14:
    case 13: {
        // плоскость широкого элемента
        auto plane = wsPlane(pw, o2->globalPoint(0), o2->globalPoint(1), o2->globalPoint(2));

        point2d s1           = o1->localPoint(0);
        point2d closestPoint = s1 >> plane >> *pw;
        return s1.z < closestPoint.z;
    }
    case 24:
    case 23:
    case 22: {
        point2d s1 = o1->localPoint(0);
        point2d e1 = o1->localPoint(1);
        point2d s2 = o2->localPoint(0);
        point2d e2 = o2->localPoint(1);

        if(o2->nodeCount() == 3)
        {
            // по ходу дела правильное сравнение невозможно, поскольку
            // возможен случай, когда прямая "протыкает" плоскость и
            // значит часть прямой лежит ниже, а часть выше плоскости
            // однако для наших целей будем заменять плоскость прямой,
            // которая состоит из нодов № 0 и 2 элемента
            s2 = o2->localPoint(0);
            e2 = o2->localPoint(2);
        }

        bool    in_segment, notparallel;
        point2d cross = cross_lines2d(s1, e1, s2, e2, &in_segment, &notparallel);

        if(!notparallel)
        {
            // нужно определить какая из параллельных линий
            // лежит ниже, но линии не пересекаются.
            // попробуем так:
            cross = s1;
        }

        // определяем видимую точку пересечения линий,
        // и на ней сравниваем координаты первой и второй линий
        auto c1 = cross.project_to_line(s1, e1);
        auto c2 = cross.project_to_line(s2, e2);

        return c1.z != c2.z ? c1.z < c2.z
               // если пересекаются в одной точке, то для определенности
               // принимаем решение по координатам начальных точек
               : s1.x != s2.x ? s1.x < s2.x
                              : s1.y < s2.y;
    }
    case 44:
    case 34:
    case 33: {
        // ничего пока не придумывается чем такое экспресс сравнение
        return o1->localPoint(0).z < o2->localPoint(1).z;
    }
    default:
        Q_ASSERT(!"impossible");
        return false;
    }
}

void SceneImpl::recalcCrosses()
{
    QSet<PObject> objectsToClip;

    // вспомогательная функция
    auto getObjectContur = [&](PObject object) -> QPainterPath {
        // берем элемент, отвечающий за контур
        auto gc = gcontur.value(object, GItemsList(&gscene));
        if(gc.isEmpty())
            return QPainterPath();

        // получаем контур элемента
        return gc[0].dynamicCast<QGraphicsPathItem>()->path();
    };

    // запоминаем элементы, которые почему-то ещё не отрисованы
    QSet<PObject> reinvalidate;

    // первый проход, создаём контуры
    foreach(PObject object, invalidatedCrossObjects)
    {
        // в любом случае надо пересчитать
        // иначе пропустим объекты, у которых нужно удалить клип
        objectsToClip << object;

        QPainterPath path = getObjectContur(object);
        if(path.isEmpty())
        {
            // элемент по какой-то причине не прорисован
            if(!gitems.contains(object))
                reinvalidate << object;

            continue;
        }

        PGItem pathItem = makePGItem(new QGraphicsPathItem(path), &crossScene);

        // сохраняем полученный контур
        crossObject2Item[object]          = pathItem;
        crossItem2Object[pathItem.data()] = object;

        crossScene.addItem(pathItem.data());
        //    }

        //    // второй проход, изучаем пересечения
        //    foreach(PObject object, invalidatedCrossObjects)
        //    {
        //        PGItem pathItem = crossObject2Item[object];
        //        if (!pathItem) continue;
        //        QPainterPath path = pathItem.dynamicCast<QGraphicsPathItem>()->path();

        ObjectCrossData &objectData = objectCrossData[object];

        // находим те элементы, которые пересекаются с нашим контуром
        auto items = crossScene.items(path);
        foreach(QGraphicsItem *item, items)
        {
            PObject partner = crossItem2Object.value(item);
            if(!partner)
                continue;

            // если пересечение уже расчитано, пропускаем
            if(objectData.upper.contains(partner) || objectData.lower.contains(partner))
                continue;

            // эти элементы не должны затенять друг друга,
            // они могут рисоваться поверх
            // пропускаем
            if(areObjectsTouching(scene, object, partner))
                continue;

            ObjectCrossData &partnerData = objectCrossData[partner];

            // определяем путь пересечения
            QPainterPath crossPath = getObjectContur(partner).intersected(path);

            // данные по пересечению
            Cross cr{crossPath};

            // определяем, какой из элементов выше или ниже
            if(objectIsLower(worldSystem, object, partner))
            {
                objectData.upper << partner;
                partnerData.lower << object;
                objectData.crosses << cr;
            }
            else
            {
                partnerData.upper << object;
                objectData.lower << partner;
                partnerData.crosses << cr;

                // партнер оказался низу, надо пересчитать
                objectsToClip << partner;
            }
        }
    }

    // закончили работу
    invalidatedCrossObjects.clear();

    // перерисуем в следующий раз
    invalidatedCrossObjects += reinvalidate;

    foreach(PObject object, objectsToClip)
    {
        // формируем области, которые должны быть вырезаны
        auto &data = objectCrossData[object];

        // сканирование закончено. теперь
        // формируем "слепые" пятна
        foreach(PGItem it0, gitems.value(object, &gscene))
        {
            if(auto item = it0.dynamicCast<GraphicsClipItem>())
            {
                // сбрасываем область
                item->resetNegativePath();

                // если нет пересечений, то и делать ничего не надо
                if(data.crosses.isEmpty())
                    continue;

                foreach(Cross const &cr, data.crosses)
                {
                    item->addNegativePath(cr.path);
                }
            }
        }
    }
}

void SceneImpl::removeObjectCrossBase(PObject object)
{
    invalidatedCrossObjects.remove(object);

    // удяляем элементы из "новой" базы
    auto item = crossObject2Item.take(object);
    if(item)
        crossItem2Object.remove(item.data());

    // объекты, которые выше не нужно пересчитывать
    foreach(PObject upper, objectCrossData[object].upper)
        objectCrossData[upper].lower.remove(object);

    foreach(PObject lower, objectCrossData[object].lower)
    {
        objectCrossData.remove(lower);
        invalidatedCrossObjects << lower;
    }

    objectCrossData.remove(object);
}

static bool canBeConnected(PObject object, PObject partner)
{
    Q_ASSERT(object && partner);
    if(object.dynamicCast<GlueObject>())
    {
        if(partner.dynamicCast<Element>())
            return 1;
        return 0; // line, glue, joiner
    }
    if(object.dynamicCast<Line>())
    {
        if(partner.dynamicCast<Line>())
            return 0;
        return 1; // joiner, element, glue
    }
    if(object.dynamicCast<Element>())
    {
        if(partner.dynamicCast<Joiner>())
            return 0;
        return 1; // element, line, glue
    }
    if(object.dynamicCast<Joiner>())
    {
        if(partner.dynamicCast<Line>())
            return 1;
        return 0; // joiner, element, glue
    }
    Q_ASSERT(!"unexpected object type");
    return 0;
}

ConnectionData SceneImpl::findConnectedNode(PNode node, bool force) const
{
    auto    object = node->object();
    point3d point  = node->globalPoint();
    point3d dir    = node->direction();
    foreach(PNode otherNode, nodesCache.values(point))
    {
        PObject otherObject = otherNode->object();
        if(!otherObject)
            continue;
        if(otherObject != object && otherNode->direction() == -dir &&
           (force || canBeConnected(object, otherObject)))
        {
            ConnectionData data = {
                point, {0.0, true, otherObject->nodes().indexOf(otherNode), point}, otherObject};
            return data;
        }
    }

    ConnectionData data = {point, {0.0, false, 0, point}, PObject()};

    return data;
}

PNode SceneImpl::getSecondNode(PNode node) const
{
    PObject object(node->object());
    PNode   second = object->nodeAt(0);
    if(node == second)
        second = object->nodeAt(1);
    return second;
}

PNode SceneImpl::getPartnerNode(PNode node) const
{
    return findConnectedNode(node).node();
}

static double byDistanceFrom(point3d point, point3d a, point3d b)
{
    return point.distance(a) < point.distance(b);
}

namespace
{

// набор функций, для построения LineAdorners

bool canTakePartInLineAdorner(PNode node)
{
    return !node->direction().isAxeParallel();
}

static std::tuple<point3d, point3d> getOuterPoints(QSet<point3d> points1)
{
    auto points = points1.toList();
    if(points.empty())
        return std::make_tuple(point3d(), point3d());

    // выбираем две самые удаленные точки
    point3d first = points.first();
    std::sort(points.begin(), points.end(), bind(byDistanceFrom, first, _1, _2));

    first = points.last();
    std::sort(points.begin(), points.end(), bind(byDistanceFrom, first, _1, _2));
    point3d second = points.last();

    return std::make_tuple(first, second);
}

} // namespace

void SceneImpl::recalculateLineAdorners()
{
    QSet<PObject>       doneObjects;
    QList<QList<PNode>> bigListOfNodes;

    while(!invalidatedAdornerObjects.empty())
    {
        // по одному выбираем объекты, для которых нужно пересчитать адорнеры
        PObject object = invalidatedAdornerObjects.takeFirst();
        if(doneObjects.contains(object))
            continue;
        doneObjects << object;

        // используем только действующие объекты
        if(!objects.contains(object))
            continue;

        // выбираем ноды, которые можем проанализировать
        QList<PNode> nodesToProceedNext =
            utils::filtered(object->nodes(), [&](PNode n) { return canTakePartInLineAdorner(n); });

        // перебираем все соседние ноды
        QSet<PNode> doneNodes;
        while(nodesToProceedNext.size())
        {
            QSet<PNode> selNodes;

            PNode first = nodesToProceedNext.first();

            auto d               = first->direction();
            auto proceedAndStore = [&](NodePathInfo const &info) -> bool {
                // проверка нахождения на одной линии
                bool layOnTheSameLine =
                    info.node->globalPoint().project_to_line(
                        first->globalPoint(), first->globalPoint() + first->direction()) ==
                    info.node->globalPoint();

                // если совпадает направление и лежат на одной линии
                // то добавляем ко множеству
                if(info.node->direction().isParallel(d) && layOnTheSameLine)
                {
                    selNodes << info.node;
                    return 1;
                }
                else
                    return 0;
            };

            Path::makePathSimple(scene, first, proceedAndStore, &Path::alwaysFalse);

            nodesToProceedNext = (nodesToProceedNext.toSet() - selNodes - doneNodes).toList();
            bigListOfNodes.append(selNodes.toList());
            doneNodes += selNodes;

            foreach(PNode n, selNodes)
                doneObjects << n->object();
        }
    }

    foreach(PObject po, doneObjects)
        removeObjectAdorner(po);

    // выше все элементы списка были перестроены
    // процедура removeObjectAdorner может изменить содержание
    // списка, поэтому обнуляем его принудительно
    invalidatedAdornerObjects.clear();

    foreach(auto selNodes, bigListOfNodes)
    {
        // преобразуем в точки
        QSet<point3d> points;

        // не нужны адорнеры если представлена только одна точка
        if(selNodes.size() <= 1)
            continue;

        foreach(PNode n, selNodes)
        {
            points << n->globalPoint();

            // если это "кривой" элемент, добавляем один из его центров
            if(auto element = n->object().dynamicCast<Element>())
            {
                int ix = element->nodes().indexOf(n);
                if(!element->canBreakLine(ix))
                {
                    points << element->center(ix);
                }
            }
        }

        // создаем адорнер
        point3d first, second;
        std::tie(first, second) = getOuterPoints(points);
        if(first == second)
            continue;

        PAdorner ado = PAdorner(new StraightLineAdorner(first, second));

        QSet<PObject> pos;
        foreach(PNode node, selNodes)
            pos << node->object();
        foreach(PObject ob, pos)
        {
            mapAdorner2object.insert(ado, ob);
            mapObject2adorner.insert(ob, ado);
        }

        attachAdorner(ado);
    }
}

bool SceneImpl::updateGraphicsItem(PObjectToDraw object, int level)
{
    GItems g(&gscene);

    // восстанавалиаем предыдущие значения элементов
    // крайне необходимо для текста
    if(gitems.contains(object))
        g.prevItems = gitems.value(object, &gscene);
    if(PObjectToSelect sel = object.dynamicCast<ObjectToSelect>())
        if(gcontur.contains(sel))
            g.prevContur = gcontur.value(sel, &gscene);

    // если элементы не удалены, то это обновление
    g.update = !g.prevItems.isEmpty();

    PObjectToSelect selobject       = object.dynamicCast<ObjectToSelect>();
    bool            load_from_cache = false;

#ifdef RECALC_CACHE_PERFORMANCE_LOG
    PerformanceLogger<__LINE__> l{"updateGraphicsItem", 1};
#endif

#ifdef OLD_GITEMS_CACHE
    if(selobject && old_gitems_cache.contains(selobject))
    {
        g = old_gitems_cache.value(selobject, g);
        old_gitems_cache.remove(selobject);

        foreach(auto it, g.items)
            gscene.addItem(it.data());
        foreach(auto it, g.contur)
            gscene.addItem(it.data());
        l.waypoint(0);

        updateGraphicsItem(object, g);
        l.waypoint(1);

        load_from_cache = true;
    }
    else
    {
        // выполняет построение элементов и собирает их в Gitems
        object->draw(&gscene, g, level);
        l.waypoint(2);

        updateGraphicsItem(object, g);
        l.waypoint(3);
    }
#else
    object->draw(&gscene, g, level);
    updateGraphicsItem(object, g);
#endif

    // если изменился состав элемента, заново устанавливаем выделение
    QSet<PObjectToSelect> set;
    set << selobject;
    if(*set.begin())
        updateSelection(set);

    return load_from_cache;
}

void SceneImpl::detachGraphicsItem(PObjectToDraw object)
{
    // сохраняем данные по удалённым элементам
#ifdef OLD_GITEMS_CACHE
    if(auto selobject = object.dynamicCast<ObjectToSelect>())
    {
        GItems it{&gscene};

        it.items = gitems.value(object, it.items);
        gitems.remove(object);

        it.contur = gcontur.value(selobject, it.contur);
        gcontur.remove(selobject);

        old_gitems_generations.first() << selobject;
        old_gitems_cache.insert(selobject, it);

        foreach(auto p, it.items)
        {
            if(p->scene() != &gscene)
                qWarning() << "another scene 1";
            gscene.removeItem(p.data());
        }
        foreach(auto p, it.contur)
        {
            if(p->scene() != &gscene)
                qWarning() << "another scene 2";
            gscene.removeItem(p.data());
        }
    }
#endif
    // удаление всех кешей
    GItems g{&gscene};
    updateGraphicsItem(object, g);
}

void SceneImpl::updateGraphicsItem(PObjectToDraw object, GItems &g)
{
    // добавляем видимые элементы
    // элементы, которые раньше были здесь удаляются
    if(g.items.isEmpty())
        gitems.remove(object);
    else
        gitems.insert(object, g.items);

    // адорнеры и грипы должны располагаться выше других элементов
    if(object.dynamicCast<Adorner>() || object.dynamicCast<Grip>())
    {
        foreach(PGItem item, g.items)
        {
            // адорнеры должны находится сверху
            item->setZValue(200);
        }
    }

    // добавляем контур (если есть)
    if(PObjectToSelect sel = object.dynamicCast<ObjectToSelect>())
    {
        // в случае обновления, нужно удалить ссылки на элементы контура
        // ищем все прошлые контура данного элемента
        foreach(auto pitem, gcontur.value(sel, &gscene))
        {
            gitemToObject.remove(pitem.data());
        }

        foreach(auto pitem, g.contur)
        {
            // контуры должны располагаться под чертежем
            pitem->setZValue(-100);

            if(auto path = pitem.dynamicCast<QGraphicsPathItem>())
            {
                // контур не должен иметь границы
                path->setPen(Qt::NoPen);
            }

            // для поиска элемента по контуру
            gitemToObject[pitem.data()] = sel;
        }

        if(g.contur.isEmpty())
            gcontur.remove(sel);
        else
            gcontur.insert(sel, g.contur);
    }
}

void SceneImpl::attachAdorner(PAdorner adorner)
{
    adorners << adorner;
    updateGraphicsItem(adorner, 1);
}

void SceneImpl::detachAdorner(PAdorner adorner)
{
    adorners.removeOne(adorner);
    detachGraphicsItem(adorner);
}

void SceneImpl::attachGrip(PGrip grip)
{
    grips << grip;
    updateGraphicsItem(grip, 1);
}

void SceneImpl::detachGrip(PGrip grip)
{
    grips.removeOne(grip);
    detachGraphicsItem(grip);
}

QSet<PObject> SceneImpl::updateNeighboursGraphicsItem(PObject object)
{
    QSet<PObject> result;
    int           n = object->nodeCount();
    while(n-- > 0)
    {
        PObject other = object->neighbour(n, true);
        if(other && other->getInteraction(object) != 0)
        {
            invalidatedObjects << other;
            invalidatedCrossObjects << other;
        }
    }
    return result;
}

void SceneImpl::onAttachObject(PObject object)
{
    foreach(PNode node, object->nodes())
    {
        nodesCache.insert(node->globalPoint(), node);
    }

    invalidatedAdornerObjects << object;
    invalidatedCrossObjects << object;

    selectedObjects.remove(object);

    updateNeighboursGraphicsItem(object);
    invalidatedObjects << object;
}

void SceneImpl::removeObjectAdorner(PObject object)
{
    // удаляем всекие напоминания об объекте
    if(PAdorner a = mapObject2adorner.value(object))
    {
        detachAdorner(a);
        mapObject2adorner.remove(object);

        QList<PObject> objects = mapAdorner2object.values(a);
        foreach(PObject obj, objects)
        {
            if(obj != object)
            {
                // пересчитывать нужно только еще не удаленные объекты
                invalidatedAdornerObjects << obj;
            }
            mapAdorner2object.remove(a, obj);
        }
    }
}

void SceneImpl::onDetachObject(PObject object)
{
    object->generation *= -1;

    foreach(PNode node, object->nodes())
    {
        nodesCache.remove(node->globalPoint(), node);
    }

    detachGraphicsItem(object);

    updateNeighboursGraphicsItem(object);

    removeObjectAdorner(object);
    removeObjectCrossBase(object);
}

void SceneImpl::recalcCaches()
{
#ifdef RECALC_CACHE_PERFORMANCE_LOG
    PerformanceLogger<__LINE__> l{"recalcCaches"};
#endif

    // оценка времени работы, для принятия решения
    QElapsedTimer tm;
    tm.start();

    QSet<PObjectToSelect> pos;
    if(invalidatedObjects.size())
    {
        foreach(auto p, objects)
            pos << p;
        foreach(auto p, labels)
            pos << p;
    }

    bool fastRedraw = scene->manipulator()->tool()->isFastRedrawPossible();

#ifdef RECALC_CACHE_PERFORMANCE_LOG
    l.waypoint("draw");
#endif

    foreach(auto p, invalidatedObjects)
    {
        if(pos.contains(p))
        {
            if(fastRedraw)
            {
                bool load_from_cache = updateGraphicsItem(p, 0);
                if(!load_from_cache)
                    invalidatedObjectsDeep << p;
            }
            else
            {
                updateGraphicsItem(p, 1);
            }
        }
        else
            detachGraphicsItem(p);
    }

    invalidatedObjects.clear();

#ifdef RECALC_CACHE_PERFORMANCE_LOG
    l.waypoint("markers");
#endif
    foreach(auto marker, markers)
    {
        if(!pos.contains(marker->leader) && pos.contains(marker->follower))
        {
            updateGraphicsItem(marker->follower, 1);
        }
#ifndef NDEBUG
        if(pos.size())
        {
            if(!pos.contains(marker->leader))
            {
                auto &T = *marker->leader;
                qWarning() << "marker without leader: " << typeid(T).name();
            }
            if(!pos.contains(marker->follower))
            {
                auto &T = *marker->follower;
                qWarning() << "marker without follower" << typeid(T).name();
            }
        }
#endif
    }

#ifdef RECALC_CACHE_PERFORMANCE_LOG
    l.waypoint("adorners");
#endif
    recalculateLineAdorners();

#ifdef RECALC_CACHE_PERFORMANCE_LOG
    l.waypoint("deepCache");
#endif
    if(tm.elapsed() < 100)
        recalcDeepCaches();
}

void SceneImpl::recalcDeepCaches()
{
#ifdef RECALC_CACHE_PERFORMANCE_LOG
    PerformanceLogger<__LINE__> l{"recalcDeepCaches"};
#endif

#ifdef OLD_GITEMS_CACHE
    if(old_gitems_generations.first().size() > 1000)
    {
        old_gitems_generations.insert(0, QSet<PObjectToSelect>());

        // удаляем устаревший кэш
        if(old_gitems_generations.size() > 2)
        {
            foreach(PObjectToSelect p, old_gitems_generations.last())
                old_gitems_cache.remove(p);
            old_gitems_generations.removeLast();
        }
    }
#endif

    QElapsedTimer tm;
    tm.start();
    while(!invalidatedObjectsDeep.empty())
    {
        if(tm.elapsed() > 100)
            return;

        auto p = invalidatedObjectsDeep.takeLast();
        if(PObject obj = p.dynamicCast<Object>())
        {
            if(objects.contains(obj))
                updateGraphicsItem(p, 1);
        }
        else if(PLabel lab = p.dynamicCast<Label>())
        {
            if(labels.contains(lab))
                updateGraphicsItem(p, 1);
        }
    }

#ifdef RECALC_CACHE_PERFORMANCE_LOG
    l.waypoint("crosses");
#endif
    recalcCrosses();
}

void SceneImpl::clearOldGItemsCache()
{
    old_gitems_cache.clear();
    foreach(auto gen, old_gitems_generations)
        gen.clear();
}

double SceneImpl::epsilon() const
{
    return 10 / scale;
}

void SceneImpl::updateCursorItems()
{
    if(cursorPoint == INCORRECT_POINT)
    {
        gscene.noCursorPoint();
    }
    else
    {
        gscene.setCursorPoint(cursorPoint);
    }
}

void SceneImpl::updateSelection(QSet<PObjectToSelect> objects)
{
    foreach(PObjectToSelect object, objects)
    {
        bool       selected = selectedObjects.contains(object);
        HoverState state    = hoverObjects.value(object, HoverState::NoHover);

        makeSelected(object, state, selected);
    }

    //    emit scene->updateUI();
}

void SceneImpl::makeSelected(PObjectToSelect obj, HoverState state, bool /*selected*/)
{
    bool   visibleContur  = false;
    bool   originalColour = true;
    QBrush conturBrush;
    QBrush penBrush = QBrush(Qt::black);

    if(obj.dynamicCast<Grip>())
        return;

    if(state & HasContour)
    {
        visibleContur = true;
    }

    if(state & Newby)
    {
        originalColour = false;
        conturBrush    = graphics::newbyBrush();
        penBrush       = graphics::newbyPenBrush();
    }
    else if(state & ToBeSelected)
    {
        originalColour = false;
        conturBrush    = graphics::toBeSelectedBrush();
        penBrush       = graphics::toBeSelectedPenBrush();
    }
    else if(state & Selected)
    {
        originalColour = false;
        conturBrush    = graphics::toBeSelectedBrush();
        penBrush       = graphics::selectionPenBrush();
    }
    else if(state & HoverState::Hover)
    {
        originalColour = true;
        conturBrush    = graphics::hoverBrush();
        penBrush       = graphics::hoverPenBrush();
    }
    else if(state & HoverState::ToModify)
    {
        originalColour = false;
        conturBrush    = graphics::toModifyBrush();
        penBrush       = graphics::toModifyPenBrush();
    }

    // если контур должен быть виден, то элементам контура
    // присваивается необходимый браш
    foreach(PGItem item, gcontur.value(obj, &gscene))
    {
        if(auto contur = item.dynamicCast<QGraphicsPathItem>())
        {
            if(visibleContur)
            {
                contur->setBrush(conturBrush);
            }
            else
            {
                contur->setBrush(Qt::NoBrush);
            }
        }
    }

    // всем элементам устанаваливается заданный цвет
    foreach(PGItem item, gitems.value(obj, &gscene))
    {
        //
        if(originalColour)
        {
            penBrush = obj->originalColour();
        }

        // временная подсветка маркеров без хозяина
        // только на время отладки маркеров
#ifdef NO_MARKERS_DEBUG_MODE
        if(auto ml = obj.dynamicCast<Label>())
        {
            auto markers = markerFollower.values(ml);
            foreach(PMarker m, markers)
            {
                auto leader = m->leader.dynamicCast<Object>();
                if(!objects.contains(leader))
                    penBrush = QBrush(Qt::magenta);
            }

            if(markers.empty())
                penBrush = QBrush(Qt::magenta);
        }
#endif

        if(auto i = item.dynamicCast<GraphicsClipItem>())
        {
            i->setPenBrush(penBrush);
        }
        else if(auto i = item.dynamicCast<QGraphicsLineItem>())
        {
            auto pen = i->pen();
            pen.setBrush(penBrush);
            i->setPen(pen);
        }
        else if(auto i = item.dynamicCast<QGraphicsEllipseItem>())
        {
            auto pen = i->pen();
            pen.setBrush(penBrush);
            i->setPen(pen);
        }
        else if(auto i = item.dynamicCast<QGraphicsPathItem>())
        {
            auto pen = i->pen();
            pen.setBrush(penBrush);
            i->setPen(pen);
            auto bru = i->brush();
            bru.setColor(penBrush.color());
            i->setBrush(bru);
        }
        else if(auto i = item.dynamicCast<QGraphicsPolygonItem>())
        {
            auto pen = i->pen();
            pen.setBrush(penBrush);
            i->setPen(pen);
            auto bru = i->brush();
            bru.setColor(penBrush.color());
            i->setBrush(bru);
        }
        else if(auto i = item.dynamicCast<QGraphicsTextItem>())
        {
            i->setDefaultTextColor(penBrush.color());
        }

        item->update();
    }
}

static void closeObject_sorting(PNeighbourhood result)
{
    // сортировка меток
    QList<PLabel> labels         = result->labelInfo.keys();
    auto          sortByDistance = [&](PLabel a, PLabel b) {
        return result->labelInfo[a].distance < result->labelInfo[b].distance;
    };
    std::sort(labels.begin(), labels.end(), sortByDistance);
    if(labels.size())
    {
        result->closestLabels << labels.first();
    }

    // сортировка грипов

    QList<PGrip> grips              = result->gripInfo.keys();
    auto         sortGripByDistance = [&](PGrip a, PGrip b) {
        return result->gripInfo[a].distance < result->gripInfo[b].distance;
    };
    std::sort(grips.begin(), grips.end(), sortGripByDistance);
    if(grips.size())
    {
        result->closestGrips << grips.first();
    }

    // сортировка объектов
    QList<std::pair<double, PObject>> sorter;
    // выбираем объекты
    foreach(PObject po, result->objectInfo.keys())
        sorter << std::make_pair(result->objectInfo[po].distance, po);

    // сортируем
    std::sort(sorter.begin(), sorter.end());

    //
    QList<PObject> objs;
    foreach(auto pair, sorter)
        objs << pair.second;

    // в перую очередь джойнеры
    qStableSort(objs.begin(), objs.end(), joinersFirst);
    result->closestObjects << objs;
}

void SceneImpl::analizeCloseObject(PNeighbourhood  result,
                                   PObjectToSelect sel,
                                   point2d         b,
                                   bool            preferNodes,
                                   bool            nolimit) const
{
    if(PLabel lab = sel.dynamicCast<Label>())
    {
        double d                        = lab->distance(b);
        result->labelInfo[lab].distance = d;
        result->hoverState[lab]         = HoverState::Hover;
    }
    else if(PGrip grip = sel.dynamicCast<Grip>())
    {
        double d                        = grip->distance(b);
        result->gripInfo[grip].distance = d;
        result->hoverState[grip]        = HoverState::Hover;
    }
    else if(PObject obj = sel.dynamicCast<Object>())
    {
        if(auto joiner = qSharedPointerDynamicCast<Joiner>(obj))
        { // у Joiner`а все точки одинаковы, но необходимо учитывать швы

            int     nodeno       = 0;
            auto    ce           = obj->localPoint(0);
            double  d            = b.distance(ce);
            point3d closestPoint = obj->globalPoint(0);
            for(int index = 0; index < joiner->nodeCount(); ++index)
            {
                point2d wp = joiner->weldPoint(index) >> *worldSystem;
                double  d1 = b.distance_to_segment(ce, wp);
                if(d1 < d)
                {
                    nodeno = index;
                    d      = d1;

                    point2d cp   = b.project_to_segment(ce, wp);
                    closestPoint = worldSystem->toGlobal_atline(
                        cp, obj->globalPoint(0), joiner->weldPoint(index));
                }
            }

            if(nolimit || d < epsilon())
            {
                result->objectInfo[obj].nodeNo      = nodeno;
                result->objectInfo[obj].isNodeFound = 1;
                result->objectInfo[obj].distance    = d;
                result->objectInfo[obj].closestPoint =
                    preferNodes ? obj->globalPoint(0) : closestPoint;
                result->hoverState[obj] = HoverState::Hover;
            }
        }
        else
        {
            double  d = 0;
            point3d cpt;

            PElement elem = obj.dynamicCast<Element>();

            if(obj->nodeCount() == 2 && elem && !elem->canBreakLine(0))
            {
                // объект в три точки используем только если нет
                // других альтернатив
                d = 1e6;
                auto plane =
                    wsPlane(worldSystem, obj->globalPoint(0), obj->globalPoint(1), elem->center());
                cpt = b >> plane;
            }
            else if(obj->nodeCount() == 2)
            {
                // если 2 точки то просто находим расстояние до линии
                d = b.distance_to_line(obj->localPoint(0), obj->localPoint(1));

                // опредляем 3d точку в ближайшую к курсору
                point2d cc = obj->localPoint(0), dd = obj->localPoint(1);
                point2d crx2 = b.project_to_line(cc, dd);
                point3d c_g = obj->globalPoint(0), d_g = obj->globalPoint(1);
                auto    line = wsLine(worldSystem, c_g, d_g);
                cpt          = crx2 >> line;

                // проверяем находится ли эта точка внутри линии
                double size   = obj->size(0, 1);
                double s1     = obj->globalPoint(0).distance(cpt);
                double s2     = obj->globalPoint(1).distance(cpt);
                bool   inside = (s1 < size && s2 < size);

                if(!inside)
                {
                    d   = 1e6;
                    cpt = s1 < s2 ? obj->globalPoint(0) : obj->globalPoint(1);
                }
            }
            else if(obj->nodeCount() == 3)
            {
                // объект в три точки используем только если нет
                // других альтернатив
                d          = 1e6;
                auto plane = wsPlane(
                    worldSystem, obj->globalPoint(0), obj->globalPoint(1), obj->globalPoint(2));
                cpt = b >> plane;
            }

            // для объектов с нескольки нодами проверяем расстояние
            // до нод (кроме линии, концы которой никогда не должны
            // быть доступны (вместо них доступны джойнеры)
            int nodeno = -1;
            if(preferNodes && !obj.dynamicCast<Line>())
            {
                double d1 = 1e6;
                for(int index = 0; index < obj->nodeCount(); ++index)
                {
                    if(obj->nodeAt(index)->jointType() != 0)
                    {
                        continue;
                    }
                    double nd = b.distance(obj->localPoint(index));
                    if(nd < d1)
                    {
                        d1 = nd, nodeno = index;
                        cpt = obj->globalPoint(index);
                    }
                }
                d = d1;
            }

            // независимо от расстояния до объекта, этот объект
            // добавляется в список результатов
            result->objectInfo[obj].nodeNo       = nodeno;
            result->objectInfo[obj].isNodeFound  = (nodeno >= 0);
            result->objectInfo[obj].distance     = d;
            result->objectInfo[obj].closestPoint = cpt;
            result->hoverState[obj]              = HoverState::Hover;
        }
    }
}

PNeighbourhood SceneImpl::getClosestObjects(point2d a, point2d b, double radius) const
{
    PNeighbourhood result = PNeighbourhood(new Neighbourhood);

    // выполняем поиск контуров, на которые попал курсор
    QSet<PObjectToSelect> underCursor;

    QPainterPath path;
    if(radius <= 0)
    {
        path.moveTo(a.toQPoint());
        path.lineTo(b.toQPoint());
    }
    else
    {
        //должно отличаться
        b = a + point2d::nx;

        double k = 0.7 * radius *
                   worldSystem->convert(point3d(0)).distance(worldSystem->convert(point3d(1, 1)));
        QPointF c = a.toQPoint();
        QRectF  rect{c.x() - k, c.y() - k, 2 * k, 2 * k};
        path.addEllipse(rect);
    }

    for(int index = 2; index-- > 0;)
    {
        // используем стандартную процедуру
        QList<QGraphicsItem *> items;
        if(index)
            // сначала ищем точно
            items = a == b
                        ? gscene.items(a.x, a.y, 1, 1, Qt::IntersectsItemShape, Qt::AscendingOrder)
                        : gscene.items(path, Qt::IntersectsItemShape, Qt::AscendingOrder);
        else
            // потом ищем более широко
            items = gscene.items(
                QRect(a.x - 3, a.y - 3, 6, 6), Qt::IntersectsItemShape, Qt::AscendingOrder);

        foreach(auto item, items)
        {
            if(auto obj = gitemToObject.value(item))
            {
                underCursor << obj;
            }
        }

        if(underCursor.size())
            break;
    }

    // поиск расстояния до цели
    foreach(PObjectToSelect sel, underCursor)
    {
        bool nolimit = radius >= 0;
        analizeCloseObject(result, sel, b, true, nolimit);
    }
    closeObject_sorting(result);
    return result;
}
PNeighbourhood SceneImpl::getObjectsRectangle(point2d a, point2d b) const
{
    PNeighbourhood result = PNeighbourhood(new Neighbourhood);

    // выполняем поиск контуров, на которые попал курсор
    QList<PObjectToSelect> underCursor;

    QPainterPath path;
    path.moveTo(a.toQPoint());
    path.lineTo(b.toQPoint());

    bool exact = a.x < b.x;
    // используем стандартную процедуру
    QList<QGraphicsItem *> items =
        gscene.items(qMin(a.x, b.x),
                     qMin(a.y, b.y),
                     fabs(a.x - b.x),
                     fabs(a.y - b.y),
                     exact ? Qt::ContainsItemShape : Qt::IntersectsItemShape,
                     Qt::AscendingOrder);

    QSet<PObjectToSelect> objects;
    foreach(auto item, items)
    {
        if(auto obj = gitemToObject.value(item))
            objects << obj;
    }

    foreach(auto obj, objects)
    {
        if(auto label = obj.dynamicCast<Label>())
        {
            result->closestLabels << label;
            result->labelInfo[label];
        }
        else if(auto object = obj.dynamicCast<Object>())
        {
            result->closestObjects << object;
            result->objectInfo[object];
        }
    }

    closeObject_sorting(result);
    return result;
}

} // namespace geometry
