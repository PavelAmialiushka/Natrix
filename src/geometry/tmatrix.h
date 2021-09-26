#ifndef TMATRIX_H
#define TMATRIX_H

#include <algorithm>
#include <array>
#include <initializer_list>

#include "point.h"

namespace geometry
{

class TMatrix
{
    std::array<std::array<double, 3>, 3> data;

public:
    TMatrix();
    TMatrix(std::initializer_list<double> lst);
    TMatrix(std::initializer_list<std::initializer_list<double>> lst);

    static TMatrix rotate_x(double angle);
    static TMatrix rotate_y(double angle);
    static TMatrix rotate_z(double angle);

    static TMatrix rotateOver(point3d v, double cosa);
    static TMatrix makeFromPoints(point3d const &source, point3d const &dest);
    static TMatrix makeFromPoints(point3d const &source1,
                                  point3d const &source2,
                                  point3d const &dest1,
                                  point3d const &dest2);

    bool operator==(TMatrix const &rhs) const;
    bool operator!=(TMatrix const &rhs) const;

    TMatrix &operator*=(TMatrix const &other);

    std::array<double, 3>  operator[](int index) const;
    std::array<double, 3> &operator[](int index);

    TMatrix transpose() const;
};

// friend
TMatrix operator*(TMatrix const &, TMatrix const &);
point3d operator*(point3d const &point, TMatrix const &matrix);
point3d operator*(TMatrix const &matrix, point3d const &point);

std::ostream &operator<<(std::ostream &os, TMatrix p);

} // namespace geometry

#endif // TMATRIX_H
