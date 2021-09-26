#include "canvasGrip.h"

#include "graphics.h"
#include "graphicsScene.h"

namespace geometry
{

CanvasGrip::CanvasGrip(int index, PCanvasRectangle canvas)
    : canvas_(canvas)
{
    index_ = index;
    lt_    = canvas->leftTop();
    rb_    = canvas->rightBottom();
}

PCanvasRectangle CanvasGrip::canvas() const
{
    return canvas_;
}

void CanvasGrip::draw(GraphicsScene *gscene, GItems &gitems, int /*level*/)
{
    gitems.items << drawRectangle(gscene, lt_, rb_, Qt::NoPen, graphics::canvasAdornerBrush());

    // устанавливаем шрифт
    QFont font("Tahoma", 64);
    font.setKerning(0);

    QGraphicsTextItem *item = gscene->addText(QString("%1").arg(index_), font);
    item->setDefaultTextColor(QColor("red"));

    // корректируем с поправкой на размер текста
    auto rect = item->boundingRect();
    auto size = rect.size();
    item->moveBy(rb_.x - size.width() * 1.2, lt_.y + size.height() * 0.2);

    gitems.items << item;
    gitems.contur << drawRectangle(gscene, lt_, rb_, Qt::NoPen, QBrush(QColor(Qt::white)));
}

double CanvasGrip::distance(const point2d &pt)
{
    if(lt_.x <= pt.x && pt.x <= rb_.x && lt_.y <= pt.y && pt.y <= rb_.y)
    {
        return 0;
    }
    return 1e6;
}

} // namespace geometry
