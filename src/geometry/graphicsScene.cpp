#include "graphicsScene.h"
#include "pointProcedures.h"
#include <QPainter>

namespace geometry
{

GraphicsScene::GraphicsScene(PWorldSystem pw)
    : pws_(pw)
{
    noCursorPoint_ = true;
    textEditMode_  = false;
}

PWorldSystem GraphicsScene::ws() const
{
    return pws_;
}

void GraphicsScene::setCursorPoint(point2d p)
{
    cursorPoint_   = p;
    noCursorPoint_ = false;
}

void GraphicsScene::noCursorPoint()
{
    noCursorPoint_ = true;
}

void GraphicsScene::drawForeground(QPainter *painter, const QRectF &rect)
{
    if(noCursorPoint_)
        return;

    point2d      pt = cursorPoint_;
    WorldSystem &pw = *pws_;
    point3d      p  = pw.toGlobal(pt);

    QList<point3d> dirs;
    dirs << point3d(1, 0) << point3d(-1, 0) << point3d(0, 1) << point3d(0, -1);

    QList<point2d> pts;
    foreach(point3d d, dirs)
    {
        pts << pw.toUser(p + d);
    }

    point2d lt(rect.left(), rect.top());
    point2d rb(rect.right(), rect.bottom());

    point2d rt(rb.x, lt.y);
    point2d lb(lt.x, rb.y);

    if(lt == rb)
        return;

    QList<point2d> brds, pts2;
    brds << lt << lb << rt << rb << lt << rt << lb << rb;

    auto compareByXY = [](point2d a, point2d b) -> bool {
        return std::abs(a.x - b.x) < POINT_PREC ? a.y < b.y : a.x < b.x;
    };

    for(int p_index = 0; p_index < pts.size(); p_index += 2)
    {
        point2d aa = pts[p_index], bb = pts[p_index + 1];

        pts2.clear();
        for(int b_index = 0; b_index < brds.size(); b_index += 2)
        {
            point2d m1 = brds[b_index];
            point2d m2 = brds[b_index + 1];

            bool    ins, np;
            point2d q = cross_lines2d(aa, bb, m1, m2, &ins, &np);
            if(np)
            {
                pts2 << q;
            }
        }

        std::sort(pts2.begin(), pts2.end(), compareByXY);
        if(pts2.size() == 4)
        {
            pts2.takeFirst();
            pts2.takeLast();
        }

        pts[p_index]     = pts2[0];
        pts[p_index + 1] = pts2[1];
    }

    QPen pen(QBrush(QColor(255, 0, 0, 192)), 1, Qt::DotLine, Qt::RoundCap);
    painter->save();
    painter->setPen(pen);
    painter->setBrushOrigin(cursorPoint_.toQPoint());
    painter->drawLine(QLineF(pts[0].toQPoint(), pts[1].toQPoint()));
    painter->drawLine(QLineF(pts[2].toQPoint(), pts[3].toQPoint()));
    painter->restore();
}

void GraphicsScene::setTextEditMode(bool mode)
{
    textEditMode_ = mode;
    if(!mode)
        setFocusItem(0);
}

void GraphicsScene::focusInEvent(QFocusEvent *event)
{
    QGraphicsScene::focusInEvent(event);
    if(focusItem() && !textEditMode_)
    {
        setFocusItem(0);
    }
}

} // namespace geometry
