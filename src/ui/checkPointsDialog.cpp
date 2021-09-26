#include "checkPointsDialog.h"
#include "ui_checkPointsDialog.h"

#include "geometry/textLabel.h"
#include "geometry/utilites.h"

#include <QKeyEvent>
#include <QPlainTextEdit>

using namespace geometry;

CheckPointsDialog::CheckPointsDialog(Scene *scene, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::CheckPointsDialog)
    , scene_(scene)
{
    ui->setupUi(this);

    ui->tableWidget->verticalHeader()->hide();
    ui->tableWidget->horizontalHeader()->hide();
    ui->tableWidget->setColumnCount(2);
    ui->tableWidget->setColumnWidth(0, 60);
    ui->tableWidget->setColumnWidth(1, 400);

    QMap<int, QString> map = analyzeLabels();
    foreach(int count, map.uniqueKeys())
    {
        int row = ui->tableWidget->rowCount();
        ui->tableWidget->insertRow(row);
        ui->tableWidget->setItem(row, 0, new QTableWidgetItem(QString("%1").arg(count)));
        ui->tableWidget->setItem(row, 1, new QTableWidgetItem(map[count]));
    }

    if(map.isEmpty())
    {
        ui->tableWidget->insertRow(0);
        ui->tableWidget->setItem(0, 0, new QTableWidgetItem(""));
        ui->tableWidget->setItem(0, 1, new QTableWidgetItem("-----------"));
    }
}

CheckPointsDialog::~CheckPointsDialog()
{
    delete ui;
}

QList<int> CheckPointsDialog::searchLabels()
{
    QList<int> labels;
    QRegExp    number{QString("\\d+")};

    foreach(auto label, utils::filter_transform<PTextLabel>(scene_->labels(), toText))
    {
        if(number.exactMatch(label->info().text))
            labels << QString(label->info().text).toInt();
    }

    qSort(labels);
    return labels;
}

void CheckPointsDialog::on_closeButton_clicked()
{
    close();
}

static QString convertListToString(QList<int> pts)
{
    qSort(pts);

    QList<QString> result;
    int            start = 0;
    for(; start < pts.size();)
    {
        // ищем номер, до которого последовательность не нарушается
        int last = start + 1;
        for(; last < pts.size() && pts[last] == pts[last - 1] + 1; ++last)
            ;
        --last;

        if(last - start >= 2)
            result << QString("%1..%2").arg(pts[start]).arg(pts[last]);
        else
            for(int index = start; index <= last; ++index)
                result << QString("%1").arg(pts[index]);

        start = last + 1;
    }

    return result.join(", ") + QString(" (%1)").arg(pts.size());
}

QMap<int, QString> CheckPointsDialog::analyzeLabels()
{
    QMap<int, QString>   result;
    QMap<int, QSet<int>> count2point;

    QList<int> points = searchLabels();
    while(points.size())
        if(points.last() > 250)
            points.takeLast();
        else
            break;

    QSet<int> pointSet = points.toSet();

    // заполняем строчку без значений
    QList<int> nullList;
    int        maxp = 0;
    if(points.size())
        maxp = *std::max_element(points.begin(), points.end());

    for(int index = 1; index <= maxp; ++index)
        if(pointSet.find(index) == pointSet.end())
            nullList << index;
    if(nullList.size())
        result[0] = convertListToString(nullList);

    // собираем значения
    foreach(int point, pointSet)
    {
        int count = std::count(points.begin(), points.end(), point);
        count2point[count].insert(point);
    }

    // заполняем строчки со значениями
    foreach(int count, count2point.keys())
    {
        QList<int> pts = count2point[count].toList();
        result[count]  = convertListToString(pts);
    }

    return result;
}
