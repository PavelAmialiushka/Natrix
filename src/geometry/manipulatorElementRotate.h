#ifndef MANIPULATORELEMENTROTATE_H
#define MANIPULATORELEMENTROTATE_H

#include "manipulator.h"
#include "neighbourhood.h"

namespace geometry
{

class Element;
typedef QSharedPointer<Element> PElement;

class RotateElement : public ManipulatorTool
{
    Q_OBJECT

    PElement element_;

    int scaleFactor_;
    int activeNodeNo_;

    enum class Mode
    {
        Flip, // направление перехода в линии туда/обратно,
        OverCenterFree, // вращение в произвольном направлении вокруг центра
        OverNodeFree, // вращение в произвольном направлении вокруг ноды
        OverNodeAxis, // вращение вокруг направления заданной ноды
    };
    Mode             mode_;
    bool             rotateAfterFlip_;
    bool             inAnyDirection_;
    bool             rotateOverCenter_;
    PManipulatorTool prevTool_;

private:
    RotateElement(Manipulator *m, PElement element);

public:
    static RotateElement *inAnyDirection(Manipulator *m, PElement e);
    static RotateElement *flipAlongNode(Manipulator *m, PElement e, int node);
    static RotateElement *flipAlongNodeTwoStages(Manipulator *m, PElement e, int node);
    static RotateElement *rotateOverNodeAxis(Manipulator *m, PElement e, int node);
    static RotateElement *overNodeTwoStages(Manipulator *m, PElement e, int node);

    void setPrevTool(PManipulatorTool);

public: //
    void do_prepare();
    void do_click(point2d, PNeighbourhood);
    void do_changeSize(int sizeF, bool absolute);

    void do_takeToolProperty(SceneProperties &);
    bool do_updateToolProperties(ScenePropertyValue);

    QString helperText() const;
};

} // namespace geometry

#endif // MANIPULATORELEMENTROTATE_H
