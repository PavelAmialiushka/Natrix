#ifndef UPDATECHECKER_H
#define UPDATECHECKER_H

#include <QObject>

class UpdateChecker : public QObject
{
    Q_OBJECT

    bool forced = false;

public:
    static void Start();
    static void StartIfNeeded();

    UpdateChecker(bool forced, QObject *parent = 0);

private:
    void showErrorDialog();
    bool showNewVersionDialog(QString web);
    void showLastVersionDialog();

private slots:

    void checkUpdate();
    void checkUpdate(QString web);
};

#endif // UPDATECHECKER_H
