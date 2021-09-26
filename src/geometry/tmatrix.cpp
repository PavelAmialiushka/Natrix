#include "tmatrix.h"

#include <QDebug>

#include <math.h>

namespace geometry
{

TMatrix::TMatrix()
{
}

// используем правую тройку, это значит, оси x, y, z соответствую пальцам
// большому, указательному и среднему для правой руки, а поворот на угол >0
// соответствует повороту против часовой стрелки

TMatrix TMatrix::rotate_x(double angle)
{
    double ca = cos(angle);
    double sa = sin(angle);
    return TMatrix{1, 0, 0, 0, ca, -sa, 0, sa, ca};
}

TMatrix TMatrix::rotate_y(double angle)
{
    double ca = cos(angle);
    double sa = sin(angle);
    return TMatrix{ca, 0, sa, 0, 1, 0, -sa, 0, ca};
}

TMatrix TMatrix::rotate_z(double angle)
{
    double ca = cos(angle);
    double sa = sin(angle);
    return TMatrix{ca, -sa, 0, sa, ca, 0, 0, 0, 1};
}

TMatrix::TMatrix(std::initializer_list<double> lst)
{
    Q_ASSERT(lst.size() == 9);
    auto first = lst.begin();
    for(int i = 0; i < 3; ++i)
        for(int j = 0; j < 3; ++j)
            data[j][i] = *first++;
}

TMatrix::TMatrix(std::initializer_list<std::initializer_list<double>> lst)
{
    Q_ASSERT(lst.size() == 3);
    auto *foo = lst.begin();
    for(int i = 0; i < 3; ++i)
    {
        auto *bar = foo->begin();
        for(int j = 0; j < 3; ++j)
            data[i][j] = *bar++;

        ++foo;
    }
}

bool TMatrix::operator==(TMatrix const &rhs) const
{
    const double eps = 0.00001;
    for(int i = 0; i < 3; ++i)
        for(int j = 0; j < 3; ++j)
            if(abs(data[i][j] - rhs.data[i][j]) > eps)
                return false;
    return true;
}

bool TMatrix::operator!=(TMatrix const &rhs) const
{
    return !operator==(rhs);
}

std::array<double, 3> TMatrix::operator[](int index) const
{
    return data[index];
}

std::array<double, 3> &TMatrix::operator[](int index)
{
    return data[index];
}

point3d operator*(point3d const &point, TMatrix const &matrix)
{
    Q_ASSERT(!std::isnan(point.x * matrix[0][0] + point.y * matrix[1][0] + point.z * matrix[2][0]));
    return point3d(point.x * matrix[0][0] + point.y * matrix[1][0] + point.z * matrix[2][0],
                   point.x * matrix[0][1] + point.y * matrix[1][1] + point.z * matrix[2][1],
                   point.x * matrix[0][2] + point.y * matrix[1][2] + point.z * matrix[2][2]);
}

point3d operator*(TMatrix const &matrix, point3d const &point)
{
    return point * matrix;
}

TMatrix operator*(TMatrix const &a, TMatrix const &b)
{
    // перемножение матриц
    TMatrix result;
    for(int i = 3; i-- > 0;)
    {
        for(int j = 3; j-- > 0;)
        {
            double s = 0;
            for(int k = 3; k-- > 0;)
            {
                s += a[i][k] * b[k][j];
            }
            result[i][j] = s;
        }
    }
    return result;
}

TMatrix &TMatrix::operator*=(TMatrix const &other)
{
    return (*this) = (*this) * other;
}

std::ostream &operator<<(std::ostream &os, TMatrix p)
{
    auto line = [](std::array<double, 3> line) {
        return QString("%1;%2;%3}")
            .arg(line[0], 0, 'f', 4)
            .arg(line[1], 0, 'f', 4)
            .arg(line[2], 0, 'f', 4);
    };
    return os << QString("{%1}-{%2}-{%3}")
                     .arg(line(p[0]))
                     .arg(line(p[1]))
                     .arg(line(p[2]))
                     .toStdString();
}

TMatrix TMatrix::transpose() const
{
    TMatrix const &self = *this;
    return TMatrix{self[0][0],
                   self[0][1],
                   self[0][2],
                   self[1][0],
                   self[1][1],
                   self[1][2],
                   self[2][0],
                   self[2][1],
                   self[2][2]};
}

TMatrix TMatrix::rotateOver(point3d v, double cosa)
{
    // вращение вокруг единичного вектора на зажанный угол
    Q_ASSERT(v == v.normalized());
    Q_ASSERT(fabs(cosa) <= 1.0);

    double sina = sqrt(1 - cosa * cosa);
    Q_ASSERT(!std::isnan(sina) && !std::isnan(cosa));

    // http://ru.vlab.wikia.com/wiki/Матрица_поворота
    return TMatrix{
        cosa + (1 - cosa) * v.x * v.x,
        (1 - cosa) * v.x * v.y - sina * v.z,
        (1 - cosa) * v.x * v.z + sina * v.y,
        (1 - cosa) * v.y * v.x + sina * v.z,
        cosa + (1 - cosa) * v.y * v.y,
        (1 - cosa) * v.y * v.z - sina * v.x,
        (1 - cosa) * v.z * v.x - sina * v.y,
        (1 - cosa) * v.z * v.y + sina * v.x,
        cosa + (1 - cosa) * v.z * v.z,
    };
}

static point3d selectNormal(point3d const &s, point3d const &d)
{
    // определяем ось вращения, вокруг которой будем вращать
    point3d v = s.cross(d);
    if(v)
        return v;

    // вектора параллельны, значит
    // подойдет любой вектор, перпендикулярный им

    // попробуем плясать от оси x
    v = point3d::nx.cross(s);
    if(v)
        return v;

    // параллелен оси x, значит берем ось y
    return point3d::ny;
}

// угол между векторами
TMatrix TMatrix::makeFromPoints(point3d const &s, point3d const &d)
{
    Q_ASSERT(fabs(s.length() - d.length()) < POINT_PREC * POINT_PREC);

    // определяем ось вращения
    point3d v = selectNormal(s, d);

    // определяем угол поворота
    // загадочный глюк со значением cosa = 1.000000002
    double cosa = std::max(-1.0, std::min(1.0, s.cosaTo(d)));

    // определяем матрицу поворота
    return rotateOver(v.normalized(), cosa);
}

TMatrix TMatrix::makeFromPoints(point3d const &s1,
                                point3d const &s2,
                                point3d const &d1,
                                point3d const &d2)
{
    // матрица поворота должна совместить вектора s1 и s2 с векторами d1 и d2
    Q_ASSERT(std::abs(s1.angleTo(s2) - d1.angleTo(d2)) < POINT_PREC);

    // если s1 и s2 параллельны, то ситуация упрощается
    if(s1.isParallel(s2))
        return makeFromPoints(s1, d1);

    // первая ступень вращения сводит вместе первую пару векторов
    TMatrix result1 = makeFromPoints(s1, d1);

    // при этом второй исходный вектор становится таким
    point3d ts2 = s2 * result1;

    // угол на который нужно повернуть
    // загадочный глюк со значением cosa = 1.000000002
    double cosa = std::max(-1.0, std::min(1.0, ts2.cosaTo(d2)));

    // в качестве оси используем ось d1, однако мы знаем наверняка
    // с каким знаком ее нужно брать. поэтому выполняем проверку:
    // если ось вращения направлена в противоположную сторону
    // меняем знак вращения на противоположный
    double sign = (d1.dot(ts2.cross(d2)) < 0) ? -1 : 1;

    // выполняем вращение
    TMatrix result2 = TMatrix::rotateOver(d1 * sign, cosa);
    return result1 * result2;
}

} // namespace geometry
