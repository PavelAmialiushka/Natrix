#include "manipulatorMoveStart.h"
#include "manipulatorCanvasMove.h"
#include "manipulatorMoveContinue.h"
#include "manipulatorMoveLabel.h"
#include "manipulatorTools.h"
#include "moveProcedures.h"
#include "neighbourhood.h"
#include "nodePointAdorner.h"

#include "canvasRectangle.h"
#include "joiner.h"
#include "markLabels.h"

namespace geometry
{

QString MoveStart::helperText() const
{
    return QString::fromUtf8("<b>Перемещение:</b> выберите элемент, который хотите переместить");
}
MoveStart::MoveStart(Manipulator *m)
    : ManipulatorTool(m)
{
}

PManipulatorTool MoveStart::create(Manipulator *m)
{
    return PManipulatorTool(new MoveStart(m));
}

void MoveStart::do_click(point2d pt, PNeighbourhood n)
{
    bool                    label  = false;
    bool                    object = true;
    QList<PLabel>           labels;
    QList<PCanvasRectangle> canvases;
    foreach(PLabel l, n->closestLabels)
    {
        if(auto c = l.dynamicCast<CanvasRectangle>())
        {
            canvases << c;
        }
        else
        {
            labels << l;
            label = true;
        }
    }

    if(n->closestObjects.empty())
        object = false;

    if(!label && !object && canvases.empty())
    { // под мышкой ничего нет
        setCursor("stop");
        return;
    }
    else if(!label && !object)
    {
        setCursor("move");
        nextTool_ = CanvasMove::create(manipulator_, pt, canvases.first());
        return;
    }

    if(label && object)
    {
        double dla = n->labelInfo[labels.first()].distance;
        double dob = n->objectInfo[n->closestObjects.first()].distance;

        if(dob >= dla)
            label = 0;
        else
            object = 0;
    }

    PLabel firstLabel;
    if(labels.size())
        firstLabel = labels.first();

    if(!object)
    {
        // пытаемся найти метку, которая присоединяется к объекту
        for(PLabel label; labels.size();)
        {
            label = labels.takeFirst();
            foreach(PMarker marker, scene_->markersOfFollower(label))
            {
                if(PObject object = marker->leader.dynamicCast<Object>())
                {
                    return doMoveObject(object, pt >> *scene_->worldSystem());
                }
                else if(PLabel label = marker->leader.dynamicCast<Label>())
                {
                    // метка, которая указывает на другую метку
                    labels << label;
                }
            }
        }
    }

    if(object)
    {
        PObject    object = n->closestObjects.first();
        ObjectInfo info   = n->objectInfo[object];
        doMoveObject(object, info.closestPoint);
    }
    else if(firstLabel)
    {
        nextTool_ = PManipulatorTool(new MoveLabel(manipulator_, pt, firstLabel));
        setCursor("size");
    }
}

void MoveStart::doMoveObject(PObject object, point3d startPoint)
{
    MoveDataInit data;
    data.object     = object;
    data.startPoint = startPoint;
    data.lineInfo   = scene_->defaultLineInfo();
    nextTool_       = MoveContinue::create(manipulator_, data);

    // adorner
    auto t = object.dynamicCast<Joiner>() ? AdornerMovingFromNode : AdornerMovingFromLine;
    scene_->pushAdorner(new NodePointAdorner(data.startPoint, t));
    setCursor("size");
}

} // namespace geometry
