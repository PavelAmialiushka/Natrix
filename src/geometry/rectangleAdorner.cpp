#include "rectangleAdorner.h"

#include "graphics.h"
#include "graphicsScene.h"

namespace geometry
{

RectangleAdorner::RectangleAdorner(point2d a, point2d b)
    : lt_(a)
    , rb_(b)
{
}

void RectangleAdorner::draw(GraphicsScene *gscene, GItems &g, int /*level*/)
{
    point2d lb{lt_.x, rb_.y};
    point2d rt{rb_.x, lt_.y};

    QPainterPath path;
    path.moveTo(lt_.toQPoint());
    path.lineTo(lb.toQPoint());
    path.lineTo(rb_.toQPoint());
    path.lineTo(rt.toQPoint());
    path.lineTo(lt_.toQPoint());

    auto pen = lt_.x < rb_.x ? graphics::rectangle1AdornerPen() : graphics::rectangle2AdornerPen();
    auto color = pen.color();
    color.setAlphaF(0.10);

    auto brush = QBrush(color, Qt::SolidPattern);
    g.items << gscene->addPath(path, pen, brush);

    // удаляем контур
    // элементы автоматически удаляются из сцены
    g.contur.clear();
}

} // namespace geometry
