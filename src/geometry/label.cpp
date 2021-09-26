#include "label.h"

#include <QMap>
#include <QtXml>

#include "scene.h"
#include "utilites.h"
#include "worldSystem.h"

#include "marker.h"

namespace geometry
{

Label::Label(Scene *scene)
    : scene_(scene)
    , activeLabel_(0)
{
}

void Label::replaceScene(Scene *newby)
{
    qSwap(scene_, newby);
}

point2d Label::center() const
{
    return (leftTop_ + rightBottom_) / 2;
}

void Label::moveCenterTo(point2d newCenter)
{
    auto delta = newCenter - center();
    leftTop_ += delta;
    rightBottom_ += delta;
}

void Label::setSize(double cx, double cy)
{
    auto delta   = point2d(cx, cy) / 2;
    auto center  = (leftTop() + rightBottom()) / 2;
    leftTop_     = center - delta;
    rightBottom_ = center + delta;
}

bool Label::isActiveLabel() const
{
    return activeLabel_;
}

void Label::setLabelActive(bool m)
{
    activeLabel_ = m;
}

point2d Label::leftTop() const
{
    return leftTop_;
}

point2d Label::rightTop() const
{
    return point2d(rightBottom_.x, leftTop_.y);
}

point2d Label::leftBottom() const
{
    return point2d(leftTop_.x, rightBottom_.y);
}

point2d Label::rightBottom() const
{
    return rightBottom_;
}

PLabel Label::cloneMoveRotate(Scene *            scene,
                              point3d            delta,
                              point3d            center,
                              double             angle,
                              MoveRotateContext &context) const
{
    point3d marker{0};
    if(context.ms.size())
        marker = context.ms.first()->point;

    auto &ws = *scene->worldSystem();

    point2d delta2;
    point2d c0    = this->center();
    point3d point = ws.toGlobal(c0);

    if(marker.empty())
    {
        point  = delta + point.rotate3dAround(center, point3d::nz, angle);
        delta2 = (point >> ws) - c0;
    }
    else
    {
        // поворот вместе с маркерной точкой, с соблюдением положения
        point3d marker2 = delta + marker.rotate3dAround(center, point3d::nz, angle);

        auto length = c0.distance(marker >> ws);

        auto d1 = marker.polar_to(point, 1);
        auto d2 = delta + d1.rotate3dAround(center, point3d::nz, angle);

        bool ok;
        auto c1 = (marker2 >> ws).polar_to(d2 >> ws, length, &ok);

        if(ok)
            delta2 = c1 - c0;
        else
            delta2 = 0;
    }

    return clone(scene, delta2);
}

PLabel Label::clone(Scene *scene, point3d delta) const
{
    point3d      base{0};
    PWorldSystem pws = scene->worldSystem();
    return clone(scene, pws->convert(base + delta) - pws->convert(base));
}

namespace
{
struct Getter
{
    QMap<QString, Label::labelCreatorDE> name2creator;
    QMap<QString, QString>               type2name;

    static Getter &inst()
    {
        static Getter instance;
        return instance;
    }
};
} // namespace

void Label::registerLabelCreator(QString name, QString type, labelCreatorDE creator)
{
    Getter::inst().name2creator[name] = creator;
    Getter::inst().type2name[type]    = name;
}

QString Label::typeName() const
{
    QString type = typeid(*const_cast<Label *>(this)).name();
    QString name = Getter::inst().type2name[type];
    Q_ASSERT(!name.isEmpty());
    return name;
}

PLabel Label::createLabel(Scene *scene, QDomElement element)
{
    QString        name    = element.attribute("class");
    labelCreatorDE creator = Getter::inst().name2creator[name];
    if(!creator)
    {
        Q_ASSERT(!"incorrect label typeName");
        return PLabel();
    }

    PLabel label = creator(scene, element);
    bool   ok    = label->loadLabel(element);
    if(!ok)
    {
        Q_ASSERT(!"incorrect in loadLabel");
        return PLabel();
    }

    return label;
}

void Label::saveLabel(PLabel self, QDomElement element)
{
    element.setAttribute("class", self->typeName());
    self->saveLabel(element);
}

void Label::saveLabel(QDomElement)
{
}

bool Label::loadLabel(QDomElement)
{
    return 1;
}

} // namespace geometry
