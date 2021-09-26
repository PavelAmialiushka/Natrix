#include "manipulatorLabelStart.h"

#include "manipulatorLabelRotate.h"
#include "manipulatorMoveLabel.h"
#include "markLabels.h"
#include "neighbourhood.h"
#include "sceneProcedures.h"
#include "textLabel.h"
#include "utilites.h"

namespace geometry
{

LabelStart::LabelStart(Manipulator *m, QString elementType)
    : ManipulatorTool(m)
    , elementType_(elementType)
{
    supportModes_ = DragToClick | Clicking;
}

PManipulatorTool LabelStart::create(Manipulator *m, QString elementType)
{
    return PManipulatorTool(new LabelStart(m, elementType));
}

void LabelStart::do_toggleMode()
{
    static QString names = "Support,SupportSpring,Clip,ClipSpring,,"
                           "ConSupport,ConSupportSpring,ConClip,ConClipSpring,,"
                           "Wall,Underground,,"
                           "NosslePlug,NossleValve,NossleTI,NosslePI,,"
                           "Arrow";
    static QStringList items = names.split(",");

    QString name = elementType_;
    QString siblingName;
    int     index = items.indexOf(name);
    if(index == -1)
        siblingName = name;
    else
    {
        if(++index == items.size() || items[index].isEmpty())
        {
            while(index != 0 && !items[index - 1].isEmpty())
                --index;
        }

        siblingName = items[index];
    }

    elementType_ = siblingName;
}

QString LabelStart::helperText() const
{
    return QString::fromUtf8("<b>Чтобы создать объект:</b> Укажите место, куда его вставить") +
           helperTab + QString::fromUtf8("Кликните на существующий объект чтобы сдвинуть его") +
           helperTab +
           QString::fromUtf8("Перетащите существующую метку для изменения ее положения");
}

namespace
{
auto isNotText = [&](PLabel a) { return !a.dynamicCast<TextLabel>(); };
}

void LabelStart::do_drag(point2d pt, PNeighbourhood nei, bool started)
{
    auto labels = utils::filtered(nei->closestLabels, isNotText);
    if(labels.size())
    {
        auto label = labels.first();
        nextTool_  = PManipulatorTool(new MoveLabel(manipulator_, pt, label));
        commit();
    }
}

void LabelStart::do_click(point2d pt, PNeighbourhood n)
{
    auto labels = utils::filtered(n->closestLabels, isNotText);
    int  type   = MarkLabel::typeToInt(elementType_);

    Command cmd;
    if(labels.size())
    {
        // двигаем метку под курсором

        auto label           = labels.first();
        nextTool_            = MoveLabel::create(manipulator_, pt, label);
        n->hoverState[label] = HoverState::ToBeSelected;
        setCursor("");
    }
    else if(n->closestObjects.size())
    {
        // вставляем рядом с объектом

        PObject object  = n->closestObjects[0];
        point3d point   = n->objectInfo[object].closestPoint;
        point3d lineDir = getMarkLandingDirection(object);
        point3d dir     = lineDir;

        if(MarkLabel::rotationType(type) == NormalRotation)
        {
            auto vars = generateDirections();
            vars      = selectMarkLabelNormalDirections(object, vars);
            dir       = vars.size() ? vars.first() : point3d::nx;
            lineDir   = calcLineDirectionOfMarkLabel(object, dir);
        }

        // создаем метку
        PMarkLabel label{new MarkLabel(scene_, point, dir, lineDir)};
        label->setType(type);

        // создаем маркер
        PMarker marker = Marker::create(point, object, label);

        cmd << cmdAttachLabel(scene_, label);
        cmd << cmdAttachMarker(scene_, marker);

        n->hoverState[label] = HoverState::Newby;

        int rtype = label->rotationType();
        if(rtype == InLineRotation)
        {
            nextTool_.reset(RotateLabel::flipAlongDirection(manipulator_, label, dir));
        }
        else if(rtype == NormalRotation)
        {
            nextTool_.reset(RotateLabel::rotateOverObject(manipulator_, label, object));
        }

        setCursor("");
    }
    else
    {
        // вставляем на свободном месте

        point3d    point = pt >> *scene_->worldSystem();
        PMarkLabel label{new MarkLabel(scene_, point, 1.0, bestNormalDirection(1.0))};
        label->setType(type);

        cmd << cmdAttachLabel(scene_, label);

        n->hoverState[label] = HoverState::Newby;

        if(label->rotationType())
            nextTool_.reset(RotateLabel::inAnyDirection(manipulator_, label));
        setCursor("");
    }

    commandList_.addAndExecute(cmd);
}

} // namespace geometry
