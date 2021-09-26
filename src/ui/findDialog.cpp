#include "findDialog.h"
#include "ui_FindDialog.h"

#include "geometry/textLabel.h"
#include "geometry/utilites.h"

#include <QKeyEvent>
#include <QPlainTextEdit>

using namespace geometry;

FindDialog::FindDialog(Scene *scene, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::FindDialog)
    , scene_(scene)
{
    ui->setupUi(this);
    ui->textEdit->installEventFilter(this);
    ui->textEdit->setTabChangesFocus(true);

    // ограничиваем высоту
    QFontMetrics m{ui->textEdit->font()};
    QMargins     margins = ui->textEdit->contentsMargins();
    ui->textEdit->setFixedHeight(
        m.lineSpacing() +
        2 * (ui->textEdit->document()->documentMargin() + ui->textEdit->frameWidth()) +
        margins.top() + margins.bottom());
}

FindDialog::~FindDialog()
{
    delete ui;
}

void FindDialog::on_textEdit_textChanged()
{
    QString t       = ui->textEdit->toPlainText();
    bool    replace = false;
    if(t.contains("@"))
    {
        t.replace("@", QChar(216));
        replace = true;
    }

    if(t.startsWith("\""))
    {
        t.replace("\"", QChar(216));
        replace = true;
    }

    if(replace)
    {
        auto tc = ui->textEdit->textCursor();
        if(tc.selectedText().isEmpty())
        {
            int p = tc.position();
            ui->textEdit->setPlainText(t);

            tc.setPosition(p);
            ui->textEdit->setTextCursor(tc);
        }
    }

    int count = searchLabels().count();
    ui->countLabel->setText(QString("%1").arg(count));
}

void FindDialog::on_findNextButton_clicked()
{
    QSet<PObjectToSelect> set;
    foreach(auto label, searchLabels())
        set << label;

    scene_->clearSelection();
    scene_->addSelectedObjects(set);

    scene_->updatePropertiesAfterSceneChange();

    close();
}

QList<PTextLabel> FindDialog::searchLabels()
{
    QString text  = ui->textEdit->toPlainText();
    bool    exact = ui->exactMatch->isChecked();

    if(text.indexOf("//") >= 0)
    {
        exact = true;
        text.remove("//");
        ui->exactMatch->setChecked(true);
    }

    QList<PTextLabel> labels;
    if(text.isEmpty())
        return labels;

    foreach(auto label, utils::filter_transform<PTextLabel>(scene_->labels(), toText))
    {
        bool match = false;
        if(exact)
        {
            auto lines = label->info().text.split("\n");
            foreach(QString line, lines)
            {
                if(line == text)
                    match = true;
            }
        }
        else if(label->info().text.indexOf(text) >= 0)
            match = true;

        if(match)
            labels << label;
    }

    return labels;
}

void FindDialog::on_excactMatch_toggled(bool /*checked*/)
{
    ui->textEdit->setFocus();
}

bool FindDialog::eventFilter(QObject *object, QEvent *event)
{
    if(object == ui->textEdit && event->type() == QEvent::KeyPress)
    {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if(keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter)
        {
            on_findNextButton_clicked();
            return true;
        }
        else
        {
            return QDialog::eventFilter(object, event);
        }
    }
    else
    {
        return QDialog::eventFilter(object, event);
    }
}

void FindDialog::on_closeButton_clicked()
{
    close();
}
