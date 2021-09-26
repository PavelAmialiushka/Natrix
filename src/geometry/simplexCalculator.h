#ifndef SIMPLEXCALCULATOR_H
#define SIMPLEXCALCULATOR_H

#include <complex>
#include <vector>

template <typename Coords>
struct ContextSample
{
    Coords focus(Coords *cs, int count)
    {
    }
    Coords interpolate(double gamma, Coords lhs, Coords rhs)
    {
    }
};

// using Sigma_t = [&](Coords c) -> double { return 0.0; }

#if __GNUC__ < 5 && __GNUC__ != 0
bool _isnan(double f)
{
    return f != f;
}
#endif

template <class context_t, class sigma_t, class coords_t>
class SimplexCalculator
{
    struct group
    {
        double   f;
        coords_t xy;

        group(double s, coords_t xy)
            : f(s)
            , xy(xy)
        {
        }

        bool operator<(group const &rhs) const
        {
            return f < rhs.f;
        }
    };

    context_t &context;
    sigma_t &  sigma; // метод расчета сигмы
    std::vector<group> vec; // группа точек, по которым идет расчет симплекса

public:
    SimplexCalculator(context_t &c, sigma_t &s)
        : sigma(s)
        , context(c)
    {
    }

    void addMeshPoint(coords_t xy)
    {
        double f = sigma(xy);
        if(!_isnan(f))
            vec.push_back(group(f, xy));
    }

    void selectBasePoints()
    {
        std::sort(STL_II(vec));

        // три точки лежащие на прямой
        while(vec.size() > 2)
        {
            if(context.isATriangle(vec[0].xy, vec[1].xy, vec[2].xy))
                break;

            // отбрасываем данную точку
            vec.erase(vec.begin() + 2);
        }

        // необходимо только три лучших попадания
        if(vec.size() > 3)
            vec.erase(vec.begin() + 3, vec.end());
    }

    void selectOnlyOneBasePoint()
    {
        if(!vec.size())
            return;

        std::sort(STL_II(vec));
        vec.erase(vec.begin() + 1, vec.end());

        coords_t a, b;
        context.make_simplex(vec[0].xy, a, b);
        addMeshPoint(a);
        addMeshPoint(b);
    }

    bool isCorrect() const
    {
        return vec.size() >= 3;
    }

    std::vector<coords_t> cs;
    void                  simplex_cycle(double alpha = 1.0, double beta = 0.5, double gamma = 2)
    {
        std::sort(vec.begin(), vec.end());

        cs.clear();
        cs.reserve(vec.size());

        // находим центр тяжести всех кроме худшей
        for(unsigned index = 0; index < vec.size() - 1; ++index)
            cs.push_back(vec[index].xy);
        coords_t c = context.focus(&cs[0], static_cast<int>(cs.size()));

        // отражение
        coords_t r  = context.interpolate(-alpha, c, vec[2].xy);
        double   rf = sigma(r);

        if(rf < vec[0].f)
        { // удачное направление
            // "сжатие"
            coords_t e  = context.interpolate(gamma, c, r);
            double   ef = sigma(e);

            if(ef < vec[0].f)
                vec[2] = group(ef, e);
            else
                vec[2] = group(rf, r);

            return;
        }
        else if(rf < vec[1].f)
        { // второе место
            vec[2] = group(rf, r);
            return;
        }
        else if(rf < vec[2].f)
        { // третье место
            vec[2] = group(rf, r);
        }

        // сжатие
        coords_t s  = context.interpolate(1 - beta, vec[2].xy, c);
        double   sf = sigma(s);
        if(sf < vec[2].f)
            vec[2] = group(sf, s);
        else
        { // все точки неудачны.
            // делаем глобальное сжатие к лучшей точке
            for(unsigned index = 1; index < 3; ++index)
            {
                vec[index].xy = context.interpolate(0.5, vec[0].xy, vec[index].xy);
            }
        }
    }

    int    actual_cycle;
    double actual_sigma;
    double actual_size;

    // запускаем симплекс метод
    bool process(double &  f,
                 coords_t &xy,
                 int *     step_count  = 0,
                 double    index_limit = 100,
                 double    sigma_limit = 1e-4,
                 double    size_limit  = 0.001)
    {
        double sigma = 0;

        int    index = 0;
        double size  = 0;
        if(step_count)
            *step_count = 0;

        bool result = false;
        for(;; ++index)
        {
            if(step_count)
                *step_count += 1;

            simplex_cycle();

            double avr = (vec[0].f + vec[1].f + vec[2].f) / 2;
            sigma = sqrt(pow(vec[0].f - avr, 2) + pow(vec[1].f - avr, 2) + pow(vec[2].f - avr, 2));
            if(sigma < sigma_limit)
                break;
            if(index > index_limit)
                break;
            if(index % 20 == 1)
            {
                size = (context.crude_distance(vec[0].xy, vec[1].xy) +
                        context.crude_distance(vec[1].xy, vec[2].xy) +
                        context.crude_distance(vec[0].xy, vec[2].xy)) /
                       3;
                if(size < size_limit)
                {
                    break;
                }
            }
        };

        actual_size  = size;
        actual_cycle = index;
        actual_sigma = sigma;

        f  = vec[0].f;
        xy = vec[0].xy;
        return result;
    }

    void report_statistics(int delta_t)
    {
        static int    total_count = 0;
        static int    total_time  = 0;
        static int    total_cicle = 0;
        static double total_sigma = 0;
        static double total_size  = 0;

        ++total_count;
        total_time += delta_t;
        total_cicle += actual_cycle;
        total_sigma += actual_sigma;
        total_size += actual_size;

        if(total_count % 100 == 0)
        {
            //            LOG(strlib::strf("time: %.3f, %.3f [msec]\n%d %.3f %.3f",
            //                             total_count,
            //                             total_time * 1.0f / total_count,
            //                             total_cicle / total_count,
            //                             total_sigma / total_count,
            //                             total_size / total_count));
        }
    }
};

template <class coords_t, class context_t, class sigma_t>
SimplexCalculator<context_t, sigma_t, coords_t> MakeSimplexCalculator(context_t &c, sigma_t &s)
{
    return SimplexCalculator<context_t, sigma_t, coords_t>(c, s);
}

#endif // SIMPLEXCALCULATOR_H
