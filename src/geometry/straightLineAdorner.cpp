#include "straightLineAdorner.h"

#include "scene.h"

#include "graphics.h"
#include "graphicsScene.h"

namespace geometry
{

StraightLineAdorner::StraightLineAdorner(point3d start, point3d end)
    : start_(start)
    , end_(end)
{
}

void StraightLineAdorner::draw(GraphicsScene *gscene, GItems &g, int /*level*/)
{
    point3d g0 = start_, g1 = end_;

    // рисуем вспомогательные линии
    if(!(g0 - g1).isAxeParallel())
    {
        point3d x(1), y(0, 1), z(0, 0, 1);
        point3d delta = g1 - g0;
        point3d dx = delta.project_to_direction(x), dy = delta.project_to_direction(y),
                dz = delta.project_to_direction(z);
        QList<point3d> tpp3;
        tpp3 << g0;
        tpp3 << (tpp3.last() + dx);
        tpp3 << (tpp3.last() + dy);
        tpp3 << (tpp3.last() + dz);
        tpp3 << (tpp3.last() - dx);
        tpp3 << (tpp3.last() - dy);
        tpp3 << (tpp3.last() - dz);
        tpp3 << g0;
        QList<point2d> tpp2;
        foreach(point3d p, tpp3)
        {
            point2d p2 = gscene->ws()->toUser(p);
            if(tpp2.empty() || tpp2.last() != p2)
                tpp2 << p2;
        }
        for(int index = 0; index < tpp2.size() - 1; ++index)
        {
            g.items << gscene->addLine(makeQLine(tpp2[index], tpp2[index + 1]),
                                       graphics::straightLineAdornerPen());
        }
    }
}

bool StraightLineAdorner::isTemporal() const
{
    return false;
}

} // namespace geometry
