#include "manipulatorTextEdit.h"
#include "manipulatorMoveLabel.h"
#include "manipulatorTextLine.h"
#include "manipulatorTextLine2.h"
#include "manipulatorTextStart.h"
#include "nodePointAdorner.h"

#include "sceneProcedures.h"
#include "textLabel.h"

#include "graphicsScene.h"
#include "graphicsTextItem.h"
#include "textGrip.h"
#include "utilites.h"

#include "sceneProperties.h"

#include <QDebug>
#include <QGraphicsScene>
#include <QTextCursor>

namespace geometry
{

///////////////////////////////////////////////////////////////////////////////
//
// class TextPartner
//{
//    PTextLabel label;
// public:
//    double  angle;
//    point2d a, b, c, d;

//    TextPartner(PLabel label);
//};

// TextPartner::TextPartner(PLabel label)
//    : label( label.dynamicCast<TextLabel>() )
//{
//}

// typedef QSharedPointer<TextPartner> PTextPartner;
//
///////////////////////////////////////////////////////////////////////////////

QString TextEdit::helperText() const
{
    return QString::fromUtf8("<b>Редактируйте текст</b>:") + helperTab +
           QString::fromUtf8("<b>Ctrl + или -</b> изменить размер, ") + helperTab +
           QString::fromUtf8("<b>Ctrl-цифра</b> установить размер, "
                             "<b>Ctrl-*</b>,<b>Ctrl-5</b> вернуть начальный размер") +
           helperTab +
           QString::fromUtf8("<b>Ctrl-[ или Ctrl-]</b> повернуть текст, "
                             "<b>Tab</b> изменить обрамление");
}

TextEdit::TextEdit(Manipulator *m, PLabel p, point2d point)
    : ManipulatorTool(m)
    , isAboutToCommit_(false)
{
    label_ = p.dynamicCast<TextLabel>();
    info_ = previousInfo_ = label_->info();

    joinToPrevious_       = true;
    point_                = point;
    selectAll_            = false;
    returnToPreviousTool_ = false;
    supportModes_         = Clicking | Dragging;
}

void TextEdit::do_setUp()
{
    scene_->setTextEdit(this);
    scene_->setCursorPoint();

    label_->setLabelActive(true);

    // задаем выделение
    scene_->updateLabel(label_);

    QMap<PObjectToSelect, HoverState> ho;
    ho[label_] = HoverState::Newby;
    scene_->setHoverObjects(ho);

    // устанавливаем фокус на элемент
    // и этим начинаем редактирование
    auto item = label_->item();
    item->setFocus();
    if(selectAll_)
    {
        QTextCursor c = item->textCursor();
        c.select(QTextCursor::Document);
        item->setTextCursor(c);
    }

    if(auto *gs = dynamic_cast<GraphicsScene *>(item->scene()))
        gs->setTextEditMode(1);
}

void TextEdit::do_tearDown()
{
    scene_->setTextEdit(0);

    // сбросить выделение
    auto item = label_->item();
    if(item)
    {
        QTextCursor cr = item->textCursor();
        cr.clearSelection();
        item->setTextCursor(cr);

        // заканчиваем редактирование
        if(auto *gs = dynamic_cast<GraphicsScene *>(item->scene()))
            gs->setTextEditMode(0);
    }

    label_->setLabelActive(false);
}

void TextEdit::do_rollback()
{
}

void TextEdit::do_prepare()
{
    // устанавливаем грипы
    pushTextGrips(scene_, scene_->markersOfFollower(label_));

    setCursor("text-new");
}

void TextEdit::do_move(point2d, PNeighbourhood nei)
{
    // ничего не делаем
    nei->hoverState[label_] = HoverState::Newby;

    auto grips = utils::filter_transform<PTextGrip>(nei->closestGrips, toTextGrip);
    if(grips.size())
    {
        auto movingGrip = grips.first();
        nextTool_       = TextLine2::create(manipulator_, movingGrip);
        setCursor("move");
    }
}

void TextEdit::do_click(point2d pt, PNeighbourhood nei)
{
    auto grips = utils::filter_transform<PTextGrip>(nei->closestGrips, toTextGrip);
    if(grips.size())
    {
        auto movingGrip = grips.first();
        nextTool_       = TextLine2::create(manipulator_, movingGrip);
        setCursor("move");
        return do_editFinished();
    }

    auto labels = utils::filtered(nei->closestLabels, isText);

    if(labels.contains(label_))
    {
        // имитация двойного щелчка
        label_->selectAll();
        return;
    }

    // клик за пределами окна редактирования закрывает редактирование
    do_editFinished();

    // если кликнули на тексте, переходим к его редактированию
    if(labels.size())
    {
        auto label = labels.first();
        nextTool_  = TextEdit::createOnExistingLabel(manipulator_, label, pt);

        setCursor("");
    }
}

void TextEdit::editFinished()
{
    do_editFinished();
    commit();
}

void TextEdit::do_editFinished()
{
    isAboutToCommit_ = true;

    // отменяем прятавшуюся метку
    commandList_.undoAllAndClear();

    bool empty = info_.text.trimmed().isEmpty();
    if(!(previousInfo_ == info_) || empty)
    {
        // предыдущая метка должна иметь предыдущие настройки
        label_->setInfo(previousInfo_);
        Command cmd;

        if(empty)
        {
            if(label_->info().text.trimmed().isEmpty())
                abortPrevious_ = true;
            else
                cmd << cmdDetachLabel(scene_, label_);
        }
        else
        {
            PLabel newByLabel = TextLabel::create(scene_, info_);
            cmd << cmdReplaceLabel(scene_, label_, newByLabel);
        }
        commandList_.addAndExecute(cmd);
    }

    if(returnToPreviousTool_)
        nextTool_ = manipulator_->prevTool();
    else
        nextTool_ = TextStart::create(manipulator_);
}

void TextEdit::do_drop(point2d /*pt*/, PNeighbourhood /*nei*/)
{
}

void TextEdit::do_drag(point2d pt, PNeighbourhood nei, bool started)
{
    if(started)
    {
        auto grips = utils::filter_transform<PTextGrip>(nei->closestGrips, toTextGrip);
        if(grips.size())
        {
            auto grip = grips.first();
            nextTool_ = TextLine2::create(manipulator_, grip);
            setCursor("move");
            return commit();
        }
    }

    if(started && nei->closestObjects.size())
    {
        do_editFinished();

        PObject object = nei->closestObjects[0];
        point3d point  = nei->objectInfo[object].closestPoint;
        nextTool_      = TextLine::create(manipulator_, object, point);
        scene_->pushAdorner(new NodePointAdorner(point, AdornerLineFromLine));
        setCursor("");
        commit();
    }
}

PManipulatorTool TextEdit::createOnExistingLabel(Manipulator *m,
                                                 PLabel       p,
                                                 point2d      point,
                                                 bool         selectAll,
                                                 bool         returnToPrev)
{
    auto edit                   = new TextEdit(m, p, point);
    edit->joinToPrevious_       = false;
    edit->selectAll_            = selectAll;
    edit->returnToPreviousTool_ = returnToPrev;

    return PManipulatorTool(edit);
}

PManipulatorTool TextEdit::create(Manipulator *m, PLabel p)
{
    auto edit = new TextEdit(m, p, point2d());

    return PManipulatorTool(edit);
}

//////////////////////////////////////////////////////////////////////////

TextInfo TextEdit::textInfo() const
{
    return info_;
}

// точка, с которой началось редактирование
point2d TextEdit::clickedPoint() const
{
    return point_;
}

void TextEdit::checkFocus()
{
    if(isAboutToCommit_)
        return;

    auto item = label_->item();
    Q_ASSERT(item);
    if(!item->hasFocus())
    {
        editFinished();
    }
}

void TextEdit::checkChanged()
{
    auto    item = label_->item();
    QString text = item->toPlainText();

    scene_->updateLabel(label_);

    if(text != info_.text)
    {
        info_.text  = text;
        info_.width = label_->info().width;
    }
}

void TextEdit::do_changeSize(int delta, bool absolute)
{
    point2d start = info_.rotationPoint();
    delta         = qMin(4, delta);
    info_.setTextScaleNo(absolute ? delta : info_.scale + delta);
    info_.setRotationPoint(start);

    label_->setInfo(info_);
    scene_->updateLabel(label_);
    scene_->updatePropertiesAfterSceneChange();
}

void TextEdit::do_takeToolProperty(SceneProperties &prop)
{
    prop.addText(info_);
}

bool TextEdit::do_updateToolProperties(ScenePropertyValue v)
{
    if(isAboutToCommit_)
        return false;

    TextInfo sample = info_;
    if(!sample.applyStyle(scene_, v))
        return false;
    info_ = sample;

    auto item = label_->item();

    QTextCursor cursor = item ? item->textCursor() : QTextCursor();
    int         a      = cursor.anchor();
    int         p      = cursor.position();

    label_->setInfo(sample);
    scene_->updateLabel(label_);
    item = label_->item();

    cursor = item->textCursor();
    cursor.setPosition(a);
    cursor.setPosition(p, QTextCursor::KeepAnchor);
    item->setTextCursor(cursor);

    scene_->setTextEdit(this);

    if(!item->hasFocus())
        item->setFocus();

    return true;
}

void TextEdit::do_toggleMode()
{
    info_.decoration = (info_.decoration + 1) % (1 + TextInfo::EllipseStyle);

    label_->setInfo(info_);
    scene_->updateLabel(label_);
    scene_->updatePropertiesAfterSceneChange();
}

void TextEdit::changeRotation(int delta)
{
    point2d start = info_.rotationPoint();

    // составляем список углов
    QList<int> angles = scene_->textAngles();

    // находим текущий угол
    int angle = (int)round(info_.rotationAngleDegrees);
    int index = angles.indexOf(angle);
    if(index == -1)
    {
        info_.rotationAngleDegrees = 0;
    }
    else
    {
        // выбираем следующий угол
        index                      = qMax(0, qMin(angles.size() - 1, index + delta));
        info_.rotationAngleDegrees = angles[index];
    }
    info_.rotationIndex = index;

    info_.setRotationPoint(start);

    label_->setInfo(info_);
    scene_->updateLabel(label_);
    scene_->updatePropertiesAfterSceneChange();
}

bool TextEdit::eventInItem(int x, int y)
{
    auto item = label_->item();
    if(!item)
        return 0;

    auto point = QPoint(x, y);
    return item->contains(item->mapFromScene(point));
}

} // namespace geometry
