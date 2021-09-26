#include "moveAdorner.h"
#include "scene.h"
#include "straightLineAdorner.h"
#include "worldSystem.h"

#include "graphics.h"
#include "graphicsScene.h"

namespace geometry
{

MoveAdorner::MoveAdorner(point3d f, point3d s, bool showStraight)
    : showStraight(showStraight)
    , first(f)
    , second(s)
{
}

MoveAdorner::MoveAdorner(point2d f, point2d s)
    : showStraight(true)
    , first2d(f)
    , second2d(s)
{
}

void MoveAdorner::draw(GraphicsScene *gscene, GItems &g, int /*level*/)
{
    WorldSystem &ws   = *gscene->ws();
    bool         is3d = first2d == second2d;
    auto         f    = is3d ? ws.toUser(first) : first2d;
    auto         s    = is3d ? ws.toUser(second) : second2d;
    g.items << gscene->addLine(makeQLine(f, s), graphics::moveLineAdornerPen());

    // доп, элементы
    if(showStraight)
    {
        auto sla = StraightLineAdorner(first, second);
        sla.draw(gscene, g, 0);
    }

    // удаляем контур
    // элементы автоматически удаляются из сцены
    g.contur.clear();
}

} // namespace geometry
