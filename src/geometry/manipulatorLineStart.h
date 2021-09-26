#ifndef MANIPULATORSTARTLINE_H
#define MANIPULATORSTARTLINE_H

#include "manipulator.h"

namespace geometry
{

class LineStart : public ManipulatorTool
{
    Q_OBJECT

    LineInfo lineInfo_;

public:
    /*
      - при щелчке на пустом месте начинает рисовать линию
      - при щелчке на линии или какой либо точке начинает рисовать
        линию от этой точки

    */

    LineStart(Manipulator *m);
    LineStart(Manipulator *m, LineInfo info);

public: //
    void    do_click(point2d, PNeighbourhood);
    QString helperText() const;

    void do_changeSize(int sizeFactor, bool absolute);
    void do_takeToolProperty(SceneProperties &);
    bool do_updateToolProperties(ScenePropertyValue);
};

} // namespace geometry

#endif // MANIPULATORSTARTLINE_H
