#include "manipulatorTextStart.h"
#include "manipulatorMoveLabel.h"
#include "manipulatorTextEdit.h"
#include "manipulatorTextLine.h"
#include "manipulatorTextLine2.h"

#include "element.h"
#include "markLabels.h"
#include "nodepointadorner.h"
#include "sceneprocedures.h"
#include "textlabel.h"
#include "utilites.h"

#include <QDebug>

namespace geometry
{

TextStart::TextStart(Manipulator *m)
    : ManipulatorTool(m)
{
    supportModes_ = Dragging | Clicking;
}

PManipulatorTool TextStart::create(Manipulator *m)
{
    return PManipulatorTool(new TextStart(m));
}

void TextStart::do_setUp()
{
    pushTextGrips(scene_, scene_->markers());
}

void TextStart::do_rollback()
{
    scene_->clearGrips();
    pushTextGrips(scene_, scene_->markers());
}

QString TextStart::helperText() const
{
    return QString::fromUtf8(
               "<b>Чтобы создать текстовую метку:</b> Укажите место, куда вставить новый текст") +
           helperTab + QString::fromUtf8("Кликните на существующей метку чтобы редактировать ее") +
           helperTab +
           QString::fromUtf8("Перетащите существующую метку для изменения ее положения") +
           helperTab + QString::fromUtf8("Перетащите начало линии для изменения ее положения");
}

void TextStart::do_prepare()
{
}

void TextStart::do_drag(point2d pt, PNeighbourhood nei, bool started)
{
    auto grips       = utils::filter_transform<PTextGrip>(nei->closestGrips, toTextGrip);
    bool canTakeGrid = manipulator_->getMode(ControlKey) == 0;
    if(grips.size() && started && canTakeGrid)
    {
        auto movingGrip = grips.first();
        nextTool_       = TextLine2::create(manipulator_, movingGrip);
        setCursor("move");
        return commit();
    }
    auto textLabels = utils::filtered(nei->closestLabels, isText);
    auto markLabels = utils::filtered(nei->closestLabels, isMarkLabel);
    if(started && textLabels.size())
    {
        auto label = textLabels.first();
        nextTool_  = PManipulatorTool(new MoveLabel(manipulator_, pt, label));
        setCursor("move");
        commit();
    }
    else if(markLabels.size())
    {
        // текстовая метка от МаркернойМетки
        PMarkLabel mark  = markLabels.first().dynamicCast<MarkLabel>();
        point3d    point = mark->locateTextGripPoint(pt);

        nextTool_ = TextLine::create(manipulator_, mark, point);
        scene_->pushAdorner(new NodePointAdorner(point, AdornerLineFromLine));
        setCursor("");
        commit();
    }
    else if(nei->closestObjects.size())
    {
        PObject object = nei->closestObjects[0];

        scene_->analizeCloseObject(nei, object, pt, false);
        point3d point = nei->objectInfo[object].closestPoint;

        nextTool_ = TextLine::create(manipulator_, object, point);
        scene_->pushAdorner(new NodePointAdorner(point, AdornerLineFromLine));
        setCursor("");
        commit();
    }
    else if(started)
    {
        do_click(pt, nei);
    }
    else
    {
        do_move(pt, nei);
    }
}

void TextStart::do_drop(point2d pt, PNeighbourhood nei)
{
}

static bool canAcceptText(PObject object)
{
    if(auto el = object.dynamicCast<Element>())
        return el->canBreakLine(0);

    return 1;
}

void TextStart::do_move(point2d pt, PNeighbourhood nei)
{
    auto grips       = utils::filter_transform<PTextGrip>(nei->closestGrips, toTextGrip);
    bool canTakeGrid = manipulator_->getMode(ControlKey) == 0;
    if(grips.size() && canTakeGrid)
    {
        setCursor("move");
    }
}

void TextStart::do_click(point2d pt, PNeighbourhood nei)
{
    auto labels = utils::filtered(nei->closestLabels, isText);

    point3d point = scene_->worldSystem()->toGlobal(pt);
    if(labels.size())
    {
        auto label = labels.first();
        nextTool_  = TextEdit::createOnExistingLabel(manipulator_, label, pt);
        setCursor("");
    }
    else
    {
        TextInfo info = scene_->defaultTextInfo();
        info.text     = QString("");
        PLabel label  = TextLabel::create(scene_, point, info);
        label->setLabelActive();

        Command cmd;
        cmd << cmdAttachLabel(scene_, label);

        QList<PObject> objects;
        objects = utils::filtered(nei->closestObjects, canAcceptText);

        if(objects.size())
        {
            PObject object = objects.first();
            point3d point  = nei->objectInfo[object].closestPoint;

            cmd << cmdAttachLabelToObject(scene_, label, label, object, point);
        }

        commandList_.addAndExecute(cmd);
        nextTool_ = TextEdit::create(manipulator_, label);
        setCursor("text-new");
    }
}

} // namespace geometry
