#ifndef DOWNLOADPLUGIN_H
#define DOWNLOADPLUGIN_H

#include "uploader.h"
#include <QObject>
#include <QtPlugin>
#include <QNetworkAccessManager>
#include <QQueue>
#include <QUrl>
#include <QTime>
#include <QSet>
#include <QString>
#include <QFile>
#include <QHash>
#include <QStringList>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSslError>
#include <QHttpMultiPart>

struct UploadItem{
    QString key;
    QString path;
    QString submitUrl;
    QFile *file;
    QTime time;
    int stage;
};

class UploadPlugin : public UploadInterface
{
    Q_OBJECT
    Q_INTERFACES(UploadInterface)

public:
    UploadPlugin(QObject * parent = 0);
    ~UploadPlugin();

    QString name() const;
    QString version() const;

    void append(const QString &file);
    void append(const QStringList & fileList);

    void pause(const QString &url);
    void pause(const QStringList & urlList);

    void resume(const QString &url);
    void resume(const QStringList & urlList);

    void stop(const QString &url);
    void stop(const QStringList & urlList);

    void setBandwidthLimit(int size);

    QString getStatus() const;

private slots:
    void startNextUpload();
    void uploadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void uploadFinished();
    void uploadError(QNetworkReply::NetworkError);
    void uploadSslErrors(QList<QSslError>);

private:
    void addSocket(QIODevice *socket);
    void removeSocket(QIODevice *socket);
    void transfer();
    void scheduleTransfer();

    void stopUpload(const QString &url, bool pause);
    void connectSignals(QNetworkReply *reply);

private:
    QNetworkAccessManager manager;
    QQueue<UploadItem> uploadQueue;
    QHash<QNetworkReply*, UploadItem> uploadHash;
    QHash<QString, QNetworkReply*> urlHash;
    QList<UploadItem> completedList;

    QSet<QIODevice*> replies;
    QTime stopWatch;
    bool transferScheduled;
};

#endif // DOWNLOADPLUGIN_H
