#ifndef CANVASRECTANGLE_H
#define CANVASRECTANGLE_H

#include "label.h"

namespace geometry
{

MAKESMART(CanvasRectangle);

enum CanvasSize
{
    A5 = 0,
    A4 = 1,
    A3 = 2,
};

struct CanvasInfo
{
    point2d leftTop;
    point2d rightBottom;
    point2d center;
    int     index;

    CanvasSize size;
    int        landscape;

    int width(int dpi) const;
    int height(int dpi) const;

    void setSize(CanvasSize s, bool landscape);
    void shiftBy(point2d);
    bool equalTo(CanvasInfo const &other) const;

    static CanvasInfo def();
};

class CanvasRectangle : public Label
{
    CanvasInfo info_;

    CanvasRectangle(Scene *, CanvasInfo c);

public:
    static PLabel create(Scene *scene, CanvasInfo info);
    static PLabel createFrom(Scene *, class QDomElement);

    CanvasInfo info() const;
    void       setInfo(CanvasInfo);

    virtual void saveLabel(QDomElement elm);

    virtual void   draw(GraphicsScene *gscene, GItems &gitems, int level);
    virtual double distance(point2d const &pt);
    virtual PLabel clone(Scene *, point2d delta) const;
    virtual QBrush originalColour() const;

    QRectF makeQRect() const;
};

inline auto toCanvasRectangle = [](PLabel l) { return l.dynamicCast<CanvasRectangle>(); };

inline auto byCanvasIndex = [](PCanvasRectangle lhs, PCanvasRectangle rhs) {
    return lhs->info().index < rhs->info().index;
};

} // namespace geometry
#endif // CANVASRECTANGLE_H
