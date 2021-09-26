#pragma once

#include <cmath>

#include <QString>
#include <QtGlobal>

#include <memory>
#include <ostream>
#include <stdexcept>

#include <QPoint>

namespace geometry
{

const double POINT_PREC = 1e-3;

template <int N>
class point_t
{
public:
    static const bool is3d = (N == 3);
    double            x, y, z;

public:
    typedef point_t<N> pointNd;

    point_t()
        : x(0)
        , y(0)
        , z(0)
    {
    }

    point_t(double x, double y = 0.0, double z = 0.0)
        : x(x)
        , y(y)
        , z(z)
    {
    }

    /////////////////////////////////////////////////////////////////////////
    // запросы о состоянии
    /////////////////////////////////////////////////////////////////////////

    // длина вектора
    double length() const
    {
        return sqrt(x * x + y * y + (is3d ? z * z : 0.0));
    }

    // приведение к единичной длине
    pointNd normalized(bool *ok = 0) const
    {
        if(empty())
        {
            if(ok)
                return *ok = 0, pointNd();

            Q_ASSERT(!"normalizing empty vector");
            throw std::runtime_error("normalizing empty vector");
        }
        if(ok)
            *ok = true;
        return operator/(length());
    }

    pointNd resized(double d, bool *ok = 0) const
    {
        if(empty())
        {
            if(ok)
                return *ok = 0, pointNd();

            Q_ASSERT(!"normalizing empty vector");
            throw std::runtime_error("normalizing empty vector");
        }
        if(ok)
            *ok = true;
        return operator*(d / length());
    }

    // равен ли вектор нулю
    bool empty() const
    {
        return std::abs(x) < POINT_PREC && std::abs(y) < POINT_PREC &&
               (!is3d || std::abs(z) < POINT_PREC);
    }

    typedef bool (pointNd::*RestrictedBool)() const;
    operator RestrictedBool() const
    {
        return !empty() ? &pointNd::empty : 0;
    }

    // равенство
    // проверка на равенство
    bool operator==(pointNd const &rhs) const
    {
        return fabs(x - rhs.x) < POINT_PREC && fabs(y - rhs.y) < POINT_PREC &&
               (!is3d || fabs(z - rhs.z) < POINT_PREC);
    }

    bool isParallel(pointNd const &rhs) const
    {
        return cross(rhs).empty();
    }

    bool isNormal(pointNd const &rhs) const
    {
        return fabs(dot(rhs)) < POINT_PREC * POINT_PREC;
    }

    bool isCoaimed(pointNd const &rhs) const
    {
        if(empty() && rhs.empty())
            return true;
        if(empty() || rhs.empty())
            return false;
        return normalized() == rhs.normalized();
    }

    bool operator!=(pointNd const &rhs) const
    {
        return !(*this == rhs);
    }

    // for use as QMap key
    bool operator<(pointNd const &rhs) const
    {
        return fabs(x - rhs.x) >= POINT_PREC           ? x < rhs.x
               : fabs(y - rhs.y) >= POINT_PREC         ? y < rhs.y
               : fabs(z - rhs.z) >= POINT_PREC && is3d ? z < rhs.z
                                                       : 0;
    }

    bool isAxeParallel() const
    {
        bool bx = fabs(x) > POINT_PREC, by = fabs(y) > POINT_PREC, bz = fabs(z) > POINT_PREC;

        return (bx && !by && !bz) || (by && !bx && !bz) || (bz && !bx && !by);
    }

    /////////////////////////////////////////////////////////////////////////
    // операции
    /////////////////////////////////////////////////////////////////////////

    // получение информации об относительном положении двух точек
    // расстояние до указанной точки
    double distance(pointNd const &rhs) const
    {
        double xx = rhs.x - x;
        double yy = rhs.y - y;
        double zz = rhs.z - z;
        return sqrt(xx * xx + yy * yy + (is3d ? zz * zz : 0.0));
    }

    // операции с точкой
    // вычитание векторов
    pointNd operator+(pointNd const &rhs) const
    {
        return pointNd(x + rhs.x, y + rhs.y, z + rhs.z);
    }
    // сложение векторов
    pointNd operator-(pointNd const &rhs) const
    {
        return pointNd(x - rhs.x, y - rhs.y, z - rhs.z);
    }

    // обратный вектор
    pointNd operator-() const
    {
        return pointNd(-x, -y, -z);
    }

    // сложение
    pointNd &operator+=(pointNd const &rhs)
    {
        x += rhs.x;
        y += rhs.y;
        z += rhs.z;
        return *this;
    }

    // сложение
    pointNd &operator-=(pointNd const &rhs)
    {
        x -= rhs.x;
        y -= rhs.y;
        z -= rhs.z;
        return *this;
    }

    // умножение и деление на скаляр
    friend pointNd operator*(double k, pointNd const &pt)
    {
        return pointNd(k * pt.x, k * pt.y, k * pt.z);
    }

    // умножение на скаляр
    pointNd operator*(double k) const
    {
        return pointNd(k * x, k * y, k * z);
    }

    // деление на скаляр
    pointNd operator/(double k) const
    {
        if(k == 0)
        {
            Q_ASSERT(!"dividing by zero");
            throw std::runtime_error("division by zero");
        }

        return pointNd(x / k, y / k, z / k);
    }

    // скалярное умножение
    double dot(pointNd const &rhs) const
    {
        return x * rhs.x + y * rhs.y + (is3d ? z * rhs.z : 0);
    }

    // векторное умножение
    pointNd cross(pointNd const &rhs) const
    {
        static_assert(is3d, "cross multiplication make no sense in 2d");
        return pointNd(y * rhs.z - z * rhs.y, z * rhs.x - x * rhs.z, x * rhs.y - y * rhs.x);
    }

    /// деление отрезка
    double fraction_at_section(pointNd const &lhs, pointNd const &rhs, bool *ok = 0) const
    {
        if(ok)
        {
            if(lhs == rhs)
                return *ok = 0, 0;
            *ok = true;
        }
        auto f = distance(lhs) / lhs.distance(rhs);
        if((lhs - *this).dot(lhs - rhs) < 0)
            return -f;
        return f;
    }

    /// деление
    double fraction_at_projection(pointNd const &lhs, pointNd const &rhs, bool *ok = 0) const
    {
        if(ok)
        {
            if(!rhs)
                return *ok = false, 0;
            *ok = true;
        }
        auto x = project_to_line(lhs, rhs);
        return x.fraction_at_section(lhs, rhs, ok);
    }

    // опредление точки пересечения
    pointNd partition_by(pointNd const &rhs, double frac) const
    {
        return operator+((rhs - *this) * frac);
    }

    // проекция точки на заданное направление
    pointNd project_to_direction(pointNd const &dir, bool *ok = 0) const
    {
        auto c2 = dir.dot(dir);
        if(ok)
        {
            if(abs(c2) < POINT_PREC * POINT_PREC)
                return *ok = false, 0;
            *ok = true;
        }
        else
        {
            Q_ASSERT(c2 && "cannot project to zero direction");
        }

        return dir * (dot(dir) / c2);
    }

    double project_and_make_coordinate(pointNd const &dir, bool *ok = 0) const
    {
        pointNd w(x, y, z);
        double  c1 = w.dot(dir);
        double  c2 = dir.dot(dir);

        if(!ok)
            Q_ASSERT(c2 && "cannot project to zero direction");
        else
        {
            if(c2 == 0)
                return *ok = false, 0;
            else
                *ok = true;
        }
        return c1 / c2;
    }

    // проекция точки на прямую
    pointNd project_to_line(pointNd a, pointNd b) const
    {
        pointNd pt(x, y, z);
        pointNd dir = b - a;
        pointNd w   = pt - a;
        double  c1  = w.dot(dir);
        double  c2  = dir.dot(dir);

        if(c2 == 0)
            return a;

        return a + dir * (c1 / c2);
    }

    // проекция точки на оторезок
    pointNd project_to_segment(pointNd a, pointNd b) const
    {
        pointNd pt(x, y, z);
        pointNd dir = b - a;
        pointNd w   = pt - a;
        double  c1  = w.dot(dir);
        double  c2  = dir.dot(dir);

        // если за пределами отрезка, возвращаем начало луча
        if(c1 <= 0)
            return a;
        if(c2 <= c1)
            return b;

        // это может быть, если v==point(0), т.е. прямая не задана
        if(c2 == 0)
            return a;

        return a + dir * (c1 / c2);
    }
    // проекция точки на луч
    pointNd project_to_ray(pointNd start, pointNd dir) const
    {
        pointNd pt(x, y, z);
        pointNd w  = pt - start;
        double  c1 = w.dot(dir);
        double  c2 = dir.dot(dir);

        // если за пределами _луча_, возвращаем начало луча
        if(c1 <= 0)
            return start;

        // это может быть, если v==point(0), т.е. прямая не задана
        if(c2 == 0)
        {
            Q_ASSERT(c2 && "cannot project to zero direction");
            return start;
        }

        return start + dir * (c1 / c2);
    }

    pointNd polar_to(pointNd point, double len, bool *ok = 0)
    {
        if(ok)
        {
            if(point == *this)
                return *ok = 0, *this;
            *ok = true;
        }
        Q_ASSERT(point != *this);
        auto r = point - (*this);
        auto n = r.normalized(ok);

        return (*this) + n * len;
    }

    pointNd scale(pointNd center, double scale) const
    {
        auto r = (*this) - center;
        return center + r * scale;
    }

    double cosaTo(pointNd point, bool *ok_r = 0) const
    {
        bool   ok, ok2;
        double result = normalized(&ok).dot(point.normalized(&ok2));

        if(ok_r && !ok2)
        {
            Q_ASSERT(!"incorrent parameters");
            *ok_r = false;
        }
        // возможна ситуация, когда cosa из-за ошибок
        // округления становится чуть-чуть больше 1.0
        return std::min(result, 1.0);
    }

    /// \brief определяет угол плоского вектора относительно оси ординат
    /// \param ok - флаг успешности, неуспешен для нулевого вектора
    ///
    double angle(bool *ok = 0) const
    {
        static_assert(!is3d, "make no sense in 3d");
        if(ok)
        {
            if(fabs(x) < POINT_PREC * POINT_PREC && fabs(y) < POINT_PREC * POINT_PREC)
            {
                return *ok = false, 0;
            }
            *ok = true;
        }

        return ::atan2(y, x);
    }

    /// \brief угол между векторами a и b принимая текущую точку за центр
    /// \param a - первый вектор
    /// \param b - второй вектор
    /// \param ok - флаг успешности, неуспешен, если точка а либо точка
    ///             b совпадает с текущей точкой
    double angleBetween(pointNd a, pointNd b, bool *ok = 0)
    {
        return (a - *this).angleTo(b - *this, ok);
    }

    /// \brief определяет угол между векторами (*this) и (point)
    ///        принимая за цент точку (0,0)
    /// \param point второй вектор
    /// \param ok - флаг успешности выполнения, неуспешен
    ///
    double angleTo(pointNd point, bool *ok = 0) const
    {
        if(ok)
        {
            if(empty() || point.empty())
                return *ok = 0, 0;
            *ok = true;
        }
        Q_ASSERT(!empty() && !point.empty());
        return acos(dot(point) / length() / point.length());
    }

    /// \brief вращение в плоскости вокруг точки center на угол angle
    ///
    pointNd rotate2dAround(pointNd center, double angle) const
    {
        static_assert(!is3d, "works only for 2d");
        pointNd r = *this - center;
        double  l = r.length();
        double  a = !r.empty() ? ::atan2(r.y, r.x) : 0;

        return center + pointNd(l * cos(a + angle), l * sin(a + angle));
    }

    /// \brief вращение вокруг оси, заданной исходной точкой и направлением
    /// на заданный угол
    /// \param start - начальная точка оси
    /// \param axe - направление оси вращения
    /// \param angle - угол вращения, выраженный в радианах
    pointNd rotate3dAround(pointNd start, pointNd axe, double angle) const
    {
        static_assert(is3d, "works only for 3d");
        pointNd center = project_to_line(start, start + axe);
        pointNd r      = (*this) - center;
        pointNd r_dop  = axe.cross(r);
        if(r.empty() || r_dop.empty())
            return *this;
        return center +
               (r.normalized() * cos(angle) + r_dop.normalized() * sin(angle)).resized(r.length());
    }

    // растояние от точки до отрезка
    double distance_to_line(pointNd a, pointNd b) const
    {
        pointNd pt(x, y, z);
        pointNd w  = pt - a;
        pointNd v  = b - a;
        double  c1 = w.dot(v);
        double  c2 = v.dot(v);

        if(c1 <= 0)
            return distance(a); // точка за участком a
        if(c2 <= c1)
            return distance(b); // точка за участком b

        return distance(a + v * (c1 / c2));
    }

    // растояние до проекции точки на отрезок
    double distance_to_segment(pointNd a, pointNd b) const
    {
        pointNd pt(x, y, z);
        pointNd dir = b - a;
        pointNd w   = pt - a;
        double  c1  = w.dot(dir);
        double  c2  = dir.dot(dir);

        // если за пределами отрезка, возвращаем начало луча
        if(c1 <= 0)
            return distance(a);
        if(c2 <= c1)
            return distance(b);

        // это может быть, если v==point(0), т.е. прямая не задана
        if(c2 == 0)
            return distance(a);

        return distance(a + dir * (c1 / c2));
    }

    // проверяем лежит ли точка на прямой
    bool lays_on_line(pointNd a, pointNd b) const
    {
        return (b - a).isParallel(b - (*this));
    }

    static pointNd fromQPoint(QPointF);

    QString toQString() const;
    QPointF toQPoint() const;

    static pointNd serialLoad(QString, bool *ok = nullptr);
    QString        serialSave();

    static const pointNd n;
    static const pointNd nx;
    static const pointNd ny;
    static const pointNd nz;
};

struct toQPoint
{
    template <class T>
    static QPointF convert(T p)
    {
        return p.toQPoint();
    }
};

template <int N>
void PrintTo(point_t<N> const &p, std::ostream *s)
{
    (*s) << p.toQString().toStdString();
};

template <int N, class T>
auto operator>>(point_t<N> const &self, T const &cvt) -> decltype(cvt.convert(point_t<N>()))
{
    return cvt.convert(self);
}

template <int N, class T>
auto operator*(point_t<N> const &self, T const &cvt) -> decltype(cvt.convert(point_t<N>()))
{
    return cvt.convert(self);
}

// extern template class point_t<2>;
// extern template class point_t<3>;

typedef point_t<2> point2d;
typedef point_t<3> point3d;

uint qHash(point2d const &);
uint qHash(point3d const &);

} // namespace geometry
