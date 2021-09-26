#include "moveProcedures.h"
#include "global.h"
#include "label.h"
#include "line.h"
#include "path.h"
#include "sceneProcedures.h"

#include "ghostAdorner.h"
#include "glueObject.h"
#include "moveAdorner.h"
#include "nodePointAdorner.h"

#include "element.h"
#include "joiners.h"

#include <tuple>

#include <QDebug>
#include <QFile>
#include <QMap>
#include <QSet>
#include <QStringList>
#include <qalgorithms.h>

#include "dispatcher.h"

#include "moveProceduresData.h"

namespace geometry
{

PObject MoveData::replacedPObject(PObject original) const
{
    while(autoReplaced.contains(original))
        original = autoReplaced.value(original);
    return original;
}

PObject MoveData::originalOfPObject(PObject sample) const
{
    while(1)
    {
        PObject back = autoReplaced.key(sample);
        if(!back)
            break;
        sample = back;
    }
    return sample;
}

PMarker MoveData::replacedMarker(PMarker m) const
{
    while(markerReplaced.contains(m))
        m = markerReplaced[m];

    return m;
}

PNode MoveData::originalOf(PNode rNode) const
{
    PObject r   = rNode->object();
    PObject orx = originalOfPObject(r);
    if(!orx)
        return PNode();
    return orx->nodeAt(r->nodes().indexOf(rNode));
}

Command cmdShiftObject(Scene *scene, MoveData &data, bool *success)
{
    Command       cmd;
    QSet<PObject> moved;
    point3d       delta = data.destination - data.startPoint;

    // сдвигаем каждый объект контура на заданное растояние
    auto moveByDelta = [&](NodePathInfo const &info) -> bool {
        PObject object = info.node->object();
        if(!moved.contains(object))
        {
            moved << object;

            PObject newby = object->cloneMove(scene, delta);
            cmd << cmdReplaceObject(scene, object, newby, &data);

            if(!data.adornerCache.contains(object))
                data.adornerCache[object].reset(new GhostAdorner(object));
            scene->pushAdorner(data.adornerCache[object]);

            data.autoReplaced[object] = newby;
        }
        return true;
    };

    PObject start = data.object;
    Path::walkContour(scene, start->nodeAt(0), moveByDelta);
    if(success)
        *success = true;

    // корректируем положение меток, которые прилеплены к нескольким местам
    cmd << cmdMoveMultiMarkerLabels(scene, data);

    return cmd;
}

Command cmdMoveMultiMarkerLabels(Scene *scene, MoveData &data)
{
    Command cmd;

    foreach(PLabel label, data.multiLabelMarkers.keys())
    {
        auto    markers = scene->markersOfFollower(label);
        int     count   = markers.count();
        point3d delta   = 0;
        foreach(MoveData::MarkerData m, data.multiLabelMarkers[label])
        {
            delta += m.delta;
        }

        if(count > 1)
            delta = delta / count;

        // сдвигаем метку
        PLabel newby = label->clone(scene, delta);
        cmd << cmdReplaceLabel(scene, label, newby, false);

        foreach(PMarker m, markers)
        {
            bool found = false;
            foreach(MoveData::MarkerData md, data.multiLabelMarkers[label])
            {
                if(data.replacedMarker(m) == md.marker)
                {
                    found = true;
                }
            }
            if(found)
            {
                data.replacedMarker(m)->follower = newby;
                continue;
            }
            auto newbyMarker = Marker::create(m->point, m->leader, newby);
            cmd << cmdAttachMarker(scene, newbyMarker);
            cmd << cmdDetachMarker(scene, m);
            data.markerReplaced[m] = newbyMarker;
        }
    }

    data.multiLabelMarkers.clear();

    return cmd;
}

static Command moveGraph2Command(Scene *scene, MoveGraph graph, MoveData &data, bool &ok)
{
    Command       cmd;
    QSet<PObject> moved;
    foreach(PEdge edge, graph.edges)
    {
        point3d a = graph.vertexData[edge->start].delta;
        point3d b = graph.vertexData[edge->finish].delta;
        if(!a.empty() || !b.empty())
        {
            PObject edgeObject = edge->object;
            PObject newby;

            // не расчитываем, что объект согласован
            if(graph.vertexOf[edgeObject->nodeAt(0)] == edge->finish)
                qSwap(a, b);

            if(edgeObject.dynamicCast<Line>())
            {
                point3d p1    = edgeObject->globalPoint(0) + a;
                point3d p2    = edgeObject->globalPoint(1) + b;
                double  delta = p1.distance(p2);
                if(delta < lineMinimumSize - POINT_PREC)
                {
                    qDebug() << "line is too short";
                    ok = false;
                    return Command();
                }
                point3d n1 = (p1 - p2).normalized();
                point3d n2 = (edgeObject->globalPoint(0) - edgeObject->globalPoint(1)).normalized();
                if(n1 != n2)
                {
                    qDebug() << "line changed direction";
                    ok = false;
                    return Command();
                }

                newby = Line::create(scene, p1, p2, edgeObject);
            }
            else if(auto elem = edgeObject.dynamicCast<Element>())
                newby = elem->cloneMoveResize(scene, a, b);
            else
                Q_ASSERT(!"error");

            cmd << cmdReplaceObject(scene, edgeObject, newby, &data);

            if(!data.adornerCache.contains(edgeObject))
                data.adornerCache[edgeObject].reset(new GhostAdorner(edgeObject));
            //            scene->pushAdorner( data.adornerCache[edgeObject] );

            data.autoReplaced[edgeObject] = newby;
            moved << edgeObject;
        }
    }

    foreach(PVertex vertex, graph.vertexes)
    {
        foreach(PNode node, vertex->nodes)
        {
            PObject object = node->object();
            point3d delta  = graph.vertexData[vertex].delta;
            if(!moved.contains(object) && !delta.empty())
            {
                PObject newby = object->cloneMove(scene, delta);

                cmd << cmdReplaceObject(scene, object, newby, &data);

                if(!data.adornerCache.contains(object))
                    data.adornerCache[object].reset(new GhostAdorner(object));
                //                scene->pushAdorner( data.adornerCache[object] );

                data.autoReplaced[object] = newby;
                moved << object;
            }
        }
    }

    // корректируем положение меток, которые прилеплены к нескольким местам
    cmd << cmdMoveMultiMarkerLabels(scene, data);

    return cmd;
}

Command cmdMoveObject(Scene *scene, MoveData &data, bool *success)
{
    // двигаем объекты входящие в контур
    Q_ASSERT(scene->contains(data.object));
    data.autoReplaced.clear();

    // параметры перемещения
    MoveParameters params{ObjectRestrictionType::Elastic};

    point3d startDelta      = data.destination - data.startPoint;
    PNode   start           = data.object->nodeAt(0);
    params.nodeMoves[start] = NodeMovement{0};

    // фиксируем заданные объекты
    QList<PObject> fixedObjects = data.fixedObjects;
    //    fixedObjects << data.object;
    foreach(PObject object, fixedObjects)
    {
        params.objectRules[object].rule = ObjectRestrictionType::Fixed;
    }

    // создаем граф перемещений
    bool      ok    = true;
    MoveGraph graph = MoveGraph::create(scene, params, ok);

    // здесь выполняются все расчеты так, чтобы величины delta по каждой из вершин
    // стали такими как положено
    if(ok)
        ok = graph.solve_MoveNode_simple(start, startDelta);

    // проверка на валидность перемещения
    //    if (ok) ok = checkMoveGraph(graph);

    // если что рапортуем об неудаче
    if(success)
        *success = ok;
    if(!ok)
        return Command();

    Command cmd = moveGraph2Command(scene, graph, data, ok);
    if(!ok)
        return Command();

    insertAdorners(scene, data);
    return cmd;
}

bool isSameContur(Scene *scene, PObject start, PObject end)
{
    bool result        = false;
    auto searchEndNode = [&](NodePathInfo const &info) -> bool {
        if(info.node->object() == end)
        {
            result = true;
            return false;
        }
        return true;
    };
    Path::walkContour(scene, start->nodeAt(0), searchEndNode);
    return result;
}

bool isSameContur(Scene *scene, PObject start, PObject end, MoveParameters &param)
{
    bool result        = false;
    auto searchEndNode = [&](NodePathInfo const &info) -> bool {
        if(info.node->object() == end)
        {
            result = true;
            return false;
        }
        return true;
    };
    Path::walkContour(scene, start->nodeAt(0), searchEndNode);
    return result;
}

Command cmdMoveConnectObject(Scene *scene, MoveData &data, PObject landObject, bool *success)
{
    Q_ASSERT(scene->contains(landObject));

    bool canMove;
    data.fixedObjects << landObject;
    Command precmd;

    // определяем величину предварительной коррекции если это возможно
    bool theSameContur = isSameContur(scene, data.object, landObject);
    if(!theSameContur)
    {
        precmd << cmdShiftObject(scene, data, &canMove);
        Q_ASSERT(scene->contains(landObject));
    }
    else
    {
        precmd = cmdMoveObject(scene, data, &canMove);
        Q_ASSERT(scene->contains(landObject));
    }

    Command cmd;
    if(!canMove)
    {
        auto t = data.object.dynamicCast<Joiner>() ? AdornerMovingFromNode : AdornerMovingFromLine;
        scene->pushAdorner(new NodePointAdorner(data.destination, t));
    }
    else
    {
        PObject moveObject = data.replaced(data.object);
        Q_ASSERT(scene->contains(landObject));

        // двойная диспетчеризация
        utils::Dispatcher<bool, PObject, PObject> Disp;

        //////////////////////////////////////////////////////////////////
        //
        auto c2c = [&](PEndCupJoiner moveCup, PEndCupJoiner landCup) -> bool {
            if(moveCup->direction(0) == -landCup->direction(0))
            {
                auto list1 = neighbours(scene, data.originalOf(moveCup));
                auto list2 = neighbours(scene, data.originalOf(landCup));

                if(list1.isEmpty() || list2.isEmpty())
                    return false;

                PObject line1 = data.replaced(list1.first());
                PObject line2 = data.replaced(list2.first());

                if(canMergeLines(line1, line2))
                {
                    point3d a, b;
                    mostDistantPoints(line1, line2, a, b);

                    PObject newbyLine = Line::create(scene, a, b, line1);
                    cmd << cmdReplaceObject2to1(scene, line1, line2, newbyLine, &data);
                    cmd << cmdDetachObject(scene, moveCup);
                    cmd << cmdDetachObject(scene, landCup);
                }
                else
                {
                    auto newby = WeldJoiner::create(
                        scene, moveCup->globalPoint(0), moveCup->direction(0), moveCup);
                    cmd << cmdReplaceObject2to1(scene, moveCup, landCup, newby, &data);
                }

                scene->pushAdorner(new NodePointAdorner(data.destination, AdornerLineFromNode));
                return true;
            }
            else if(moveCup->direction(0) != landCup->direction(0))
            {
                PObject newby = BendJoiner::create(scene,
                                                   landCup->globalPoint(0),
                                                   landCup->direction(0),
                                                   moveCup->direction(0),
                                                   Object::NoSample);

                newby->applyStyle(data.lineInfo);
                newby->applyStyle(moveCup);

                cmd << cmdReplaceObject2to1(scene, moveCup, landCup, newby, &data);
                scene->pushAdorner(new NodePointAdorner(data.destination, AdornerLineFromNode));
                return true;
            }
            return false;
        };
        Disp.addHandler<PEndCupJoiner, PEndCupJoiner>(c2c);

        //////////////////////////////////////////////////////////////////
        //
        auto c2b = [&](PEndCupJoiner moveCup, PBendJoiner landBend) -> bool {
            // отвод
            if(moveCup->direction(0) == -landBend->direction(0))
            {
                PObject newby = TeeJoiner::create(scene,
                                                  landBend->globalPoint(0),
                                                  landBend->direction(0),
                                                  landBend->direction(1),
                                                  Object::NoSample
#ifdef _COPY_JOINER_WELD_POSITION
                                                  ,
                                                  landBend->weldPosition(0),
                                                  -1,
                                                  landBend->weldPosition(1)
#endif
                );

                newby->applyStyle(data.lineInfo);
                newby->applyStyle(landBend);

                cmd << cmdReplaceObject2to1(scene, moveCup, landBend, newby, &data);
                scene->pushAdorner(new NodePointAdorner(data.destination, AdornerLineFromNode));
                return true;
            }
            else if(moveCup->direction(0) == -landBend->direction(1))
            {
                PObject newby = TeeJoiner::create(scene,
                                                  landBend->globalPoint(0),
                                                  landBend->direction(1),
                                                  landBend->direction(0),
                                                  Object::NoSample
#ifdef _COPY_JOINER_WELD_POSITION
                                                  ,
                                                  landBend->weldPosition(1),
                                                  -1,
                                                  landBend->weldPosition(0)
#endif
                );

                newby->applyStyle(data.lineInfo);
                newby->applyStyle(landBend);

                cmd << cmdReplaceObject2to1(scene, moveCup, landBend, newby, &data);
                scene->pushAdorner(new NodePointAdorner(data.destination, AdornerLineFromNode));
                return true;
            }
            return false;
        };
        Disp.addHandler<PEndCupJoiner, PBendJoiner>(c2b);

        //////////////////////////////////////////////////////////////////
        //
        auto c2t = [&](PEndCupJoiner moveCup, PTeeJoiner landTee) -> bool {
            // готовим четверник
            if(moveCup->direction(0) == -landTee->direction(2))
            {
                PObject newby = FourJoiner::create(scene,
                                                   landTee->globalPoint(0),
                                                   landTee->direction(0),
                                                   landTee->direction(2),
                                                   Object::NoSample
#ifdef _COPY_JOINER_WELD_POSITION
                                                   ,
                                                   landBend->weldPosition(0),
                                                   -1,
                                                   landBend->weldPosition(1)
#endif
                );

                newby->applyStyle(data.lineInfo);
                newby->applyStyle(landTee);

                cmd << cmdReplaceObject2to1(scene, moveCup, landTee, newby, &data);
                scene->pushAdorner(new NodePointAdorner(data.destination, AdornerLineFromNode));
                return true;
            }
            return false;
        };
        Disp.addHandler<PEndCupJoiner, PTeeJoiner>(c2t);
        //////////////////////////////////////////////////////////////////
        //
        auto c2w = [&](PEndCupJoiner moveCup, PWeldJoiner landWeld) -> bool {
            // отвод
            if(!moveCup->direction(0).isParallel(landWeld->direction(0)))
            {
                PObject newby = TeeJoiner::create(scene,
                                                  landWeld->globalPoint(0),
                                                  landWeld->direction(0),
                                                  moveCup->direction(0),
                                                  Object::NoSample);

                newby->applyStyle(data.lineInfo);
                newby->applyStyle(landWeld);

                cmd << cmdReplaceObject2to1(scene, moveCup, landWeld, newby, &data);
                scene->pushAdorner(new NodePointAdorner(data.destination, AdornerLineFromNode));
                return true;
            }
            return false;
        };
        Disp.addHandler<PEndCupJoiner, PWeldJoiner>(c2w);

        //////////////////////////////////////////////////////////////////
        //
        auto c2l = [&](PEndCupJoiner moveCup, PLine landLine) -> bool {
            // середина линии присоединяется только если не параллельны
            if(!moveCup->direction(0).isParallel(landLine->direction(0)))
            {
                point3d p1 = landLine->globalPoint(0), p2 = landLine->globalPoint(1), e1, e2;
                bool    canBreak = false;

                // удаётся ли разделить линию так, чтобы концы были не слишком коротки
                if(data.originalOf(moveCup) == data.object)
                {
                    // если двигается свободный конец
                    std::tie(e1, e2) = breakLineBy(p1, p2, data.destination, 0.0, &canBreak);
                    if(!canBreak)
                        return false;

                    // переигрываем движение до уточненной точки
                    data.destination = e1;
                }
                else
                { // двигается линия, значит движение линии должно быть скоректировано
                    // таким образом, чтобы точка разрыва была там, где нужно

                    // если двигается свободный конец
                    std::tie(e1, e2) = breakLineBy(p1, p2, data.startPoint, 0.0, &canBreak);
                    if(!canBreak)
                        return false;

                    data.startPoint = e1;
                    point3d delta   = data.destination - data.startPoint;
                    e1 += delta;
                    e2 += delta;
                }

                // возвращаем исходные значения
                landLine = data.originalOf(landLine);
                moveCup  = data.originalOf(moveCup);

                Command move_cmd;
                move_cmd = theSameContur ? cmdMoveObject(scene, data, &canMove)
                                         : cmdShiftObject(scene, data, &canMove);
                if(!canMove)
                    return false;
                precmd = move_cmd;

                // ищем конечные значения
                landLine = data.replaced(landLine);
                moveCup  = data.replaced(moveCup);

                PObject line1 = Line::create(scene, p1, e1, landLine);
                PObject line2 = Line::create(scene, e2, p2, landLine);
                PObject tee   = TeeJoiner::create(
                    scene, e1, landLine->direction(0), moveCup->direction(0), Object::NoSample);
                tee->applyStyle(data.lineInfo);
                tee->applyStyle(landLine);

                if(!line1 || !line2)
                    return false;

                cmd << cmdReplaceObject(scene, moveCup, tee, &data);
                cmd << cmdReplaceObject1to2(scene, landLine, line1, line2, &data);

                scene->pushAdorner(new NodePointAdorner(e1, AdornerLineFromNode));
                return true;
            }
            return false;
        };
        Disp.addHandler<PEndCupJoiner, PLine>(c2l);

        //////////////////////////////////////////////////////////////////
        //
        auto c2e = [&](PEndCupJoiner moveCup, PElement landElement) -> bool {
            foreach(PNode node, landElement->nodes())
            {
                if(moveCup->direction(0) != node->direction() ||
                   moveCup->globalPoint(0) != node->globalPoint())
                    // не то направление или не совпадает вообще
                    continue;

                if(connectedNode(scene, data.originalOf(node)))
                    // уже занято
                    continue;

                cmd << cmdDetachObject(scene, moveCup);

                scene->pushAdorner(new NodePointAdorner(data.destination, AdornerLineFromNode));
                return true;
            }
            return false;
        };
        Disp.addHandler<PEndCupJoiner, PElement>(c2e);

        //////////////////////////////////////////////////////////////////
        //
        auto e2e = [&](PElement moveElement, PElement landElement) -> bool {
            // присоединять объект к самому себе не стоит
            if(moveElement == landElement)
                return false;

            double sizeDefect = getSizeDefect(moveElement, landElement);

            foreach(PNode landNode, landElement->nodes())
            {
                foreach(PNode moveNode, moveElement->nodes())
                {
                    if(moveNode->direction() == -landNode->direction() &&
                       moveNode->globalPoint() == landNode->globalPoint())
                    {
                        auto movePoint = landNode->globalPoint();
                        if(sizeDefect > 0)
                        {
                            auto landPointDefect =
                                movePoint.polar_to(movePoint - landNode->direction(), sizeDefect);
                            auto glue = GlueObject::create(scene, movePoint, landPointDefect);

                            data.destination = landPointDefect;

                            bool    canMove = false;
                            Command move_cmd;
                            move_cmd = theSameContur ? cmdMoveObject(scene, data, &canMove)
                                                     : cmdShiftObject(scene, data, &canMove);
                            if(!canMove)
                                return false;
                            precmd = move_cmd;

                            cmd << cmdAttachObject(scene, glue);
                        }

                        scene->pushAdorner(
                            new NodePointAdorner(data.destination, AdornerLineFromNode));
                        return true;
                    }
                }
            }
            return false;
        };
        Disp.addHandler<PElement, PElement>(e2e);

        //////////////////////////////////////////////////////////////////
        //
        auto o2o = [&](PObject, PObject) -> bool { return false; };
        Disp.addHandler<PObject, PObject>(o2o);

        if(Disp(moveObject, landObject) || Disp(landObject, moveObject))
        {
            scene->pushAdorner(new MoveAdorner(data.startPoint, data.destination));
            if(success)
                *success = true;
            precmd << cmd;
            return precmd;
        }
    }

    if(success)
        *success = 0;
    return Command();
}

QStringList MoveGraph::toStringList() const
{
    QStringList result;

    result << "~~~~~~~~~~~~~~~~~~~~";

    foreach(PVertex vertex, vertexes)
    {
        result << QString("Vertex: %1").arg(vertex->name);

        QStringList conns;
        foreach(PEdge edge, vertex->edges)
            conns << (edge->start == vertex ? edge->finish : edge->start)->name;

        if(vertexData[vertex].fixed)
            result << "Fixed";
        result << QString("Connected to: ") + conns.join(", ");
        result << QString("Delta: %1").arg(vertexData[vertex].delta.toQString());

        QSet<PObject> objects;
        foreach(PNode node, vertex->nodes)
            objects << node->object();

        foreach(PObject object, objects)
            foreach(PNode node, vertex->nodes)
                if(object == node->object())
                {
                    result << QString("> object %1 node #%2 at %3")
                                  .arg(object->typeName())
                                  .arg(object->nodes().indexOf(node))
                                  .arg(node->globalPoint().toQString());
                }
    }
    return result;
}

Command cmdMoveNodes(Scene *scene, MoveData &data, MoveParameters &params, bool *success)
{
    data.autoReplaced.clear();

    Q_ASSERT(params.nodeMoves.size());
    if(params.nodeMoves.empty())
    {
        *success = false;
        return Command();
    }

    // фиксируем заданные объекты
    QList<PObject> fixedObjects = data.fixedObjects;
    fixedObjects << data.object;
    foreach(PObject object, fixedObjects)
    {
        params.objectRules[object].rule = ObjectRestrictionType::Fixed;
    }

    // создаем граф перемещений
    bool      ok    = true;
    MoveGraph graph = MoveGraph::create(scene, params, ok);

    // здесь выполняются все расчеты так, чтобы величины delta по каждой из вершин
    // стали такими как положено
    if(ok)
        ok = graph.solve_MoveNode_advanced();

    // если что рапортуем об неудаче
    if(success)
        *success = ok;
    if(!ok)
        return Command();

    Command cmd = moveGraph2Command(scene, graph, data, ok);
    if(success)
        *success = ok;
    if(!ok)
        return Command();

    return cmd;
}

void insertAdorners(Scene *scene, MoveData &data)
{
    foreach(PObjectToSelect object, data.adornerCache.keys())
    {
        scene->pushAdorner(data.adornerCache[object]);
    }
}

} // namespace geometry
