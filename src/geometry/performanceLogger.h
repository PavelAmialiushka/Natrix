#ifndef PERFORMANCETIMER_H
#define PERFORMANCETIMER_H

#include <QDebug>
#include <QElapsedTimer>
#include <QString>

template <int X = 0>
class PerformanceLogger
{
    struct Data
    {
        int                 executionCount;
        int                 showCount;
        QList<qint64>       inProcTime;
        QList<int>          inProcCounter;
        qint64              inProcTimeTotal;
        qint64              outOfProcTime;
        QElapsedTimer       timer;
        QElapsedTimer       totalTimer;
        QList<const char *> wpNames;

        void reset()
        {
            executionCount  = 0;
            showCount       = 1;
            outOfProcTime   = 0;
            inProcTimeTotal = 0;
            timer.restart();
            totalTimer.restart();

            inProcCounter.clear();
            inProcTime.clear();
            for(int counter = 11; counter-- > 0;)
            {
                inProcTime << 0;
                inProcCounter << 0;
            }

            int counter = 11;
            while(wpNames.size() < counter)
                wpNames << "";
        }

        Data()
        {
            reset();
        }
    };

    Data &data()
    {
        static Data d;
        return d;
    }

    int         inCounter_;
    const char *name_;
    bool        active_;

public:
    PerformanceLogger(const char *name = "", bool active = true)
        : name_(name)
        , active_(active)
    {
        if(active_)
            start();
    }

    ~PerformanceLogger()
    {
        if(active_)
            finish();
    }

    void start()
    {
        Data &self = data();

        inCounter_ = 0;
        ++self.executionCount;

        self.outOfProcTime += self.timer.nsecsElapsed();
        self.timer.restart();
    }

    void waypoint(int index)
    {
        if(!active_)
            return;
        Data &self = data();

        auto time = self.timer.nsecsElapsed();
        self.inProcTime[index] += time;
        self.inProcTimeTotal += time;
        self.inProcCounter[index] += 1;

        self.timer.restart();
        inCounter_ = 1 + qMax(index, inCounter_);
    }

    void waypoint(const char *name = "")
    {
        if(!active_)
            return;
        Data &self = data();

        self.wpNames[inCounter_ + 1] = name;
        waypoint(inCounter_);
    }

    void finish()
    {
        Data &self = data();

        waypoint(10);
        if(/*self.inProcTimeTotal > 5e+9 * self.showCount*/
           self.totalTimer.elapsed() > 5.0e3)
        {
            ++self.showCount;
            report();
            self.reset();
            self.totalTimer.restart();
        }
        self.timer.restart();
    }

    void report()
    {
        Data &self = data();

        qDebug() << name_;
        qDebug() << QString("  count: %1").arg(self.executionCount);
        qDebug() << QString(" inproc: %1 (%2 ms per call)")
                        .arg(self.inProcTimeTotal / 1.0e9, 0, 'f', 3)
                        .arg(self.inProcTimeTotal / 1.0e6 / self.executionCount, 0, 'f', 6);

        for(int index = 0; index < self.inProcTime.size(); ++index)
        {
            if(self.inProcCounter[index] == 0)
                continue;

            QString name;
            if(index == 0)
                name = "prolog";
            else if(index == 10)
                name = "rest";
            else
                name = self.wpNames[index];

            qDebug() << QString("    wp%1: %2 [%3] (%4 ms per call) %5%% [%6]")
                            .arg(index, 2, 10, QChar('0'))
                            .arg(self.inProcTime[index] / 1.0e9, 0, 'f', 3)
                            .arg(self.inProcCounter[index], 4)
                            .arg(self.inProcTime[index] / 1.0e6 / self.executionCount, 0, 'f', 6)
                            .arg(self.inProcTime[index] * 100.0 / self.inProcTimeTotal, 5, 'f', 2)
                            .arg(name);
        }
        qDebug() << QString(" ouproc: %1").arg(self.outOfProcTime / 1.0e9, 0, 'f', 3);
        qDebug() << QString("   perc: %1%%")
                        .arg(100.0 * self.inProcTimeTotal /
                                 (self.inProcTimeTotal + self.outOfProcTime),
                             0,
                             'f',
                             6);
    }
};

#endif // PERFORMANCETIMER_H
