#include "manipulatorLabelRotate.h"
#include "label.h"
#include "manipulator.h"
#include "manipulatorLabelStart.h"
#include "manipulatorTools.h"
#include "moveProcedures.h"
#include "nodePointAdorner.h"

#include "global.h"

#include "moveAdorner.h"
#include "sceneProcedures.h"

#include "scales.h"
#include <algorithm>

namespace geometry
{

RotateLabel::RotateLabel(Manipulator *m, PMarkLabel Label)
    : ManipulatorTool(m)
    , label_(Label)
    , fixedByDirection_(0)
    , activeNodeNo_(0)
    , markSpectialDirection_(false)
    , inAnyDirection_(0)
    , rotateOverObject_(0)
{
    joinToPrevious_ = true;
}

RotateLabel *RotateLabel::inAnyDirection(Manipulator *m, PMarkLabel e)
{
    auto tool   = new RotateLabel(m, e);
    tool->mode_ = Mode::OverCenterFree;

    tool->inAnyDirection_ = true;
    return tool;
}

RotateLabel *RotateLabel::flipAlongDirection(Manipulator *m, PMarkLabel e, point3d dir)
{
    auto tool   = new RotateLabel(m, e);
    tool->mode_ = Mode::Flip;

    tool->fixedByDirection_ = true;
    tool->fixingDirection_  = dir;

    return tool;
}

RotateLabel *RotateLabel::rotateOverObject(Manipulator *m, PMarkLabel e, PObject object)
{
    auto tool   = new RotateLabel(m, e);
    tool->mode_ = Mode::Flip;

    tool->rotateOverObject_ = true;
    tool->baseObject_       = object;

    return tool;
}

void RotateLabel::setMarkSpecialDirection()
{
    markSpectialDirection_ = true;
}

void RotateLabel::setPrevTool(PManipulatorTool t)
{
    prevTool_ = t;
}

QString RotateLabel::helperText() const
{
    return QString::fromUtf8("<b>Выберите направление</b>, куда будет направлен элемент") +
           helperTab + ManipulatorTool::helperText();
}

void RotateLabel::do_prepare()
{
    auto type = MarkLabel::typeFromInt(label_->type());
    nextTool_ = prevTool_ ? prevTool_ : PManipulatorTool(new LabelStart(manipulator_, type));
}

void RotateLabel::do_click(point2d clicked, PNeighbourhood n)
{
    // точка, вокруг которой будем вращать
    point3d rotateCenter = 0;
    auto    markers      = scene_->markersOfFollower(label_);
    if(markers.empty())
        rotateCenter = scene_->worldSystem()->convert(label_->basePoint());
    else
        rotateCenter = markers.first()->point;

    // выбираем все свободные направления
    QList<point3d> variants = generateDirections();
    if(fixedByDirection_)
    {
        variants.clear();
        variants << fixingDirection_;
        variants << -fixingDirection_;
    }
    else if(rotateOverObject_ && baseObject_)
    {
        // оставляем только непарарлельные направления
        variants = selectMarkLabelNormalDirections(baseObject_, variants);
    }

    // выбор направления
    bool    success;
    point3d result_end;
    std::tie(result_end, success) = selectDirection(variants, rotateCenter, clicked, 1);
    if(!success)
        return;
    point3d cursorDirection = (result_end - rotateCenter).normalized();

    point3d lineDirection = label_->lineDirection();
    if(inAnyDirection_)
    {
        lineDirection = variants.first();
    }

    PMarkLabel newby   = label_->clone(scene_, point3d{}, cursorDirection);
    auto       lineDir = calcLineDirectionOfMarkLabel(baseObject_, cursorDirection);
    newby->setLineDirection(lineDir);

    Command cmd;
    cmd << cmdReplaceLabel(scene_, label_, newby);
    commandList_.addAndExecute(cmd);

    nextTool_.reset(new LabelStart(manipulator_, label_->typeFromInt(label_->type())));
    setCursor("");

    if(markSpectialDirection_ && cursorDirection == fixingDirection_)
        n->hoverState[newby] = HoverState::ToBeSelected;
    else
        n->hoverState[newby] = HoverState::Newby;

    scene_->pushAdorner(new NodePointAdorner(rotateCenter, AdornerLineFromNone));
    scene_->pushAdorner(new MoveAdorner(rotateCenter, result_end));
}

void RotateLabel::do_changeSize(int sizeF, bool absolute)
{
}

QList<point3d> selectMarkLabelNormalDirections(PObject object, QList<point3d> variants)
{
    QList<point3d> result;
    bool           isElem = object.dynamicCast<Element>();
    foreach(point3d x, variants)
    {
        bool ok = true;
        if(!isElem)
        {
            foreach(PNode node, object->nodes())
                if(x == node->direction())
                    ok = false;
        }
        else
        {
            ok = false;
            foreach(PNode node, object->nodes())
                if(x == node->direction() && !connectedNode(object->scene(), node))
                    ok = true;
        }
        if(ok)
            result << x;
    }
    return result;
}

point3d calcLineDirectionOfMarkLabel(PObject object, point3d dir)
{
    bool isElem = object.dynamicCast<Element>();

    if(!isElem)
    {
        // для отводов выбираем направление вдоль одной из осей
        QList<point3d> dirs;
        if(object)
            foreach(PNode node, object->nodes())
                dirs << node->direction();
        while(dirs.size())
        {
            if(!dir.isParallel(dirs.first()))
                return dirs.first();
            dirs.takeFirst();
        }
    }

    return !dir.isParallel(point3d::ny) ? point3d::ny : point3d::nx;
}

} // namespace geometry
