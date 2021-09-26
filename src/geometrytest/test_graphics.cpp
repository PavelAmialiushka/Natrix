#include "geometry/graphics.h"

#include "gtest/gtest.h"

#include <QSet>

using namespace geometry;

namespace
{
std::ostream &operator<<(std::ostream &os, point2d p)
{
    return os << p.toQString().toStdString();
}

std::ostream &operator<<(std::ostream &os, point3d p)
{
    return os << p.toQString().toStdString();
}
} // namespace

TEST(hull, creation1)
{
    // одна точка это оболочка
    QList<point2d> list;
    list << point2d(10, 10);
    ASSERT_TRUE(isHull(list));
}

TEST(hull, creation2)
{
    // две точки это всегда оболочка
    QList<point2d> list;
    list << point2d(10, 10);
    list << point2d(10, 20);
    ASSERT_TRUE(isHull(list));
}

TEST(hull, creation1_same)
{
    // одинаковые точки
    QList<point2d> list;
    list << point2d(10, 10);
    list << point2d(10, 10);
    ASSERT_TRUE(isHull(list));
}

TEST(hull, creation2_same)
{
    // одинаковые точки
    QList<point2d> list;
    list << point2d(10, 20);
    list << point2d(10, 10);
    list << point2d(10, 20);
    list << point2d(10, 10);
    ASSERT_TRUE(isHull(list));
}

TEST(hull, creation2_2_same)
{
    // три точки
    QList<point2d> list;
    list << point2d(10, 20);
    list << point2d(10, 10);
    list << point2d(10, 20);
    list << point2d(10, 10);
    ASSERT_TRUE(isHull(list));
}

TEST(hull, creation3)
{
    // три точки
    QList<point2d> list;
    list << point2d(10, 20);
    list << point2d(30, 30);
    list << point2d(10, 10);
    ASSERT_TRUE(isHull(list));
}

TEST(hull, creation3_same)
{
    // три точки
    QList<point2d> list;
    list << point2d(10, 20);
    list << point2d(30, 30);
    list << point2d(10, 20);
    list << point2d(10, 10);
    list << point2d(30, 30);
    ASSERT_TRUE(isHull(list));
}

TEST(hull, creation_not_a_hull)
{
    // три точки
    QList<point2d> list;
    list << point2d(0, 0);
    list << point2d(0, 40);
    list << point2d(40, 40);
    list << point2d(10, 20);
    ASSERT_FALSE(isHull(list));
}

TEST(hull, flange_not_a_hull_make_a_hull)
{
    QList<point2d> list;
    list << point2d(76.902680752337872, 56.445221176959429);
    list << point2d(73.432923066127685, 58.435392425617522);
    list << point2d(84.746631565112452, 64.924669028379739);
    list << point2d(88.216389251322639, 62.934497779721646);
    list << point2d(84.105301003648449, 65.292521126347282);
    list << point2d(87.575058689858636, 63.302349877689188);
    list << point2d(76.261350190873870, 56.813073274926971);
    list << point2d(72.791592504663683, 58.803244523585064);
    ASSERT_FALSE(isHull(list));

    auto list2 = makeAHull(list);
    ASSERT_TRUE(isHull(list2));

    enlargeRegion_hull(list2, 2);
    ASSERT_TRUE(isHull(list2));
}
