#include "manipulatorWeldMove.h"
#include "manipulatorWeldStart.h"
#include "neighbourhood.h"
#include "nodePointAdorner.h"

#include "global.h"
#include "sceneProcedures.h"
#include "weldProcedures.h"

#include "bendJoiner.h"
#include "line.h"
#include "manipulator.h"
#include "teeJoiner.h"
#include "weldJoiner.h"

namespace geometry
{

QString WeldMove::helperText() const
{
    return QString::fromUtf8("Укажите место куда <b>переместить шов</b>") + helperTab +
           QString::fromUtf8("Либо сдвигайте его в сторону, чтобы удалить") + helperTab +
           QString::fromUtf8("<b>Ctrl</b> чтобы разделить два шва");
}

WeldMove::WeldMove(Manipulator *m, QList<WeldInfo> infos)
    : ManipulatorTool(m)
    , infos_(infos)
{
    Q_ASSERT(infos.size());
    supportModes_ |= DragToClick;
}

PManipulatorTool WeldMove::create(Manipulator *m, QList<WeldInfo> infos)
{
    return PManipulatorTool(new WeldMove(m, infos));
}

Command WeldMove::cmdRemoveWeldJoiner(PObject weldObject)
{
    Command cmd;

    auto line  = connectedNode(scene_, weldObject->nodeAt(0))->object();
    auto line2 = connectedNode(scene_, weldObject->nodeAt(1))->object();

    auto outerPoint1 = secondNode(commonNode(line, weldObject))->globalPoint();
    auto outerPoint2 = secondNode(commonNode(line2, weldObject))->globalPoint();

    auto newbyLine = Line::create(scene_, outerPoint1, outerPoint2, line2);
    cmd << cmdReplaceObject2to1(scene_, line, line2, newbyLine);
    cmd << cmdDetachObject(scene_, weldObject);

    QSet<PObject> added;
    added << newbyLine;

    // маркер, принадлежащий точно шву должен быть переназначен
    auto lostMarkers = scene_->markersOfLeader(weldObject);
    foreach(auto marker, lostMarkers)
        cmd << cmdTryToReconnectMarker(scene_, marker, added);

    return cmd;
}

Command WeldMove::moveWeld(point2d point, PNeighbourhood nei)
{
    Command cmd;
    foreach(WeldInfo winfo, infos_)
    {
        PObject weldObject = winfo.object;

        auto lines = neighbours(scene_, weldObject);
        foreach(auto obj, lines)
            scene_->analizeCloseObject(nei, obj, point, false);

        // выбираем линию под курсором
        auto byDistance = [&](PObject a, PObject b) -> bool {
            return nei->objectInfo[a].distance < nei->objectInfo[b].distance;
        };
        std::sort(lines.begin(), lines.end(), byDistance);

        if(!lines.empty())
        {
            auto line = lines.takeFirst();
            auto node = commonNode(weldObject, line);

            auto line2 =
                secondNode(node) ? connectedNode(scene_, secondNode(node))->object() : PObject();
            auto outerPoint1 = secondNode(commonNode(line, weldObject))->globalPoint();
            auto outerPoint2 = secondNode(commonNode(line2, weldObject))->globalPoint();

            // определяем остальные швы, которые находятся на данной линии
            auto welds        = object2welds(scene_, QList<PObject>() << line).values();
            auto isWeldObject = [&](WeldInfo const &info) { return info.object == weldObject; };
            welds.erase(std::remove_if(welds.begin(), welds.end(), isWeldObject), welds.end());

            if(auto weld = weldObject.dynamicCast<WeldJoiner>())
            {
                double d        = nei->objectInfo[line].distance;
                bool   doRemove = d >= scene_->epsilon();

                // удаление и вставка
                auto insertPoint = nei->objectInfo[line].closestPoint;
                auto newbyLine1  = Line::create(scene_, outerPoint1, insertPoint, line);
                auto newbyLine2  = Line::create(scene_, outerPoint2, insertPoint, line2);

                if(!newbyLine1 || !newbyLine2)
                    doRemove = true;

                if(doRemove)
                {
                    // только удаление
                    cmd << cmdRemoveWeldJoiner(weldObject);
                }
                else
                {
                    cmd << cmdReplaceObject(scene_, line, newbyLine1);
                    cmd << cmdReplaceObject(scene_, line2, newbyLine2);

                    auto newbyWeld = WeldJoiner::create(
                        scene_, insertPoint, weldObject->direction(0), weldObject);
                    cmd << cmdReplaceObject(scene_, weldObject, newbyWeld);

                    scene_->pushAdorner(new NodePointAdorner(insertPoint, AdornerMovingWeld));
                }

                // после этого перемещения уже больше ничего не пытаемся сделать
                return cmd;
            }
            else if(weldObject.dynamicCast<BendJoiner>() || weldObject.dynamicCast<TeeJoiner>())
            {
                auto info        = scene_->analizeCloseObject(line, point, false);
                auto insertPoint = info.closestPoint;

                // расстояния до угла отвода/тройника
                double node_dist = weldObject->globalPoint(0).distance(info.closestPoint);

                // тут только выбираем какой шов перемещать, для случая, когда у нас несколько швов
                bool moveOnlyOne = manipulator_->getMode(ControlKey);
                if(moveOnlyOne)
                {
                    bool isShrinking = (weldObject->globalPoint(0).distance(insertPoint) <
                                        weldObject->globalPoint(0).distance(winfo.globalPoint));
                    if(!isShrinking)
                        continue;
                }

                PObject weldToJoin;
                // сравниваем с расстоянием до других швов
                foreach(WeldInfo wi, welds)
                {
                    // пропускаем швы, которые учавствуют в перемещении
                    bool ignore_wi = false;
                    foreach(WeldInfo lhs, infos_)
                        if(lhs.object == wi.object)
                            ignore_wi = true;
                    if(ignore_wi)
                        continue;

                    // расстояние до чужого шва
                    double wi_dist = weldObject->globalPoint(0).distance(wi.globalPoint);

                    // проехали чужой шов, а значит нам сюда нельзя
                    // либо если слишком близко к чужому шву
                    if(node_dist > wi_dist ||
                       wi.globalPoint.distance(insertPoint) < lineMinimumSize / 2)
                    {
                        // перемещаем шов на шов
                        weldToJoin  = wi.object;
                        insertPoint = wi.globalPoint;
                    }
                }

                double d = insertPoint.distance(weldObject->globalPoint(0));

                // если слишком далеко от линии
                bool doRemove = info.distance >= scene_->epsilon();
                if(doRemove)
                    d = 0;

                if(!doRemove && weldToJoin.dynamicCast<WeldJoiner>())
                {
                    cmd << cmdRemoveWeldJoiner(weldToJoin);
                }

                auto clone = weldObject->cloneMove(scene_, 0);
                clone.dynamicCast<Joiner>()->setWeldPosition(winfo.nodeIndex, d);
                cmd << cmdReplaceObject(scene_, weldObject, clone);
                nei->hoverState[clone] = Hover;

                if(!doRemove)
                    scene_->pushAdorner(new NodePointAdorner(insertPoint, AdornerMovingWeld));
            }
        }
    }
    return cmd;
}

void WeldMove::do_prepare()
{
    nextTool_ = WeldStart::create(manipulator_);
}

void WeldMove::do_click(point2d point, PNeighbourhood nei)
{
    nextTool_ = WeldStart::create(manipulator_);

    Command cmd = moveWeld(point, nei);
    commandList_.addAndExecute(cmd);
}

} // namespace geometry
