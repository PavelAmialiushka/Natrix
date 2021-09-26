#include "sceneprocedures.h"

#include "element.h"
#include "global.h"
#include "joiners.h"
#include "line.h"
#include "marker.h"
#include "neighbourhood.h"
#include "path.h"
#include "utilites.h"

#include "markLabels.h"
#include "textLabel.h"

#include <algorithm>

#include <QDebug>
#include <QSet>

#include "GlueObject.h"
#include "moveProcedures.h"
#include "pointProcedures.h"
#include "textGrip.h"

namespace geometry
{

bool areObjectsConnected(PObject alpha, PObject bravo)
{
    for(int ii = 0; ii < alpha->nodeCount(); ++ii)
    {
        for(int jj = 0; jj < bravo->nodeCount(); ++jj)
        {
            if(alpha->globalPoint(ii) == bravo->globalPoint(jj) &&
               alpha->direction(ii) == -bravo->direction(jj))
            {
                return true;
            }
        }
    }
    return false;
}

// если объекты касаются, то они не пытаются перекрыть друг друга
// сюда входят:
// * соседние объекты
// * линии, присоединенные к одним и тем же joiner`ам
// * элементы соединенные при помощи glue
// * джойнеры, присоединённые к одной и той же линии (нужно для отводов)
bool areObjectsTouching(Scene *scene, PObject alpha, PObject bravo)
{
    for(int ii = 0; ii < alpha->nodeCount(); ++ii)
    {
        auto cn1 = connectedNode(scene, alpha->nodeAt(ii));

        if(cn1)
        {
            // проверка на джойнеры общей линии
            for(int jj = 0; jj < bravo->nodeCount(); ++jj)
            {
                auto cn2 = connectedNode(scene, bravo->nodeAt(jj));
                if(cn2 && cn1->object() == cn2->object())
                    return true;
            }

            // проверяем на совмещение через GlueObject
            if(cn1->object().dynamicCast<GlueObject>())
            {
                cn1 = connectedNode(scene, secondNode(cn1));
                if(cn1 && cn1->object() == bravo)
                    return true;
            }
        }

        // проверяем на совпадение по точкам
        auto point = alpha->globalPoint(ii);
        for(int jj = 0; jj < bravo->nodeCount(); ++jj)
        {
            if(point == bravo->globalPoint(jj))
            {
                return true;
            }
        }
    }
    return false;
}

PNode connectedNode(Scene const *scene, PNode node)
{
    if(!node)
        return PNode();

    return scene->findConnectedNode(node).node();
}

PNode secondNode(PNode node)
{
    if(!node)
        return PNode();

    PObject object = node->object();
    if(object->nodeCount() < 2)
        return PNode();

    if(object->nodeAt(0) == node)
        return object->nodeAt(1);
    else
        return object->nodeAt(0);
}

PNode thirdNode(PNode node)
{
    if(!node)
        return PNode();

    PObject object = node->object();
    if(object->nodeCount() < 3)
        return PNode();

    if(object->nodeAt(0) == node || object->nodeAt(1) == node)
        return object->nodeAt(2);
    else
        return object->nodeAt(1);
}

PNode forthNode(PNode node)
{
    if(!node)
        return PNode();

    PObject object = node->object();
    if(object->nodeCount() < 4)
        return PNode();

    if(object->nodeAt(0) == node || object->nodeAt(1) == node || object->nodeAt(2) == node)
        return object->nodeAt(3);
    else
        return object->nodeAt(2);
}

PNode commonNode(PObject object, PObject partner)
{
    for(int ii = 0; ii < object->nodeCount(); ++ii)
    {
        for(int jj = 0; jj < partner->nodeCount(); ++jj)
        {
            if(object->globalPoint(ii) == partner->globalPoint(jj) &&
               object->direction(ii) == -partner->direction(jj))
            {
                return object->nodeAt(ii);
            }
        }
    }
    Q_ASSERT(!"do not have common nodes");
    return PNode();
}

QList<PObject> neighbours(Scene const *scene, PObject object, bool keepEmpty)
{
    QList<PObject> result;
    foreach(PNode node, object->nodes())
    {
        PNode cn = connectedNode(scene, node);
        if(cn)
            result << cn->object();
        else if(keepEmpty)
            result << PObject();
    }
    return result;
}

Command cmdModify(Scene *scene)
{
    Command cmd;
    cmd << bind(&Scene::modify, scene, 1);
    cmd >> bind(&Scene::modify, scene, -1);

    return cmd;
}

Command cmdTouchObject(Scene *scene, PObject object)
{
    Command cmd;

    cmd << bind(&Scene::invalidateObject, scene, object);
    cmd >> bind(&Scene::invalidateObject, scene, object);

    return cmd;
}

Command cmdAttachObject(Scene *scene, PObject object, PObject selectionSample)
{
    Command cmd;

    cmd << bind(&Scene::attach, scene, object);
    cmd >> bind(&Scene::detach, scene, object);

    if(scene->isSelected(selectionSample))
    {
        QSet<PObjectToSelect> group;
        group << object;
        scene->addSelectedObjects(group);
        cmd << bind(&Scene::addSelectedObjects, scene, group);
    }
    return cmd;
}

Command cmdDetachObject(Scene *scene, PObject object, bool removeMarkers)
{
    Command cmd;

    cmd << bind(&Scene::detach, scene, object);
    cmd >> bind(&Scene::attach, scene, object);

    if(removeMarkers)
    {
        auto list = scene->markersOfLeader(object);
        foreach(auto marker, list)
        {
            cmd << cmdDetachMarker(scene, marker);
        }
    }

    if(scene->isSelected(object))
    {
        QSet<PObjectToSelect> group;
        group << object;
        cmd >> bind(&Scene::addSelectedObjects, scene, group);
    }
    return cmd;
}

Command cmdReplaceObject(Scene *   scene,
                         PObject   first,
                         PObject   second,
                         MoveData *moveData,
                         bool      reconnectMarkers)
{
    Q_ASSERT(first && second);

    Command cmd;
    cmd << cmdAttachObject(scene, second, /*sample_to_select=*/first);
    cmd << cmdDetachObject(scene, first);

    if(reconnectMarkers)
        cmd << cmdReconnectMarkers(scene, first, second, moveData);
    return cmd;
}

Command cmdReplaceElement(Scene *scene, PElement element, ElementInfo opt, MoveData *moveData)
{
    Command cmd;

    auto newbyElement = element->clone(scene, opt);
    cmd << cmdReplaceObject(scene, element, newbyElement, moveData);

    return cmd;
}

Command cmdReplaceObject1to2(Scene *   scene,
                             PObject   first,
                             PObject   second1,
                             PObject   second2,
                             MoveData *moveData,
                             bool      reconnectMarkers)
{
    Q_ASSERT(first && second1 && second2);

    Command cmd;
    cmd << cmdAttachObject(scene, second1, /*sample_to_select=*/first);
    cmd << cmdAttachObject(scene, second2, /*sample_to_select=*/first);
    cmd << cmdDetachObject(scene, first);

    if(reconnectMarkers)
        cmd << cmdReconnectMarkers(scene, first, PObject(), second1, second2, moveData);
    return cmd;
}

Command cmdReplaceObject2to1(Scene *   scene,
                             PObject   first1,
                             PObject   first2,
                             PObject   second,
                             MoveData *moveData,
                             bool      reconnectMarkers)
{
    Q_ASSERT(first1 && first2 && second);

    Command cmd;
    cmd << cmdAttachObject(scene, second, /*sample_to_select=*/first1);
    cmd << cmdDetachObject(scene, first1);
    cmd << cmdDetachObject(scene, first2);

    if(reconnectMarkers)
        cmd << cmdReconnectMarkers(scene, first1, first2, second, PObject(), moveData);
    return cmd;
}

Command cmdAttachMarker(Scene *scene, PMarker marker)
{
    Q_ASSERT(marker->follower && marker->leader);

    Command cmd;
    cmd << bind(&Scene::attachMarker, scene, marker);
    cmd >> bind(&Scene::detachMarker, scene, marker);
    return cmd;
}

Command cmdDetachMarker(Scene *scene, PMarker marker)
{
    Command cmd;
    cmd << bind(&Scene::detachMarker, scene, marker);
    cmd >> bind(&Scene::attachMarker, scene, marker);
    return cmd;
}

Command cmdTryToReconnectMarker(Scene *scene, PMarker marker, QSet<PObject> objects)
{
    Command cmd;
    cmd << cmdDetachMarker(scene, marker);
    foreach(PObject object, objects)
    {
        if(object->nodeCount() == 2 &&
           marker->point.lays_on_line(object->globalPoint(0), object->globalPoint(1)))
        {
            auto newbyMarker = Marker::create(marker->point, object, marker->follower);
            cmd << cmdAttachMarker(scene, newbyMarker);

            break;
        }
    }

    return cmd;
}

Command cmdReconnectMarkers(Scene *scene, PObject orig1, PObject dest1, MoveData *moveData)
{
    return cmdReconnectMarkers(scene, orig1, PObject(), dest1, PObject(), moveData);
}

Command cmdReconnectMarkers(Scene *   scene,
                            PObject   orig1,
                            PObject   orig2,
                            PObject   dest1,
                            PObject   dest2,
                            MoveData *moveData)
{
    Command cmd;

    QList<PMarker> markers;
    markers << scene->markersOfLeader(orig1);
    markers << scene->markersOfLeader(orig2);

    if(moveData)
    {
        QList<PMarker> allMarkers;
        allMarkers << moveData->markerReplaced.keys();
        allMarkers << moveData->markerReplaced.values();

        foreach(PMarker m, allMarkers.toSet())
        {
            if(m->leader == orig1 || m->leader == orig2)
                markers << m;
        }
    }

    // удалить повторы
    markers = markers.toSet().toList();

    foreach(PMarker marker, markers)
    {
        // отключаем маркер
        cmd << cmdDetachMarker(scene, marker);

        point3d newPoint;
        PObject newDest;

        auto joiner1 = orig1.dynamicCast<Joiner>();
        if(joiner1)
        {
            auto point   = marker->point;
            int  nodeNo1 = 0;
            for(int index = 0; index < orig1->nodeCount(); ++index)
            {
                double d =
                    point.distance_to_segment(orig1->globalPoint(index), joiner1->weldPoint(index));
                if(d < 1e-6)
                {
                    nodeNo1 = index;
                    break;
                }
            }

            double f =
                point.fraction_at_section(orig1->globalPoint(nodeNo1), joiner1->weldPoint(nodeNo1));

            int nodeNo2 = -1;
            for(int index = 0; index < dest1->nodeCount(); ++index)
            {
                if(dest1->direction(index) == orig1->direction(nodeNo1))
                    nodeNo2 = index;
            }

            if(nodeNo2 == -1)
            {
                newPoint = dest1->globalPoint(0);
            }
            else
            {
                bool ok = false;
                auto c  = dest1->globalPoint(nodeNo2);
                if(dest1.dynamicCast<Joiner>())
                {
                    auto a   = dest1.dynamicCast<Joiner>()->weldPoint(nodeNo2);
                    newPoint = c.polar_to(a, c.distance(a) * f, &ok);
                    if(!ok)
                    {
                        newPoint = dest1->globalPoint(0);
                    }
                }
                else
                {
                    auto elem1 = dest1.dynamicCast<Element>();
                    if(elem1)
                        newPoint = elem1->center();
                    else
                        newPoint = dest1->globalPoint(0);
                }
            }
            newDest = dest1;
        }
        else if(orig1->nodeCount() == 3)
        {
            // объединять два объекта по три точки не умеем
            Q_ASSERT(!orig2 && !dest2);

            auto p  = marker->point >> *scene->worldSystem();
            auto r1 = orig1->localPoint(0);
            auto r2 = orig1->localPoint(1);
            auto r3 = orig1->localPoint(2);

            auto d1 = dest1->localPoint(0);
            auto d2 = dest1->localPoint(1);
            auto d3 = dest1->localPoint(2);

            auto newPoint2d = replacePointBase3to3(p, r1, r2, r3, d1, d2, d3);
            newPoint        = newPoint2d >> *scene->worldSystem();
            newDest         = dest1;
        }
        else if(orig1->nodeCount() == 2 && (!orig2 || orig2->nodeCount() == 2) &&
                dest1->nodeCount() == 2 && (!dest2 || dest2->nodeCount() == 2))
        {
            // работа с прямыми линиями
            // либо линия превращается в две
            // либо две превращаются в одну

            auto get_limits =
                [&](PObject f1, PObject f2, point3d &alfa, point3d &omega, bool &alfaIs1) -> void {
                QList<point3d> list;
                list << f1->globalPoint(0);
                list << f1->globalPoint(1);
                if(f2)
                {
                    list << f2->globalPoint(0);
                    list << f2->globalPoint(1);
                }
                qSort(list);

                alfa  = list.first();
                omega = list.last();

                alfaIs1 = (alfa == f1->globalPoint(0) || alfa == f1->globalPoint(1));
            };

            bool    ain1s;
            point3d as, os;
            get_limits(orig1, orig2, as, os, ain1s);

            // точка к которой прикреплен маркер
            point3d p = marker->point;

            // размер первого объекта
            double f1 = orig1->size(0, 1);
            // размер второго объекта
            double f2 = orig2 ? orig2->size(0, 1) : 0;

            // суммарный линейный размер двух объектов
            double fs = f1 + f2;

            double pos;
            if(ain1s) // альфа это первый объект
            {
                pos = as.distance(p);
                if(pos > f1)
                    pos = f1 + (f2 - os.distance(p));
            }
            else
            {
                pos = as.distance(p);
                if(pos > f2)
                    pos = f2 + (f1 - os.distance(p));
            }
            pos /= fs;
            pos = std::min(1.0, std::max(0., pos));

            /////////////////////////////////
            /// переходим к конечным объектам
            ///

            // размер первого объекта
            double d1 = dest1->size(0, 1);
            // размер второго объекта
            double d2 = dest2 ? dest2->size(0, 1) : 0;
            // суммарный размер двух объектов
            double ds = d1 + d2;

            bool    ain1d;
            point3d ad, od;
            get_limits(dest1, dest2, ad, od, ain1d);

            pos *= ds;
            if(pos <= (ain1d ? d1 : d2))
            {
                bool ok;
                newPoint = ad.polar_to(od, pos, &ok);
                if(!ok)
                    newPoint = ad;
                newDest = ain1d ? dest1 : dest2;
            }
            else
            {
                bool ok;
                newPoint = od.polar_to(ad, ds - pos, &ok);
                if(!ok)
                    newPoint = ad;
                newDest = ain1d ? dest2 : dest1;
            }
        }
        else
        {
            Q_ASSERT(!"incorrect (unknown) case");
        }

        auto label       = marker->follower;
        auto newbyMarker = Marker::create(newPoint, newDest, label);

        auto markerCount = scene->markersOfFollower(label).size();
        if(markerCount == 1)
        {
            // WARNING
            // если несколько линий, то может получиться, что
            // метка удаляется несколько раз

            // итак, маркер сместился. ИМХО здесь хорошее место,
            // для того, чтобы сместить еще и метку
            point3d delta = newPoint - marker->point;

            // заменяем метку
            PLabel newbyLabel     = label->clone(scene, delta);
            newbyMarker->follower = newbyLabel;

            cmd << cmdReplaceLabel(scene, label, newbyLabel, false);
        }
        else if(markerCount > 1)
        {
            point3d delta = newPoint - marker->point;

            MoveData::MarkerData data{delta, newbyMarker};
            moveData->multiLabelMarkers[marker->follower] << data;
        }

        // новый маркер
        cmd << cmdAttachMarker(scene, newbyMarker);
        cmd << cmdDetachMarker(scene, marker);

        if(moveData)
        {
            moveData->markerReplaced[marker] = newbyMarker;
        }
    }

    return cmd;
}

Command cmdAttachLabel(Scene *scene, PLabel label)
{
    Command cmd;
    cmd << bind(&Scene::attachLabel, scene, label);
    cmd >> bind(&Scene::detachLabel, scene, label);

    return cmd;
}

Command cmdDetachLabel(Scene *scene, PLabel label, bool checkMarker)
{
    Command cmd;
    cmd << bind(&Scene::detachLabel, scene, label);
    cmd >> bind(&Scene::attachLabel, scene, label);

    if(scene->isSelected(label))
    {
        QSet<PObjectToSelect> group;
        group << label;
        cmd >> bind(&Scene::addSelectedObjects, scene, group);
    }

    if(checkMarker)
    {
        QList<PMarker> markers = scene->markersOfFollower(label);
        foreach(PMarker marker, markers)
        {
            cmd << cmdDetachMarker(scene, marker);
        }
    }
    return cmd;
}

// найти прилипучий маркер
PMarker takeStickyMarker(Scene *scene, TextInfo const &info, QList<PMarker> markers)
{
    bool    stickyTextMode = false;
    PMarker stickyMarker;
    foreach(PMarker marker, markers)
    {
        PObject leader = marker->leader.dynamicCast<Object>();
        if(!leader)
            continue;
        if(leader->nodeCount() != 2)
            continue;

        if(!stickyMarker)
        {
            // нашли маркер, запомнили
            point2d markerPoint = marker->point >> *scene->worldSystem();
            if(markerPoint.distance(info.markerPoint()) < 3)
                stickyTextMode = true;

            stickyMarker = marker;
        }
        else
        {
            // если маркеров несколько, то что-то тут не так, ничего не возвращаем
            stickyTextMode = false;
            stickyMarker.reset();
            break;
        }
    }

    if(!stickyTextMode)
        return PMarker();
    return stickyMarker;
}

Command cmdReplaceLabel(Scene *scene,
                        PLabel label1,
                        PLabel label2,
                        bool   checkMarker,
                        bool   dropMarker)
{
    Command cmd;

    cmd << cmdDetachLabel(scene, label1, 0);
    cmd << cmdAttachLabel(scene, label2);

    if(scene->isSelected(label1))
    {
        QSet<PObjectToSelect> group;
        group << label2;
        cmd << bind(&Scene::addSelectedObjects, scene, group);
    }

    if(checkMarker || dropMarker)
    {
        QList<PMarker> markers = scene->markersOfFollower(label1);
        foreach(PMarker marker, markers)
        {
            cmd << cmdDetachMarker(scene, marker);

            if(checkMarker)
            {
                auto newby = Marker::create(marker->point, marker->leader, label2);
                cmd << cmdAttachMarker(scene, newby);
            }
        }
    }

    auto mlabel1 = label1.dynamicCast<MarkLabel>();
    auto mlabel2 = label2.dynamicCast<MarkLabel>();
    if(mlabel1 && mlabel2)
    {
        // подчинённые маркеры
        QList<PMarker> markers = scene->markersOfLeader(label1);
        foreach(PMarker marker, markers)
        {
            cmd << cmdDetachMarker(scene, marker);

            point3d point = mlabel1->transformPoint(marker->point, mlabel2);
            auto    delta = scene->worldSystem()->convert(point - marker->point);

            auto newbyLabel = marker->follower->clone(scene, delta);
            cmd << cmdReplaceLabel(scene, marker->follower, newbyLabel, false);

            auto newbyMarker = Marker::create(point, mlabel2, newbyLabel);
            cmd << cmdAttachMarker(scene, newbyMarker);
        }
    }

    return cmd;
}

double toDegrees(point2d d)
{
    double angle = d.angle();
    angle        = fmod((angle * 180 / M_PI + 360), 360);
    while(angle > 89)
        angle -= 180;
    while(angle < -91)
        angle += 180;
    return angle;
}

Command cmdAttachLabelToObject(Scene * scene,
                               PLabel  label,
                               PLabel  newBy,
                               PObject object,
                               point3d point)
{
    Command cmd;
    point3d direction = object->direction(0);

    auto markerList = scene->markersOfFollower(label);
    foreach(PMarker marker, markerList)
    {
        cmd << cmdDetachMarker(scene, marker);
    }

    // вставляем новый маркер
    auto marker = Marker::create(point, object, newBy);
    cmd << cmdAttachMarker(scene, marker);

    if(auto text = newBy.dynamicCast<TextLabel>())
        attachTextLabelTo(scene, text, point, direction);

    return cmd;
}

static int objectStatus(PObject a)
{
    return a.dynamicCast<Element>() ? 0 : a.dynamicCast<Line>() ? 1 : 2;
}

void EraseData::addToReplace(PObject old, PObject newbie)
{
    toReplace << old;
    replaceMap.insert(old, newbie);
}

void EraseData::addToReplace(QList<PObject> olds, PObject newbie)
{
    foreach(auto old, olds)
        addToReplace(old, newbie);
}

void EraseData::addToReplace(QSet<PObject> olds, PObject newbie)
{
    foreach(auto old, olds)
        addToReplace(old, newbie);
}

bool linesFirst(PObject a, PObject b)
{
    return objectStatus(a) < objectStatus(b);
}

static void scanForAliveElements(Scene *       scene,
                                 PObject       object,
                                 QSet<PObject> deleteQueue,
                                 EraseData &   data)
{
    auto atTheSameLine = [&](NodePathInfo const &info) -> bool {
        point3d fst;
        if(info.route.size() > 1 && info.route[0]->object() == object)
        {
            // если первый элемент наш объект, то второй элемент - ориентир
            fst = info.route[1]->direction();
        }
        else if(info.route.size() > 0 && info.route[0]->object() != object)
        {
            // если первый элемент не наш объект, то он и есть ориентир
            fst = info.route[0]->direction();
        }
        else
            // в остальных случаях ориенира нет
            return true;

        // тройники отбрасываем
        if(info.node->object().dynamicCast<TeeJoiner>())
            return false;

        return fst.isParallel(info.node->direction());
    };

    // поиск заканчивается, когда находится живой объект (не джойнер)
    //
    auto searchAliveObject = [&](NodePathInfo const &info) {
        bool alive = !data.toDelete.contains(info.node->object()) &&
                     !deleteQueue.contains(info.node->object());
        return alive && !info.node->object().dynamicCast<Joiner>() && info.node->object() != object;
    };

    if(!data.toReplace.contains(object))
    {
        Path a = Path::makePathSimple(scene, object->nodeAt(0), atTheSameLine, searchAliveObject);
        Routes const &rs = a.routes();

        bool addEndCups = true;

        // найдено не менее двух концов
        if(rs.size() == 2)
        {
            PNode   node1   = rs[0].last();
            PNode   node2   = rs[1].last();
            PObject object1 = node1->object();
            PObject object2 = node2->object();

            bool isElem1 = !object1.dynamicCast<Line>();
            bool isElem2 = !object2.dynamicCast<Line>();

            // если по крайней мере с одного конца не элемент
            // и если концы направлены навстречу друг другу
            if(!(isElem1 && isElem2) && node1->direction() == -node2->direction())
            {
                // опрделяем самые удаленные точки
                point3d p1 = isElem1 ? node1->globalPoint() : secondNode(node1)->globalPoint();
                point3d p2 = isElem2 ? node2->globalPoint() : secondNode(node2)->globalPoint();

                QSet<PObject> localReplaceSet;
                // помечаем эти элементы для удаления
                if(!isElem1)
                    localReplaceSet << object1;
                if(!isElem2)
                    localReplaceSet << object2;

                // все элементы, которые будут здесь удалены помечаем
                foreach(Route const &r, rs)
                {
                    foreach(PNode node, r)
                    {
                        PObject object = node->object();
                        if(object != object1 && object != object2)
                        {
                            localReplaceSet << object;
                        }
                    }
                }

                // создаем новую линию на место удаленных элементов
                // TODO: может быть можно нечто по-лучше
                auto line = Line::create(scene, p1, p2, Object::NoSample);
                data.addToReplace(localReplaceSet, line);

                // заменяем элементы на самих себя,
                // чтобы обновить их на чертеже
                if(isElem1)
                    data.toTouch.insert(object1);
                if(isElem2)
                    data.toTouch.insert(object2);

                // если удаляются элементы но сохраняется линия
                // удаляем маркеры
                foreach(auto it, localReplaceSet)
                {
                    if(it.dynamicCast<Element>())
                    {
                        auto markers = scene->markersOfLeader(it);
                        foreach(PMarker marker, markers)
                        {
                            data.toEraseMarkers << marker;
                        }
                    }
                }

                addEndCups = false;
            }
        }

        // концы должны быть обработаны
        if(addEndCups)
        {
            foreach(auto route, rs)
            {
                PNode   endNode = route.last();
                PObject object1 = endNode->object();
                if(object1.dynamicCast<Line>())
                {
                    point3d p1 = endNode->globalPoint();
                    point3d d1 = endNode->direction();
                    data.toAdd << EndCupJoiner::create(scene, p1, -d1);
                }
            }
        }
    }
}

void eraseObjectsToSelect(Scene *scene, EraseData &data, QSet<PObjectToSelect> srcDeleteQueue)
{
    QSet<PObject> objectDeleteQueue;
    // сортируем отдельно объекты и метки
    foreach(PObjectToSelect sel, srcDeleteQueue)
        if(auto obj = sel.dynamicCast<Object>())
            objectDeleteQueue << obj;
        else if(auto lab = sel.dynamicCast<Label>())
            data.labelsToDelete << lab;

    while(objectDeleteQueue.size())
    {
        // выбираем сначала линии потом все остальное
        PObject object =
            *std::min_element(objectDeleteQueue.begin(), objectDeleteQueue.end(), linesFirst);
        objectDeleteQueue.remove(object);

        // выбираем соседей элемента
        auto neibsTotal = neighbours(scene, object);

        // отбираем из них "живых"
        QList<PObject> neibsAlive;
        foreach(PObject ob, neibsTotal)
            if(!data.toDelete.contains(ob) && !objectDeleteQueue.contains(ob))
                neibsAlive << ob;

        auto sizeAlive = neibsAlive.size();

        if(object.dynamicCast<Line>())
        {
            // если удаляется линия, то обязательно
            // удаляются джойнеры с концов линии
            foreach(PObject ob, neibsAlive)
                if(ob.dynamicCast<Joiner>())
                    objectDeleteQueue << ob;
        }
        else if(object.dynamicCast<GlueObject>())
        {
            // ничего не делаем
        }
        else if(object.dynamicCast<Element>())
        {
            // обязательно удаляются glue объекты
            foreach(PObject ob, neibsAlive)
                if(ob.dynamicCast<GlueObject>())
                {
                    objectDeleteQueue << ob;
                    auto ns = neighbours(scene, ob);
                    foreach(PObject el, ns)
                    {
                        if(!data.toDelete.contains(el))
                            data.toTouch << el;
                    }
                }

            // если это элемент, выполняем поиск линий
            // которые могли бы замкнуться на месте элемента

            // подходят только линии,которые совпадают по направлению
            // с элементом
            scanForAliveElements(scene, object, objectDeleteQueue, data);
        }
        else if(object.dynamicCast<EndCupJoiner>())
        {
            if(sizeAlive == 1)
                data.toSave << object;
        }
        else if(object.dynamicCast<BendJoiner>())
        {
            if(sizeAlive == 2)
            {
                data.toSave << object;
            }
            else if(sizeAlive == 1)
            {
                auto node  = commonNode(object, neibsAlive[0]);
                auto newby = EndCupJoiner::create(scene, node->globalPoint(), node->direction());

                data.addToReplace(object, newby);
            }
        }
        else if(object.dynamicCast<WeldJoiner>())
        {
            if(sizeAlive == 2)
            {
                // находим дальние концы линии
                point3d a, b;
                mostDistantPoints(neibsAlive[0], neibsAlive[1], a, b);

                // TODO: найти образец для копирования стиля
                PObject line = Line::create(scene, a, b, Object::NoSample);
                data.addToReplace(neibsAlive.toSet(), line);
                data.addToReplace(object, line);
            }
            else if(sizeAlive == 1)
            {
                auto node  = commonNode(object, neibsAlive[0]);
                auto newby = EndCupJoiner::create(scene, node->globalPoint(), node->direction());
                data.addToReplace(object, newby);
            }
        }
        else if(object.dynamicCast<TeeJoiner>())
        {
            if(sizeAlive == 3)
                data.toSave << object;
            else if(sizeAlive == 2)
            {
                auto node1 = commonNode(object, neibsAlive[0]);
                auto node2 = commonNode(object, neibsAlive[1]);
                if(node1->direction() == -node2->direction())
                {
                    // находим дальние концы линии
                    point3d a, b;
                    mostDistantPoints(neibsAlive[0], neibsAlive[1], a, b);

                    // TODO: найти образец для копирования
                    PObject line = Line::create(scene, a, b, Object::NoSample);

                    data.addToReplace(neibsAlive.toSet(), line);
                    data.addToReplace(object, line);
                }
                else
                {
                    auto newby = BendJoiner::create(scene,
                                                    node1->globalPoint(),
                                                    node1->direction(),
                                                    node2->direction(),
                                                    Object::NoSample
#ifdef _COPY_JOINER_WELD_POSITION
                                                    ,
                                                    node1->weldPosition(),
                                                    node2->weldPosition()
#endif
                    );
                    data.addToReplace(object, newby);
                }
            }
            else if(sizeAlive == 1)
            {
                auto node  = commonNode(object, neibsAlive[0]);
                auto newby = EndCupJoiner::create(scene, node->globalPoint(), node->direction());

                data.addToReplace(object, newby);
            }
        }
        else if(object.dynamicCast<FourJoiner>())
        {
            if(sizeAlive == 4)
                data.toSave << object;
            else if(sizeAlive == 3)
            {
                auto node1 = commonNode(object, neibsAlive[0]);
                auto node2 = commonNode(object, neibsAlive[1]);
                auto node3 = commonNode(object, neibsAlive[2]);

                if(node1->direction() == -node2->direction())
                    qSwap(node2, node3);
                else if(node2->direction() == -node3->direction())
                    qSwap(node1, node2);
                else
                {
                } // всё уже ок

                auto newby = TeeJoiner::create(scene,
                                               node1->globalPoint(),
                                               node1->direction(),
                                               node2->direction(),
                                               Object::NoSample);
                data.addToReplace(object, newby);
            }
            else if(sizeAlive == 2)
            {
                auto node1 = commonNode(object, neibsAlive[0]);
                auto node2 = commonNode(object, neibsAlive[1]);

                if(node1->direction() == -node2->direction())
                { // на одной прямой

                    // находим дальние концы линии
                    point3d a, b;
                    mostDistantPoints(neibsAlive[0], neibsAlive[1], a, b);

                    // TODO: найти образец для копирования
                    PObject line = Line::create(scene, a, b, Object::NoSample);

                    data.addToReplace(neibsAlive.toSet(), line);
                    data.addToReplace(object, line);
                }
                else
                {

                    auto newby = BendJoiner::create(scene,
                                                    node1->globalPoint(),
                                                    node1->direction(),
                                                    node2->direction(),
                                                    Object::NoSample
#ifdef _COPY_JOINER_WELD_POSITION
                                                    ,
                                                    node1->weldPosition(),
                                                    node2->weldPosition()
#endif
                    );
                    data.addToReplace(object, newby);
                }
            }
            else if(sizeAlive == 1)
            {
                auto node  = commonNode(object, neibsAlive[0]);
                auto newby = EndCupJoiner::create(scene, node->globalPoint(), node->direction());

                data.addToReplace(object, newby);
            }
        }

        if(!data.toSave.contains(object) && !data.toReplace.contains(object))
            data.toDelete << object;
    }

    // разруливаем маркеры
    foreach(PObject object, data.toDelete)
    {
        // это не джойнер, замененный на другой
        // значит просто удаляем маркеры
        auto markers = scene->markersOfLeader(object);
        foreach(PMarker marker, markers)
        {
            data.toEraseMarkers << marker;
        }
    }
    // список маркеров, которые удаляются по причине
    foreach(PLabel lab, data.labelsToDelete)
    {
        data.toEraseMarkers << scene->markersOfFollower(lab);
        data.toEraseMarkers << scene->markersOfLeader(lab);
    }
    foreach(PObject object, data.toReplace)
    {
        // вместо элемента была вставлена линия
        PObject line = data.replaceMap.value(object);

        // подменяем маркеры
        auto markers = scene->markersOfLeader(object);
        foreach(PMarker marker, markers)
        {
            if(!data.toEraseMarkers.contains(marker))
            {
                auto newbyMarker = Marker::create(marker->point, line, marker->follower);
                data.toReconnectMarkers << marker;
                data.markerReplaceMap[marker] = newbyMarker;
            }
        }
    }

    data.toTouch -= data.toDelete;
    data.toTouch -= data.toReplace;
}

Command doErase(Scene *scene, EraseData &data)
{
    Command cmd;

    // проверка корректности
    foreach(PObject object, data.toDelete)
        Q_ASSERT(!data.toReplace.contains(object));
    foreach(PObject object, data.toReplace)
        Q_ASSERT(!data.toDelete.contains(object));
    foreach(PMarker marker, data.toEraseMarkers)
        Q_ASSERT(!data.toReconnectMarkers.contains(marker));

    foreach(PObject object, data.toDelete)
    {
        cmd << cmdDetachObject(scene, object);
    }

    // реверсируем
    QMultiMap<PObject, PObject> newbiesMap;
    foreach(PObject old, data.replaceMap.keys())
        foreach(PObject newbie, data.replaceMap.values(old))
            newbiesMap.insert(newbie, old);

    foreach(PObject newbyObject, newbiesMap.keys().toSet())
    {
        auto olds = newbiesMap.values(newbyObject);
        if(olds.size() == 1)
        {
            cmd << cmdReplaceObject(scene, olds[0], newbyObject, 0, false);
        }
        else if(olds.size() == 2)
        {
            cmd << cmdReplaceObject2to1(scene, olds[0], olds[1], newbyObject, 0, false);
        }
        else
        {
            foreach(auto it, olds)
                cmd << cmdDetachObject(scene, it);
            cmd << cmdAttachObject(scene, newbyObject);
        }
    }

    // добавляем объекты, которые добавляем
    foreach(PObject object, data.toAdd)
    {
        cmd << cmdAttachObject(scene, object);
    }

    foreach(PMarker m, data.toReconnectMarkers)
    {
        cmd << cmdDetachMarker(scene, m);
        cmd << cmdAttachMarker(scene, data.markerReplaceMap[m]);
    }

    foreach(PObject object, data.toTouch)
    {
        cmd << cmdDetachObject(scene, object);
        cmd << cmdAttachObject(scene, object);
    }

    // отсоединяем все маркеры
    Command preCmd;
    foreach(PMarker marker, data.toEraseMarkers.toSet())
    {
        preCmd << cmdDetachMarker(scene, marker);
    }
    // вставляем удаление маркеров перед всеми другими удалениями
    cmd = preCmd << cmd;

    // удаляем метки
    foreach(PLabel lab, data.labelsToDelete)
    {
        // удаляем метки
        cmd << cmdDetachLabel(scene, lab);
    }

    return cmd;
}

Command cmdEraseObjectsToSelect(Scene *scene, QSet<PObjectToSelect> selObjects)
{
    EraseData data;
    eraseObjectsToSelect(scene, data, selObjects);

    // удаляем объекты
    Command cmd = doErase(scene, data);
    return cmd;
}

std::tuple<point3d, point3d> breakOrShortLineBy(point3d p1,
                                                point3d p2,
                                                point3d start,
                                                double  size,
                                                bool    acceptSinglePart,
                                                bool *  ok)
{
    point3d el1     = p1;
    point3d el2     = el1;
    bool    failure = false;

    // может получиться 2 отрезка
    bool placeForTwo = (p1.distance(p2) >= size + lineMinimumSize * 2);
    if((!placeForTwo && !acceptSinglePart) || p1.distance(p2) < size + lineMinimumSize)
    {
        // не проходит
        failure = true;
    }
    else if(acceptSinglePart && p1.distance(start) < size)
    {
        // пытаемся выдавить из точки p1
        bool lok1;
        el1 = p1;
        el2 = p1.polar_to(p2, size, &lok1);
        if(!lok1)
            failure = true;
        if(el2.distance(p2) < lineMinimumSize - POINT_PREC)
            failure = true;
    }
    else if(acceptSinglePart && p2.distance(start) < size)
    {
        // пытаемся выдавить из точки p2
        bool lok1;
        el1 = p2.polar_to(p1, size, &lok1);
        el2 = p2;
        if(!lok1)
            failure = true;
        if(el1.distance(p1) < lineMinimumSize - POINT_PREC)
            failure = true;
    }
    else if(p1 != start && p1.distance(start) - size / 2 < lineMinimumSize)
    {
        // места мало, точка старт ближе к р1
        bool lok1, lok2;
        el1 = p1.polar_to(p2, lineMinimumSize, &lok1);
        el2 = el1.polar_to(p2, size, &lok2);
        if(el2.distance(p2) < lineMinimumSize - POINT_PREC)
            failure = true;
        else if(!lok1 || !lok2)
            failure = true;
    }
    else if(p2 != start && p2.distance(start) - size / 2 < lineMinimumSize)
    {
        // места мало, точка start ближе к p2
        bool lok1, lok2;
        el1 = p2.polar_to(p1, lineMinimumSize + size, &lok1);
        el2 = el1.polar_to(p2, size, &lok2);
        if(el1.distance(p1) < lineMinimumSize - POINT_PREC)
            failure = true;
        else if(!lok1 || !lok2)
            failure = true;
    }
    else
    {
        // места навалом
        if(p1 != start)
        { // рисуем начиная от точки p1
            bool lok1, lok2;
            el1 = start.polar_to(p1, size / 2, &lok1);
            el2 = el1.polar_to(p2, size, &lok2);
            if(el2.distance(p2) < lineMinimumSize - POINT_PREC)
                failure = true;
            if(!lok1 || !lok2)
                failure = true;
        }
        else if(p2 != start)
        {
            // рисуем начиная от точки p2
            bool lok1, lok2;
            el2 = start.polar_to(p2, size / 2, &lok1);
            el1 = el2.polar_to(p1, size, &lok2);
            if(!lok1 || !lok2)
                failure = true;
            if(el1.distance(p1) < lineMinimumSize - POINT_PREC)
                failure = true;
        }
        else
        {
            // непонятно как так вышло,
            // но блокируем дальнейшее создание
            failure = true;
            el1     = p1;
            Q_ASSERT(!"unforeseen consequences");
        }
    }

    if(el1.distance(el2) < size - POINT_PREC)
        failure = true;
    if(!acceptSinglePart && qMin(p1.distance(el1), p2.distance(el2)) < lineMinimumSize - POINT_PREC)
        failure = true;

    if(ok)
        *ok = !failure;

    return std::make_tuple(el1, el2);
}

std::tuple<point3d, point3d> breakLineBy(point3d p1,
                                         point3d p2,
                                         point3d start,
                                         double  size,
                                         bool *  ok)
{
    return breakOrShortLineBy(p1, p2, start, size, false, ok);
}

geometry::Command cmdPlaceLabelAndMarker(Scene *scene, int type, PObject object, point3d point)
{
    auto slabel = new MarkLabel(scene, point, 1.0, 1.0);
    slabel->setType(type);

    PLabel  label(slabel);
    PMarker marker = Marker::create(point, object, label);

    Command cmd;
    cmd << cmdAttachLabel(scene, label);
    cmd << cmdAttachMarker(scene, marker);
    return cmd;
}

void mostDistantPoints(PObject left, PObject right, point3d &a, point3d &b)
{
    if(left->globalPoint(0).distance(right->globalPoint(0)) >
       left->globalPoint(0).distance(right->globalPoint(1)))
    {
        b = right->globalPoint(0);
    }
    else
    {
        b = right->globalPoint(1);
    }

    if(right->globalPoint(0).distance(left->globalPoint(0)) >
       right->globalPoint(0).distance(left->globalPoint(1)))
    {
        a = left->globalPoint(0);
    }
    else
    {
        a = left->globalPoint(1);
    }
}

bool canMergeLines(PObject a, PObject b)
{
    auto linea = a.dynamicCast<Line>();
    auto lineb = b.dynamicCast<Line>();

    if(linea && lineb && linea->lineStyle() == lineb->lineStyle())
        return true;

    return false;
}

point3d getMarkLandingDirection(PObject object)
{
    // определяем есть ли другие направления
    auto nodes = object->nodes();

    // предпочитаем направления в горизонтальной плоскости
    auto rank = [](PNode a) {
        if(a->direction().isNormal(point3d(0, 0, 1)))
            return 2;
        return 1;
    };
    std::sort(nodes.begin(), nodes.end(), [&](PNode a, PNode b) { return rank(a) > rank(b); });

    return -nodes.first()->direction();
}

Command cmdMatchStyles(Scene *scene, QSet<PObjectToSelect> selObjects, ScenePropertyList values)
{
    Command cmd;

    foreach(PObjectToSelect s, selObjects)
    {
        if(PObject object = s.dynamicCast<Object>())
        {
            PObject newby = object->cloneMove(scene, 0);

            bool s = false;
            foreach(auto v, values)
                if(newby->applyStyle(v))
                    s = true;

            if(s)
                cmd << cmdReplaceObject(scene, object, newby);
        }

        if(PTextLabel label = s.dynamicCast<TextLabel>())
        {
            PTextLabel newby = label->clone(scene, 0).dynamicCast<TextLabel>();

            bool s = false;
            foreach(auto v, values)
                if(newby->applyStyle(v))
                    s = true;

            if(s)
                cmd << cmdReplaceLabel(scene, label, newby);
        }
    }

    return cmd;
}

void pushTextGrips(Scene *scene, QList<PMarker> list)
{
    foreach(PMarker marker, list)
    {
        if(marker->follower.dynamicCast<TextLabel>())
        {
            auto      point = marker->point >> *scene->worldSystem();
            PTextGrip grid(new TextGrip(point, marker));
            scene->pushGrip(grid);
        }
    }
}

} // namespace geometry
