#ifndef MANIPULATORLabelROTATE_H
#define MANIPULATORLabelROTATE_H

#include "manipulator.h"
#include "markLabels.h"
#include "neighbourhood.h"

namespace geometry
{

class RotateLabel : public ManipulatorTool
{
    Q_OBJECT

    PMarkLabel label_;

    bool    fixedByDirection_;
    point3d fixingDirection_;
    int     activeNodeNo_;
    bool    markSpectialDirection_;

    enum class Mode
    {
        Flip, // направление перехода в линии туда/обратно,
        OverCenterFree, // вращение в произвольном направлении вокруг центра
        OverNodeFree, // вращение в произвольном направлении вокруг ноды
        OverNodeAxis, // вращение вокруг направления заданной ноды
    };
    Mode             mode_;
    bool             inAnyDirection_;
    bool             rotateOverObject_;
    PObject          baseObject_;
    PManipulatorTool prevTool_;

private:
    RotateLabel(Manipulator *m, PMarkLabel Label);

public:
    static RotateLabel *inAnyDirection(Manipulator *m, PMarkLabel e);
    static RotateLabel *flipAlongDirection(Manipulator *m, PMarkLabel e, point3d dir);
    static RotateLabel *rotateOverObject(Manipulator *m, PMarkLabel e, PObject object);

    void setMarkSpecialDirection();

    void setPrevTool(PManipulatorTool);

public: //
    void do_prepare();
    void do_click(point2d, PNeighbourhood);
    void do_changeSize(int sizeF, bool absolute);

    QString helperText() const;
};

QList<point3d> selectMarkLabelNormalDirections(PObject object, QList<point3d> variants);
point3d        calcLineDirectionOfMarkLabel(PObject object, point3d dir);

} // namespace geometry

#endif // MANIPULATORLabelROTATE_H
