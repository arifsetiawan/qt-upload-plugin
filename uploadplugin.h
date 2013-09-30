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

struct UploadItem {
    QString key;
    QString path;
    QString submitUrl;
    QFile *file;
    QTime time;
    int stage;
    int chunkCounter;
    bool final;
    qint64 size;
    qint64 start;
    qint64 end;
    qint64 sent;
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
    void setDefaultParameters();

    QString getStatus() const;

    void append(const QString &file);
    void append(const QStringList & fileList);

    void pause(const QString &url);
    void pause(const QStringList & urlList);

    void resume(const QString &url);
    void resume(const QStringList & urlList);

    void stop(const QString &url);
    void stop(const QStringList & urlList);

    void setBandwidthLimit(int size);

private slots:
    void startNextUpload();
    void uploadProgress(qint64 bytesSent, qint64 bytesTotal);
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

    void uploadChunk(QNetworkReply *reply);

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
