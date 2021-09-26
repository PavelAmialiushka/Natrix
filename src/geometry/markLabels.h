#ifndef MARKLABELS_H
#define MARKLABELS_H

#include "label.h"

namespace geometry
{
MAKESMART(Marker);

enum SupportType
{
    Support          = 0,
    SupportSpring    = 1,
    Clip             = 2,
    ClipSpring       = 3,
    Wall             = 4,
    ConSupport       = 5,
    ConSupportSpring = 6,
    ConClip          = 9,
    ConClipSpring    = 10,
    Arrow            = 7,
    Underground      = 8,
    NosslePlug       = 20,
    NossleValve      = 21,
    NossleTI         = 22,
    NosslePI         = 23,
};

enum RotationType
{
    NoRotation     = 0,
    InLineRotation = 1,
    NormalRotation = 2
};

MAKESMART(MarkLabel);

inline auto isMarkLabel = [](PLabel a) { return a.dynamicCast<MarkLabel>(); };
inline auto toMarkLabel = [](PLabel a) { return a.dynamicCast<MarkLabel>(); };

// метка, жестко привязанная к объекту

class MarkLabel : public Label
{
    point2d basePoint_;
    point3d objectPoint_;
    point3d direction_;
    point3d lineDirection_;
    int     type_;

public:
    MarkLabel(Scene *, point3d point, point3d direction, point3d lineDirection);

public:
    int     type() const;
    void    setType(int);
    point2d basePoint() const;

    point3d objectPoint() const;

    int        rotationType() const;
    static int rotationType(int);
    point3d    direction() const;

    void    setLineDirection(point3d);
    point3d lineDirection() const;

    point3d locateTextGripPoint(point2d pt) const;
    point3d transformPoint(point3d point, PMarkLabel label2);

public:
    void       draw(GraphicsScene *gscene, GItems &gitems, int level);
    PLabel     clone(Scene *, point2d delta) const;
    PLabel     clone(Scene *, point3d delta) const;
    PMarkLabel clone(Scene *, point3d delta, point3d direction) const;
    PLabel     cloneMoveRotate(Scene *            scene,
                               point3d            delta,
                               point3d            center,
                               double             angle,
                               MoveRotateContext &context) const;
    double     distance(const point2d &pt);

    static PLabel createFrom(Scene *, class QDomElement);
    void          saveLabel(QDomElement);
    bool          loadLabel(QDomElement);

    static int     typeToInt(QString);
    static QString typeFromInt(int);
};

} // namespace geometry

#endif // MARKLABELS_H
