#include "geometry/point.h"
#include "geometry/tmatrix.h"

#include "geometry/element.h"
#include "geometry/elementtypes.h"

#include "gtest/gtest.h"

using namespace geometry;

TEST(elementsTest, cloneMoveRotateElement)
{
    PElement elm = PElement(new ValveElement);
    elm->setPObject(elm);

    double size = elm->globalPoint(0).distance(elm->globalPoint(1));
    ASSERT_EQ(elm->globalPoint(0), point3d::n);
    ASSERT_EQ(elm->direction(0), -point3d::nx);
    ASSERT_EQ(elm->globalPoint(1), point3d(size));
    ASSERT_EQ(elm->direction(1), point3d::nx);

    point3d  start(100, 200, 300);
    point3d  direction(0, 0, -1);
    PElement second = elm->cloneMoveRotate(0, 0, start, direction);

    ASSERT_EQ(second->globalPoint(0), start);
    ASSERT_EQ(second->direction(0), -direction);
    ASSERT_EQ(second->globalPoint(1), start + direction * size);
    ASSERT_EQ(second->direction(1), direction);
}
