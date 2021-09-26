#include "geometry/point.h"
#include "geometry/tmatrix.h"

#include "gtest/gtest.h"

#include <QList>
#include <tuple>
#include <utility>

using namespace geometry;

TEST(tmatrixTest, TransformMatrixMul)
{
    TMatrix e{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
    TMatrix a{{1, 2, 3}, {4, 5, 6}, {7, 8, 9}};
    TMatrix b{{2, 3, 4}, {5, 6, 7}, {8, 9, 10}};
    TMatrix m{{36, 42, 48}, {81, 96, 111}, {126, 150, 174}};

    ASSERT_EQ(a, e * a);
    ASSERT_EQ(a, a * e);
    ASSERT_EQ(b, e * b);
    ASSERT_EQ(b, b * e);
    ASSERT_EQ(m, a * b);
}

TEST(tmatrixTest, TransformMatrixWorks)
{
    TMatrix m{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};

    point3d pt{35, 12, 7};
    ASSERT_EQ(pt, pt * m);
}

// используем правую тройку, это значит, оси x, y, z соответствую пальцам
// большому, указательному и среднему для правой руки, а поворот на угол >0
// соответствует повороту против часовой стрелки

TEST(tmatrixTest, CreateRotateTMatrix)
{
    point3d x{1, 0, 0}, y{0, 1, 0}, z(0, 0, 1);
    TMatrix rotate_x_90 = TMatrix::rotate_x(M_PI_2);
    TMatrix rotate_y_90 = TMatrix::rotate_y(M_PI_2);
    TMatrix rotate_z_90 = TMatrix::rotate_z(M_PI_2);

    ASSERT_EQ(rotate_x_90 * x, x);
    ASSERT_EQ(rotate_x_90 * y, z);
    ASSERT_EQ(rotate_x_90 * z, -y);

    ASSERT_EQ(rotate_y_90 * x, -z);
    ASSERT_EQ(rotate_y_90 * y, y);
    ASSERT_EQ(rotate_y_90 * z, x);

    ASSERT_EQ(rotate_z_90 * x, y);
    ASSERT_EQ(rotate_z_90 * y, -x);
    ASSERT_EQ(rotate_z_90 * z, z);
}

TEST(tmatrixTest, rotation3d)
{
    point3d x{7, 0, 0}, y{0, 7, 0}, z(0, 0, 7);

    ASSERT_EQ(TMatrix::makeFromPoints(x, y), TMatrix::rotate_z(M_PI_2));
    ASSERT_EQ(TMatrix::makeFromPoints(y, z), TMatrix::rotate_x(M_PI_2));
    ASSERT_EQ(TMatrix::makeFromPoints(z, x), TMatrix::rotate_y(M_PI_2));

    // длина векторов должна быть одинакова
    point3d p{1, 2, 3}, q{2, -3, 1};

    ASSERT_EQ(TMatrix::makeFromPoints(p, q) * p, q);
    ASSERT_EQ(TMatrix::makeFromPoints(q, p) * q, p);

    TMatrix fromXZ = TMatrix::makeFromPoints(x, z);

    ASSERT_EQ(fromXZ * point3d(100, 0, 0), point3d(0, 0, 100));
    ASSERT_EQ(fromXZ * point3d(0, 10, 0), point3d(0, 10, 0));
    ASSERT_EQ(fromXZ * point3d(0, 0, 20), point3d(-20, 0, 0));
}

TEST(tmatrixTest, makeFromPoints2_single)
{
    point3d a{+0.8660254, +0.500000};
    point3d b{-0.8660254, -0.500000};
    TMatrix tm = TMatrix::makeFromPoints(a, b);

    ASSERT_EQ(a, tm * b);
    ASSERT_EQ(b, tm * a);
}

TEST(tmatrixTest, makeFromPoints4_multi)
{
    typedef std::tuple<point3d, point3d> tpair;
    QList<tpair>                         list;

    double ar[] = {1, 1, 1, 1,             //
                   1, 1, 1, 1, 1, 1, 1, 1, //
                   1, 1, 1, 1, 1, 1, 1, 1, //
                   1, 1, 1, 1};

    list << std::make_tuple(point3d::nx * ar[0], point3d::ny * ar[1]);
    list << std::make_tuple(-point3d::nx * ar[2], point3d::ny * ar[3]);
    list << std::make_tuple(-point3d::nx * ar[4], -point3d::ny * ar[5]);
    list << std::make_tuple(point3d::nx * ar[6], -point3d::ny * ar[7]);

    list << std::make_tuple(point3d::nz * ar[8], point3d::ny * ar[9]);
    list << std::make_tuple(-point3d::nz * ar[10], point3d::ny * ar[11]);
    list << std::make_tuple(-point3d::nz * ar[12], -point3d::ny * ar[13]);
    list << std::make_tuple(point3d::nz * ar[14], -point3d::ny * ar[15]);

    list << std::make_tuple(point3d::nz * ar[16], point3d::nx * ar[17]);
    list << std::make_tuple(-point3d::nz * ar[18], point3d::nx * ar[19]);
    list << std::make_tuple(-point3d::nz * ar[20], -point3d::nx * ar[21]);
    list << std::make_tuple(point3d::nz * ar[22], -point3d::nx * ar[23]);

    // добавляем повторно все элементы, меняя местами первый и второй
    for(int index = list.size(); index-- > 0;)
    {
        list << std::make_tuple(std::get<1>(list[index]), std::get<0>(list[index]));
    }

    //    point3d s1=-point3d::ny;
    //    point3d s2=point3d::nx;
    //    point3d d1=-point3d::nz;
    //    point3d d2=point3d::ny;
    //    TMatrix t = TMatrix::makeFromPoints(s1, s2, d1, d2);

    int counter = 0;
    foreach(tpair pair0, list)
    {
        point3d s1 = std::get<0>(pair0);
        point3d s2 = std::get<1>(pair0);
        foreach(tpair pair1, list)
        {
            point3d d1 = std::get<0>(pair1);
            point3d d2 = std::get<1>(pair1);
            TMatrix t1 = TMatrix::makeFromPoints(s1, d1);
            ASSERT_EQ(s1 * t1, d1);

            TMatrix t2 = TMatrix::makeFromPoints(s2, d2);
            ASSERT_EQ(s2 * t2, d2);

            QString str = QString("from %1+%2 to %3+%4")
                              .arg(s1.toQString())
                              .arg(s2.toQString())
                              .arg(d1.toQString())
                              .arg(d2.toQString());

            TMatrix tm    = TMatrix::makeFromPoints(s1, s2, d1, d2);
            bool    check = s1 * tm == d1 && s2 * tm == d2;
            EXPECT_TRUE(check) << str.toStdString();
            if(!check)
                counter++;
        }
    }
    EXPECT_EQ(counter, 0);
}
