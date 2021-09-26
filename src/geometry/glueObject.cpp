#include "glueObject.h"
#include "global.h"
#include "graphics.h"
#include "objectFactory.h"

#include "graphicsScene.h"
#include "scene.h"

namespace geometry
{

CREATE_CLASS_BY_NODELIST(GlueObject);

GlueObject::GlueObject(Scene *s)
    : Object(s, 2)
{
    double          size = flangesWidth;
    QList<NodeInfo> list;
    list << NodeInfo{point3d::n, -point3d::nx};
    list << NodeInfo{point3d::nx * size, point3d::nx};
    setNodeList(s, list);
}

PObject GlueObject::create(Scene *scene, point3d p1, point3d p2)
{
    Q_ASSERT(p1 != p2);

    // реверсия
    auto dr = -(p1 - p2).normalized();

    QList<NodeInfo> list;
    list << NodeInfo{p1, dr} << NodeInfo{p2, -dr};
    return createFromList(scene, list);
}
PObject GlueObject::createFromList(Scene *scene, QList<NodeInfo> list)
{
    Q_ASSERT(list.size());

    auto    glue = new GlueObject{scene};
    PObject result(glue);
    glue->setPObject(result);

    glue->nodes_[0]->setPoint(scene->worldSystem(), list[0].globalPoint, list[0].direction);
    glue->nodes_[1]->setPoint(scene->worldSystem(), list[1].globalPoint, list[1].direction);

    return result;
}

PObject GlueObject::clone(Scene *scene, QList<NodeInfo> nodeList) const
{
    PObject elem(new GlueObject{scene});
    elem->setNodeList(scene, nodeList);
    elem->setPObject(elem);
    return elem;
}

void GlueObject::draw(GraphicsScene *gscene, GItems &g, int /*level*/)
{
#ifndef NDEBUG
    // не рисуем ничего вообще
    auto t  = localPoint(1) - localPoint(0);
    bool ok = false;
    auto d1 = t.rotate2dAround(0, M_PI_2).resized(t.length() * 3, &ok);
    auto d2 = t.rotate2dAround(0, -M_PI_2).resized(t.length() * 3, &ok);

    d1 += localPoint(0);
    d2 += localPoint(1);

    g.items << GraphicsClipItem::create(gscene, makeQLine(d1, d1 + t), graphics::elementPen());
    g.items << GraphicsClipItem::create(gscene, makeQLine(d2, d2 + t), graphics::elementPen());
#endif
}

double GlueObject::width() const
{
    return 0;
}

PObject GlueObject::cloneMove(Scene *scene, point3d delta)
{
    return create(scene, globalPoint(0) + delta, globalPoint(1) + delta);
}

PObject GlueObject::cloneMoveRotate(Scene *scene, point3d delta, point3d center, double angle)
{
    auto a = delta + globalPoint(0).rotate3dAround(center, point3d::nz, angle);
    auto b = delta + globalPoint(1).rotate3dAround(center, point3d::nz, angle);
    return create(scene, a, b);
}

} // namespace geometry
