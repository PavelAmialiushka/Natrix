#ifndef MANIPULATORCONTINUELINE_H
#define MANIPULATORCONTINUELINE_H

#include "manipulator.h"
#include "neighbourhood.h"

namespace geometry
{
class LineContinue : public ManipulatorTool
{
    Q_OBJECT
    point3d  start_;
    LineInfo lineInfo_;

    ConnectionData data_;

public:
    LineContinue(Manipulator *m, LineInfo info);

    static PManipulatorTool create_free(Manipulator *m, point3d const &start, LineInfo info);

    static PManipulatorTool createStartingFromLine(Manipulator *  m,
                                                   ConnectionData data,
                                                   LineInfo       info);

public:
    void    do_prepare();
    void    do_click(point2d, PNeighbourhood);
    QString helperText() const;

    void do_changeSize(int sizeFactor, bool absolute);
    void do_takeToolProperty(SceneProperties &);
    bool do_updateToolProperties(ScenePropertyValue);
};

} // namespace geometry

#endif // MANIPULATORCONTINUELINE_H
