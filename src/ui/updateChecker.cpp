#include "updateChecker.h"

#include <QApplication>
#include <QDesktopServices>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSettings>
#include <QTimer>

#include <QDebug>

void UpdateChecker::Start()
{
    auto check = new UpdateChecker(true);
    check->checkUpdate();
}

void UpdateChecker::StartIfNeeded()
{
    auto check = new UpdateChecker(false);

    QDate date = QSettings().value("checkUpdateDate").toDate();
    if(date.isNull() || date.daysTo(QDate::currentDate()) > 14)
    {
        QTimer::singleShot(0, check, SLOT(checkUpdate()));
    }
}

UpdateChecker::UpdateChecker(bool f, QObject *parent)
    : QObject(parent)
    , forced(f)
{
}

void UpdateChecker::showErrorDialog()
{
    QMessageBox msg;
    msg.setText(QString::fromUtf8("Не удалось подключиться к сайту программы"));
    msg.setWindowTitle(QApplication::instance()->applicationName());
    msg.setStandardButtons(QMessageBox::Ok);
    msg.setDefaultButton(QMessageBox::Ok);
    msg.setIcon(QMessageBox::Warning);
    msg.exec();
}

bool UpdateChecker::showNewVersionDialog(QString web)
{
    QMessageBox msg;
    QString     text =
        QString::fromUtf8("Обнаружена новая версия программы: Natrix %1, скачать?").arg(web);
    msg.setText(text);
    msg.setWindowTitle(QApplication::instance()->applicationName());
    msg.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
    msg.setDefaultButton(QMessageBox::Yes);
    msg.setIcon(QMessageBox::Information);
    if(msg.exec() == QMessageBox::Yes)
        return true;
    return false;
}

void UpdateChecker::showLastVersionDialog()
{
    QMessageBox msg;
    msg.setText(QString::fromUtf8("Вы пользуетесь последней версией программы"));
    msg.setWindowTitle(QApplication::instance()->applicationName());
    msg.setStandardButtons(QMessageBox::Ok);
    msg.setDefaultButton(QMessageBox::Ok);
    msg.setIcon(QMessageBox::Information);
    msg.exec();
}

void UpdateChecker::checkUpdate()
{
    QString url = QCoreApplication::instance()->organizationDomain();
    url         = QString("http://%1/download/version.nfo").arg(url);

    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    connect(manager, &QNetworkAccessManager::finished, [=](QNetworkReply *response) {
        response->deleteLater();
        //        int statusCode =
        //        response->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(); qDebug() <<
        //        statusCode;

        if(response->error() != QNetworkReply::NoError)
        {
            if(forced)
                showErrorDialog();

            this->deleteLater();
            return;
        }

        QVariant redirectionTarget =
            response->attribute(QNetworkRequest::RedirectionTargetAttribute);
        if(!redirectionTarget.isNull())
        {
            QUrl newUrl = QUrl(redirectionTarget.toUrl());
            manager->get(QNetworkRequest(newUrl));
            return;
        }

        QByteArray data    = response->readAll();
        QString    content = data;

        int index = content.indexOf("Natrix_Setup_");
        if(index >= 0)
        {
            content = content.mid(index, 100);
            content = content.section("_", 2, 4);
            checkUpdate(content.replace("_", "."));
        }
        else
        {
            if(forced)
                showErrorDialog();
        }

        this->deleteLater();
    });
    manager->get(QNetworkRequest(QUrl(url)));
}

void UpdateChecker::checkUpdate(QString web)
{
    QSettings().setValue("checkUpdateDate", QDate::currentDate());
    QString cur = QApplication::instance()->applicationVersion();

    auto versionOf = [](QString a) -> int {
        auto s = a.split(".");

        if(s.size() < 3)
            return -1;

        return s[0].toInt() * 100000 + s[1].toInt() * 1000 + s[2].toInt();
    };

    QSettings s;
    QString   last = s.value("checkedLastWebVersion").toString();

    if(forced || last != web)
    {
        s.setValue("checkedLastWebVersion", web);

        if(versionOf(cur) < versionOf(web))
        {
            if(showNewVersionDialog(web))
            {
                QString url = QCoreApplication::instance()->organizationDomain();
                url         = QString("http://%1/checkversion/").arg(url);
                QDesktopServices::openUrl(url);
            }
        }
        else if(forced)
        {
            showLastVersionDialog();
        }
    }
}
