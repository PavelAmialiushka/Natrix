#ifndef GRAPHICPAINTER_H
#define GRAPHICPAINTER_H

#include "geometry/global.h"
#include "geometry/manipulator.h"
#include "geometry/scene.h"

#include <QPainter>

#if 0


using namespace geometry;

class SceneWidgetDrawer
        : public geometry::Drawer
{
    QPainter* painter_;
    QMap<int, QPen> pens_;
    QFont font_;
    int currentStyle_;
    QPainterPath windowPath_;
public:


    SceneWidgetDrawer(QPainter& painter_, QRect windowRect);

    static QString fontString();
    static void exportPage(Scene*, QPaintDevice*, QRect windowRect);
public:
    void changeScale(double s);

    void setPen(int style);
    void prepare_draw(int style, int hover, int selected, function<void()> drawPrimitives);

    void draw(PrimitiveLine const &line);
    void simple_draw_line(PrimitiveLine const &line);

    void draw(PrimitiveBox const& box);
    void simple_draw_box(PrimitiveBox const &box);

    void draw(PrimitiveSpline const& spline);
    void simple_draw_spline(QList<point2d> points);

    void draw(PrimitiveCircle const& box);
    void simple_draw_circle(point2d, double);

    void draw(PrimitiveSpaceOn const& space);
    void draw(PrimitiveSpaceOff const&);

    void draw(PrimitiveText const&);
};

#endif
#endif // GRAPHICPAINTER_H
