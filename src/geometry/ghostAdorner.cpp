#include "ghostAdorner.h"

#include "graphics.h"
#include "graphicsScene.h"

namespace geometry
{

GhostAdorner::GhostAdorner(PObjectToSelect object)
    : sample(object)
    , cache()
{
}

void GhostAdorner::createCache(GraphicsScene *gscene, GItems &g)
{
    if(auto sample_locked = sample.lock())
    {
        sample_locked->draw(gscene, g, 0 /* контур не нужен */);
    }

    // подменяем стили
    foreach(PGItem item, g.items)
    {
        if(auto line = item.dynamicCast<GraphicsClipItem>())
            line->setPen(graphics::ghostAdornerPen());
        else if(auto line = item.dynamicCast<QGraphicsLineItem>())
            line->setPen(graphics::ghostAdornerPen());
        else if(auto line = item.dynamicCast<QGraphicsPathItem>())
        {
            line->setPen(graphics::ghostAdornerPen());
            line->setBrush(graphics::transparentBrush());
        }
    }
    cache.reset(new GItemsList(g.items));
}

void GhostAdorner::draw(GraphicsScene *gscene, GItems &g, int /*level*/)
{
    if(!cache)
    {
        createCache(gscene, g);
    }
    g.items = *cache;

    // удаляем контур
    // элементы автоматически удаляются из сцены
    g.contur.clear();
}
} // namespace geometry
