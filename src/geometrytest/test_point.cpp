#include "geometry/graphics.h"
#include "geometry/point.h"
#include "geometry/pointProcedures.h"

#include "gtest/gtest.h"

#include <QSet>

using namespace geometry;

std::ostream &operator<<(std::ostream &os, point2d p)
{
    return os << p.toQString().toStdString();
}

std::ostream &operator<<(std::ostream &os, point3d p)
{
    return os << p.toQString().toStdString();
}

TEST(pointTest, creation)
{
    point3d a = point3d(1, 2, 3);
    point3d b = point3d(7, 5, 6);
    //    ASSERT_NE(a, a);

    ASSERT_EQ(1, a.x);
    ASSERT_EQ(2, a.y);
    ASSERT_EQ(3, a.z);

    ASSERT_EQ(5, b.y);
    ASSERT_EQ(6, b.z);
    ASSERT_EQ(7, b.x);

    point3d p = point3d(1, 2);
    ASSERT_EQ(1, p.x);
    ASSERT_EQ(2, p.y);
    ASSERT_EQ(0, p.z);
}

TEST(pointTest, recreation)
{
    // project_to
    ASSERT_EQ(point3d(100, 0, 0), point3d(100, 200, 300).project_to_direction(point3d(50)));
    ASSERT_EQ(point3d(0, 200, 0), point3d(100, 200, 300).project_to_direction(point3d(0, 1, 0)));
    ASSERT_EQ(point3d(0, 0, 300), point3d(100, 200, 300).project_to_direction(point3d(0, 0, -5)));

    // resize
}

TEST(pointTest, compare)
{
    EXPECT_TRUE(point3d(1, 2, 3) == point3d(1, 2, 3));
    EXPECT_TRUE(point3d(1.00000001, 2, 3) == point3d(1, 2, 3));
    EXPECT_TRUE(point3d(1, 2, 3) == point3d(1, 2, 3.00009));

    EXPECT_TRUE(point3d(1.005, 2, 3) != point3d(1, 2, 3));
    EXPECT_TRUE(point3d(1.1, 2, 3) != point3d(1, 2, 3));
    EXPECT_TRUE(point3d(1, 2, 3.3) != point3d(1, 2, 3));
    EXPECT_TRUE(point3d(1, 2, 3.3) != point3d(1, 2, 3));

    EXPECT_TRUE(point3d(0, 0, 1));
    EXPECT_TRUE(!point3d(1, 2, 3).empty());
    EXPECT_TRUE(point3d(0, 0, 0).empty());
    EXPECT_TRUE(!point3d(0, 0, 0));
}

TEST(pointTest, distance)
{
    // length
    ASSERT_EQ(point3d(12, 0, 0).length(), 12.0);
    EXPECT_NEAR(point3d(1, 1, 0).length(), 1.41421356237309, POINT_PREC);
    EXPECT_NEAR(point3d(-1, -1, 0).length(), 1.41421356237309, POINT_PREC);
    EXPECT_NEAR(point3d(1, 2, 3).length(), 3.74165738677394, POINT_PREC);
    EXPECT_NEAR(point3d(5, 5, 5).length(), 8.66025403784438, POINT_PREC);

    // distance
    EXPECT_NEAR(point3d(1, 2, 3).distance(point3d(4, 5, 6)), point3d(3, 3, 3).length(), POINT_PREC);
    EXPECT_NEAR(
        point3d(12, 0, 0).distance(point3d(0, 12, 0)), point3d(12, 12, 0).length(), POINT_PREC);

    // normalized
    ASSERT_EQ(point3d(10, 0, 0).normalized(), point3d(1, 0, 0));
    ASSERT_EQ(point3d(1, 2, 3).normalized(), point3d(0.26726, 0.53452, 0.801783));
    ASSERT_EQ(point3d(-1, 0, 0).normalized(), point3d(-1, 0, 0));
}

TEST(pointTest, operators)
{
    // operators
    // operator+
    // operator-
    // operator*k
    ASSERT_EQ(point3d(1, 2, 3) + point3d(4, 5, 6), point3d(5, 7, 9));

    ASSERT_EQ(point3d(5, 10, 15) - point3d(3, 2, 1), point3d(2, 8, 14));
    ASSERT_EQ(point3d(-5, -10, -15) - point3d(-5, -10, -15), point3d(0, 0, 0));

    ASSERT_EQ(-point3d(1, 2, 3), point3d(-1, -2, -3));
}

TEST(pointTest, project_and_make_coordinate)
{
    ASSERT_DOUBLE_EQ(point3d(10).project_and_make_coordinate(point3d(1)), 10);
    ASSERT_DOUBLE_EQ(point3d(0, 10).project_and_make_coordinate(point3d(1)), 0);
    ASSERT_DOUBLE_EQ(point3d(10, 10).project_and_make_coordinate(point3d(1)), 10);
}

TEST(pointTest, fraction_at_projection)
{
    // fraction_at_section

    ASSERT_DOUBLE_EQ(point2d(12, 7, 0).fraction_at_projection(point2d(0, 0), point2d(0, 16)),
                     7.0 / 16);
    ASSERT_DOUBLE_EQ(point2d(9, -7, 0).fraction_at_projection(point2d(0, 0), point2d(0, 16)),
                     -7.0 / 16);
    ASSERT_DOUBLE_EQ(point2d(3, 21, 0).fraction_at_projection(point2d(0, 0), point2d(0, 16)),
                     21.0 / 16);
}

TEST(pointTest, fraction_at_section)
{
    // fraction_at_section

    ASSERT_DOUBLE_EQ(point2d(0, 7, 0).fraction_at_section(point2d(0, 0), point2d(0, 16)), 7.0 / 16);
    ASSERT_DOUBLE_EQ(point2d(0, -7, 0).fraction_at_section(point2d(0, 0), point2d(0, 16)),
                     -7.0 / 16);
    ASSERT_DOUBLE_EQ(point2d(0, 21, 0).fraction_at_section(point2d(0, 0), point2d(0, 16)),
                     21.0 / 16);
}

TEST(pointTest, partition_by)
{
    // partition_by
    ASSERT_EQ(point3d(0, 0, 0).partition_by(point3d(2, 4, 6), 0.0), point3d(0, 0, 0));
    ASSERT_EQ(point3d(0, 0, 0).partition_by(point3d(2, 4, 6), 0.5), point3d(1, 2, 3));
    ASSERT_EQ(point3d(0, 0, 0).partition_by(point3d(2, 4, 6), 1.0), point3d(2, 4, 6));
    ASSERT_EQ(point3d(0, 0, 0).partition_by(point3d(2, 4, 6), 2.0), point3d(4, 8, 12));
    ASSERT_EQ(point3d(1, 7, 9).partition_by(point3d(-11, 4, 8), 0.25), point3d(-2, 6.25, 8.75));
}

TEST(pointTest, multiplication)
{
    // operator *k
    ASSERT_EQ(point3d(1, 2, 3) * 5, point3d(5, 10, 15));
    ASSERT_EQ(5 * point3d(1, 2, 3), point3d(5, 10, 15));
    ASSERT_EQ(point3d(-1, -2, -3) * 1, point3d(-1, -2, -3));
    ASSERT_EQ(point3d(-1, 2, -3) * (-1), point3d(1, -2, 3));

    // operator /k
    ASSERT_EQ(point3d(10, 20, 40) / 2.5, point3d(4, 8, 16));
    ASSERT_EQ(point3d(1, -2, 3) / (-0.5), point3d(-2, 4, -6));

    // dot
    ASSERT_EQ(point3d(1, 0, 0).dot(point3d(1, 0, 0)), 1.0);
    ASSERT_EQ(point3d(1, 0, 0).dot(point3d(0, 1, 0)), 0.0);
    ASSERT_EQ(point3d(2, 0, 0).dot(point3d(3, 0, 0)), 6.0);
    ASSERT_EQ(point3d(0, -6, 0).dot(point3d(0, -5, 0)), 30.0);
    ASSERT_EQ(point3d(1, 2, 3).dot(point3d(4, 5, 6)), 32.0);
}

TEST(pointTest, project_to_line)
{
    point3d a(10, 20);
    ASSERT_EQ(a.project_to_line(point3d(), point3d(1, 0)), point3d(10, 0));
    ASSERT_EQ(a.project_to_line(point3d(), point3d(0, 1)), point3d(0, 20));

    // distance
    ASSERT_EQ(a.distance_to_line(point3d(), point3d(0, 100)), 10.0);
    ASSERT_EQ(a.distance_to_line(point3d(), point3d(100, 0)), 20.0);
}

TEST(pointTest, testPoint2d3ddifference)
{
    ASSERT_EQ(point2d(1, 2, 100), point2d(1, 2, 50));
    ASSERT_EQ(point2d(1, 2, 100).length(), point2d(1, 2, 50).length());
    ASSERT_EQ(point2d(1, 2, 100).dot(point2d(1, 2, 50)), point2d(1, 2).dot(point2d(1, 2)));
}

TEST(pointTest, testRotatePoints)
{
    ASSERT_EQ(point2d(10, 0).rotate2dAround(point2d(), M_PI_2), point2d(0, 10));

    ASSERT_EQ(point3d(100, 0, 0).rotate3dAround(point3d(), point3d(0, 0, 1), M_PI),
              point3d(-100, 0, 0));
    ASSERT_EQ(point3d(100, 0, 0).rotate3dAround(point3d(), point3d(0, 0, 1), M_PI / 2),
              point3d(0, 100, 0));

    ASSERT_EQ(point2d(5, 5).rotate2dAround(point2d(5, 5), M_PI_2), point2d(5, 5));
}

TEST(pointTest, testRotatePoints3d)
{
    ASSERT_EQ(point3d(1, 1, 1).rotate3dAround(0, point3d::nz, M_PI_2), point3d(-1, 1, 1));
    ASSERT_EQ(point3d(1, 1, 1).rotate3dAround(0, point3d::nz, M_PI), point3d(-1, -1, 1));
    ASSERT_EQ(point3d(0, 0, 1).rotate3dAround(0, point3d::nz, M_PI_2), point3d(0, 0, 1));
    ASSERT_EQ(point3d(0, 0, 12).rotate3dAround(0, point3d::nz, M_PI_2), point3d(0, 0, 12));
    ASSERT_EQ(point3d(7, 8, 11).rotate3dAround(0, point3d::nz, 0), point3d(7, 8, 11));
}

TEST(pointTest, testCrossLines2d)
{
    bool cross, nonparallel;
    // перпендикулярны
    ASSERT_EQ(
        cross_lines2d(point2d(), point2d(100, 0), point2d(), point2d(0, 100), &cross, &nonparallel),
        point2d(0, 0));
    ASSERT_EQ(cross, true);
    ASSERT_EQ(nonparallel, true);

    // параллельны
    ASSERT_EQ(cross_lines2d(
                  point2d(), point2d(100, 0), point2d(0, 2), point2d(10, 2), &cross, &nonparallel),
              point2d(0, 0));
    ASSERT_EQ(cross, false);
    ASSERT_EQ(nonparallel, false);

    // пересекаются вне границы
    ASSERT_EQ(cross_lines2d(
                  point2d(), point2d(5, 5), point2d(100, 0), point2d(0, 100), &cross, &nonparallel),
              point2d(50, 50));
    ASSERT_EQ(cross, false);
    ASSERT_EQ(nonparallel, true);

    point2d cr_point = cross_lines2d(
        point2d(), point2d(), point2d(100, 0, 0), point2d(100, 0, 0), &cross, &nonparallel);

    ASSERT_EQ(cr_point, point2d());
    ASSERT_EQ(cross, false);
    ASSERT_EQ(nonparallel, false);
}

TEST(pointTest, testCrossLines3d)
{
    bool cross, nonparallel;
    // перпендикулярны 3d
    point3d cr_point;
    cr_point = cross_lines3d(point3d(0, 100, 77),
                             point3d(0, 200, 77),
                             point3d(100, 0, 77),
                             point3d(200, 0, 77),
                             &cross,
                             &nonparallel);
    ASSERT_EQ(cr_point, point3d(0, 0, 77));
    ASSERT_EQ(cross, true);
    ASSERT_EQ(nonparallel, true);

    cr_point = cross_lines3d(point3d(77, 0, 100),
                             point3d(77, 0, -100),
                             point3d(77, 100, 0),
                             point3d(77, 150, 0),
                             &cross,
                             &nonparallel);
    ASSERT_EQ(cr_point, point3d(77, 0, 0));
    ASSERT_EQ(cross, true);
    ASSERT_EQ(nonparallel, true);

    cr_point = cross_lines3d(point3d(0, 77, 100),
                             point3d(0, 77, 200),
                             point3d(100, 77, 0),
                             point3d(200, 77, 0),
                             &cross,
                             &nonparallel);
    ASSERT_EQ(cr_point, point3d(0, 77, 0));
    ASSERT_EQ(cross, true);
    ASSERT_EQ(nonparallel, true);
}

TEST(pointTest, testReplacePointBase3to3)
{
    // при некорректно входном профиле выход - первая точка выходного профиля
    //
    auto a = replacePointBase3to3(point2d(0, 0),
                                  point2d(0, 0),
                                  point2d(0, 0),
                                  point2d(0, 0),
                                  point2d(1, 0),
                                  point2d(0, 1),
                                  point2d(1, 1));
    ASSERT_EQ(a, point2d(1, 0));

    // если выход свернут в точку, то и результат попадает в эту точку
    //
    auto b = replacePointBase3to3(point2d(1, 1),
                                  point2d(5, 0),
                                  point2d(0, 5),
                                  point2d(5, 5),
                                  point2d(1, 0),
                                  point2d(1, 0),
                                  point2d(1, 0));
    ASSERT_EQ(b, point2d(1, 0));

    // параллельное смешение
    //
    auto c = replacePointBase3to3(point2d(1, 1),
                                  point2d(0, 0),
                                  point2d(0, 5),
                                  point2d(5, 0),
                                  point2d(5, 0),
                                  point2d(5, 5),
                                  point2d(10, 0));
    ASSERT_EQ(c, point2d(6, 1));

    // вращение отображение
    //
    auto d = replacePointBase3to3(point2d(1, 1),
                                  point2d(0, 0),
                                  point2d(0, 5),
                                  point2d(5, 0),
                                  point2d(0, 0),
                                  point2d(-5, 0),
                                  point2d(0, 5));
    ASSERT_EQ(d, point2d(-1, 1));

    // отображение 1 в 1
    //
    auto e = replacePointBase3to3(point2d(1, 1),
                                  point2d(0, 0),
                                  point2d(0, 9),
                                  point2d(5, 0),
                                  point2d(0, 0),
                                  point2d(0, 9),
                                  point2d(5, 0));
    ASSERT_EQ(e, point2d(1, 1));

    // отображение 1 в 1
    //
    auto f = replacePointBase3to3(point2d(1, 1),
                                  point2d(0, 0),
                                  point2d(0, 9),
                                  point2d(5, 0),
                                  point2d(0, 0),
                                  point2d(0, 9),
                                  point2d(5, 0));
    ASSERT_EQ(f, point2d(1, 1));
}

TEST(pointTest, testSerialization)
{
    point3d A(1, 2, -7);
    point3d B(-3, 4, 8);

    QString xA = A.serialSave();

    bool    ok;
    point3d a = point3d::serialLoad(xA, &ok);
    ASSERT_EQ(a, A);
    ASSERT_EQ(ok, true);

    QString xB = B.serialSave();
    point3d b  = point3d::serialLoad(xB, &ok);
    ASSERT_EQ(b, B);
    ASSERT_EQ(ok, true);

    point3d::serialLoad(",,", &ok);
    //    ASSERT_EQ(ok, false);

    point3d::serialLoad("", &ok);
    ASSERT_EQ(ok, false);

    point3d::serialLoad("4", &ok);
    ASSERT_EQ(ok, false);

    point3d::serialLoad("4,42,3,4", &ok);
    ASSERT_EQ(ok, false);

    point3d::serialLoad("abra,kadab,ra", &ok);
    //    ASSERT_EQ(ok, false);
}

TEST(hullTest, testSimple)
{
    point2d a(0, 0), b(1, 1), c(1, 2), d(1, 3), e(2, 4);

    QList<point2d> points;
    points << a << b << c << d << e;

    QSet<point2d> okSet;
    okSet << a << b << d << e;

    points = makeAHull(points);
    ASSERT_TRUE(points.toSet() == okSet);

    enlargeRegion_hull(points, 4);
}
