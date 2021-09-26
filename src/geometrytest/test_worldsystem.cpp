#include "geometry/worldsystem.h"
#include "gtest/gtest.h"

using namespace geometry;

TEST(worldTest, ortoWorld)
{
    PWorldSystem pw = WorldSystem::createOrtoWorldSystem();

    EXPECT_EQ(point3d(-1, 2, 3), pw->toGlobal(point2d(-1, 2, 3)));
    EXPECT_EQ(point2d(4, 5, -6), pw->toUser(point3d(4, 5, -6)));
}

TEST(worldTest, isometricWorld)
{
    PWorldSystem base0 = WorldSystem::createIsoplaneWorldSystem(30, 0);
    PWorldSystem base1 = WorldSystem::createIsoplaneWorldSystem(45, 30);
    point3d      pt03  = point3d(0, 0, 0);
    point3d      pt13  = point3d(0, 0, 1);
    point3d      pt23  = point3d(-1, 2, -3);
    point3d      pt33  = point3d(-10, -8, 15);

    point2d pt02 = point2d(0, 0, 0);
    point2d pt12 = point2d(0, 0, 1);
    point2d pt22 = point2d(-1, 2, -3);
    point2d pt32 = point2d(-10, -8, 15);

    // base 0
    EXPECT_EQ(base0->toGlobal(base0->toUser(pt03)), pt03);
    EXPECT_EQ(base0->toUser(base0->toGlobal(pt02)), pt02);

    EXPECT_EQ(pt03 >> *base0 >> *base0, pt03);
    EXPECT_EQ(pt02 >> *base0 >> *base0, pt02);

    EXPECT_EQ(base0->toGlobal(base0->toUser(pt13)), pt13);
    EXPECT_EQ(base0->toUser(base0->toGlobal(pt12)), pt12);

    EXPECT_EQ(base0->toGlobal(base0->toUser(pt23)), pt23);
    EXPECT_EQ(base0->toUser(base0->toGlobal(pt22)), pt22);

    EXPECT_EQ(base0->toGlobal(base0->toUser(pt33)), pt33);
    EXPECT_EQ(base0->toUser(base0->toGlobal(pt32)), pt32);

    // base 1
    EXPECT_EQ(base1->toGlobal(base1->toUser(pt33)), pt33);
    EXPECT_EQ(base1->toUser(base1->toGlobal(pt32)), pt32);
}

TEST(worldTest, globalAtLine)
{
    PWorldSystem pw = WorldSystem::createIsoplaneWorldSystem(45, 60);
    point3d      pt1(10, 0, 10);
    point3d      pt2(0, 20, 30);

    point3d m3 = (pt1 + pt2 * 3) / 4;
    EXPECT_EQ(m3, point3d(2.5, 15, 25));

    point2d m2 = pw->toUser(m3);

    point3d s = pw->toGlobal_atline(m2, pt1, pt2);
    EXPECT_EQ(s, m3);

    auto line = wsLine(pw, pt1, pt2);
    EXPECT_EQ(m2 >> line, s);
}

TEST(worldTest, globalAtPlane)
{
    PWorldSystem pw = WorldSystem::createIsoplaneWorldSystem(45, 60);

    point3d a(0, 0, 10);
    point3d b(10, 0, 0);
    point3d c(0, 10, 0);

    point3d onplane1(0, 5, 5);
    point3d onplane2(10. / 3, 10. / 3, 10. / 3);

    point2d op1 = pw->toUser(onplane1);
    point2d op2 = pw->toUser(onplane2);

    auto plane0 = wsPlane(pw, a, b, c);
    EXPECT_EQ(onplane1, op1 >> plane0);
    EXPECT_EQ(onplane2, op2 >> plane0);

    // группа элементов
    QList<point2d> pts;
    pts << point2d(100, 100);
    pts << point2d(-10, 10);
    pts << point2d(10, -10);
    pts << point2d(0, 0);

    auto plane1 = wsPlane(pw, point3d(0, 0, 0), point3d(10, 0, 0), point3d(0, 10, 0));

    auto plane2 = wsPlane(pw, point3d(0, 0, 0), point3d(0, 0, 10), point3d(0, 10, 0));

    foreach(point2d p, pts)
    {
        EXPECT_EQ(p, p >> plane0 >> plane0);
        EXPECT_EQ(p, p >> plane1 >> plane1);
        EXPECT_EQ(p, p >> plane2 >> plane2);
    }
}
