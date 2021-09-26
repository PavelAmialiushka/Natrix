#include "exportDialog.h"
#include "ui_exportDialog.h"

#include "sceneWidgetDrawer.h"

#include <QClipboard>
#include <QFileDialog>
#include <QGraphicsView>
#include <QMessageBox>
#include <QSettings>

#include <geometry/canvasRectangle.h>
#include <geometry/document.h>

ExportDialog::ExportDialog(Scene *scene, QWidget *parent)
    : QDialog(parent)
    , scene_(scene)
    , ui(new Ui::ExportDialog)
{
    ui->setupUi(this);

    connect(ui->comboSize, SIGNAL(currentIndexChanged(QString)), SLOT(imageSizeChanged(QString)));

    connect(ui->pushClipboard, SIGNAL(clicked()), SLOT(exportToClipboard()));
    connect(ui->pushImageSaveAs, SIGNAL(clicked()), SLOT(exportToFile()));
    connect(ui->pushPDFSaveAs, SIGNAL(clicked(bool)), SLOT(exportToPDF()));

    // настройка списка страниц
    int canvasCount = scene->canvasCount();
    int bvc         = scene->bestVisibleCanvas() + 1;
    for(int index = 1; index < canvasCount + 1; ++index)
    {
        auto item = new QListWidgetItem;

        QPixmap  image{64, 64};
        QPainter paint(&image);
        paint.fillRect(QRect(0, 0, 64, 64), Qt::white);

        QFont font{"Tahoma", 48};
        paint.setFont(font);

        auto text = QString(QString::fromUtf8("%1").arg(index));
        paint.drawText(0, 0, 64, 64, Qt::AlignCenter, text);
        paint.end();

        item->setIcon(QIcon(image));
        ui->canvasNo->addItem(item);
        if(index == bvc)
        {
            ui->canvasNo->setCurrentItem(item);
            ui->canvasNo->selectionModel()->select(ui->canvasNo->currentIndex(),
                                                   QItemSelectionModel::Select);
        }
    }

    if(ui->canvasNo->selectionModel()->selectedIndexes().empty())
    {
        auto rootIndex = ui->canvasNo->rootIndex();
        ui->canvasNo->selectionModel()->select(rootIndex, QItemSelectionModel::Select);
    }

    QSettings sett;
    bool      ok    = 0;
    int       index = sett.value("export_resolution").toInt(&ok);
    if(ok && 0 <= index && index < ui->comboSize->count())
        ui->comboSize->setCurrentIndex(index);
    else
        ui->comboSize->setCurrentIndex(1);
    imageSizeChanged("");
}

QImage *ExportDialog::makeAnImage()
{
    int  dpi  = getCurrentDPI();
    auto info = scene_->canvas(0)->info();

    int dx = info.width(dpi);
    int dy = info.height(dpi);

    QImage *image = new QImage(dx, dy, QImage::Format_RGB32);
    QRect   imageRect{0, 0, dx, dy};

    image->fill(0xFFFFFF);

    int  canvasNo = 0;
    auto items    = ui->canvasNo->selectedItems();
    foreach(QListWidgetItem *item, items)
    {
        canvasNo = ui->canvasNo->row(item) + 1;
        break;
    }

    QPainter painter(image);
    scene_->render(&painter, 0, canvasNo);

    return image;
}

void ExportDialog::exportToClipboard()
{
    QScopedPointer<QImage> image(makeAnImage());

    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setImage(*image);

    this->close();
}

void ExportDialog::exportToFile()
{
    QScopedPointer<QImage> image(makeAnImage());

    QSettings sett;
    QString   defaultPath = sett.value("export_path", "c:\\").toString();

    QString fileName =
        QFileDialog::getSaveFileName(this,
                                     QString::fromUtf8("Экспортировать файл"), // caption
                                     defaultPath,
                                     QString::fromUtf8("PNG файлы(*.png)"));
    if(fileName.isEmpty())
        return;

    sett.setValue("export_path", QFileInfo(fileName).absolutePath());

    // сохранение
    if(image->save(fileName, "PNG"))
    {
        this->close();
        return;
    }

    QMessageBox msg;
    msg.setText(QString::fromUtf8("Ошибка при записи файла"));
    msg.setInformativeText(fileName);
    msg.setStandardButtons(QMessageBox::Retry | QMessageBox::Cancel);
    msg.setIcon(QMessageBox::Critical);
    int result = msg.exec();
    if(result == QMessageBox::Retry)
        return exportToFile();
}

void ExportDialog::exportToPDF()
{
    QSettings sett;
    QString   defaultPath = sett.value("export_path_pdf", "c:\\").toString();

    QFileInfo info = scene_->document()->fileName();
    if(info.path().size() > 1) // WTF: "." для пустого пути
        defaultPath = info.path() + "\\" + info.baseName();

    QString fileName = QFileDialog::getSaveFileName(this,
                                                    QString::fromUtf8("Экспортировать PDF файл"),
                                                    defaultPath,
                                                    QString::fromUtf8("PDF файлы(*.pdf)"));
    if(fileName.isEmpty())
        return;

    sett.setValue("export_path_pdf", QFileInfo(fileName).absolutePath());

    QPrinter printer(QPrinter::PrinterResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOrientation(QPrinter::Landscape);
    printer.setPaperSize(QPrinter::A4);
    printer.setOutputFileName(fileName);

    QPainter painter;
    bool     ready = painter.begin(&printer);
    if(ready)
    {
        scene_->render(&painter, &printer);
        painter.end();

        this->close();
    }
    else
    {
        QMessageBox msg;
        msg.setText(QString::fromUtf8("Ошибка при записи PDF файла"));
        msg.setInformativeText(fileName);
        msg.setStandardButtons(QMessageBox::Retry | QMessageBox::Cancel);
        msg.setIcon(QMessageBox::Critical);
        int result = msg.exec();
        if(result == QMessageBox::Retry)
            return exportToPDF();
    }
}

int ExportDialog::getCurrentDPI()
{
    unsigned index  = ui->comboSize->currentIndex();
    int      dpis[] = {100, 175, 250};
    index           = qMin(index, static_cast<unsigned>(sizeof(dpis) / sizeof(*dpis) - 1));
    return dpis[index];
}

void ExportDialog::imageSizeChanged(QString)
{
    QSettings sett;
    sett.setValue("export_resolution", ui->comboSize->currentIndex());

    int  dpi  = getCurrentDPI();
    auto info = scene_->canvas(0)->info();
    int  w    = info.width(dpi);
    int  h    = info.height(dpi);
    ui->lineSize->setText(QString("%1x%2").arg(w).arg(h));
    ui->lineDPI->setText(QString("%1 dpi").arg(dpi));
}

ExportDialog::~ExportDialog()
{
    delete ui;
}
