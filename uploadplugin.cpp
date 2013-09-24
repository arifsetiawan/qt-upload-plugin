#include "uploadplugin.h"

#include <QDebug>
#include <QFile>
#include <QUuid>
#include <QFileInfo>
#include <QTimer>

#include "json.h"
using QtJson::JsonObject;
using QtJson::JsonArray;

UploadPlugin::UploadPlugin(QObject * parent)
    : UploadInterface (parent)
{
    m_uploadProtocol == UploadInterface::ProtocolTus;
}

Q_EXPORT_PLUGIN2(UploadPlugin, UploadPlugin)

UploadPlugin::~UploadPlugin()
{

}

QString UploadPlugin::name(void) const
{
    return "UploadPlugin";
}

QString UploadPlugin::version() const
{
    return "1.0";
}

void UploadPlugin::append(const QString &file)
{
    QFileInfo fi(file);

    qDebug() << "upload append" << file;

    UploadItem item;
    item.key = fi.fileName();
    item.path = file;

    if (uploadQueue.isEmpty())
        QTimer::singleShot(0, this, SLOT(startNextUpload()));

    uploadQueue.enqueue(item);
}

void UploadPlugin::append(const QStringList &fileList)
{
    foreach (QString file, fileList){
        append(file);
    }

    if (uploadQueue.isEmpty())
        QTimer::singleShot(0, this, SIGNAL(finishedAll()));
}

void UploadPlugin::pause(const QString &url)
{
    stopUpload(url, true);
}

void UploadPlugin::pause(const QStringList &urlList)
{
    foreach (QString url, urlList){
        pause(url);
    }
}

void UploadPlugin::resume(const QString &url)
{
    append(url);
}

void UploadPlugin::resume(const QStringList & urlList)
{
    foreach (QString url, urlList){
        resume(url);
    }
}

void UploadPlugin::stop(const QString &url)
{
    stopUpload(url, false);
}

void UploadPlugin::stop(const QStringList &urlList)
{
    foreach (QString url, urlList){
        stop(url);
    }
}

void UploadPlugin::stopUpload(const QString &url, bool pause)
{

}

void UploadPlugin::connectSignals(QNetworkReply *reply)
{
    connect(reply, SIGNAL(uploadProgress(qint64,qint64)),
            this, SLOT(uploadProgress(qint64,qint64)));
    connect(reply, SIGNAL(finished()),
            this, SLOT(uploadFinished()));
    connect(reply, SIGNAL(error(QNetworkReply::NetworkError)),
            this, SLOT(uploadError(QNetworkReply::NetworkError)));
    connect(reply, SIGNAL(sslErrors(QList<QSslError>)),
            this, SLOT(uploadSslErrors(QList<QSslError>)));
}

void UploadPlugin::startNextUpload()
{
    if (uploadQueue.isEmpty()) {
        emit finishedAll();
        return;
    }

    if (uploadHash.size() < m_queueSize)
    {
        UploadItem item = uploadQueue.dequeue();

        if (m_uploadProtocol == UploadInterface::ProtocolTus) {

            item.file = new QFile(item.path);
            item.stage = 0;

            QNetworkRequest request(m_uploadUrl);
            request.setRawHeader("User-Agent", m_userAgent);
            QByteArray fileLength;
            fileLength.setNum(item.file->size());
            request.setRawHeader("Final-Length", fileLength);
            QByteArray contentLength;
            contentLength.setNum(0);
            request.setRawHeader("Content-Length", 0);
            request.setRawHeader("Content-Type", "application/octet-stream");

            QNetworkReply * reply = manager.post(request, "");
            connectSignals(reply);

            emit status(item.path, "Upload", "Start uploading file", item.postUrl);

            item.time.start();
            uploadHash[reply] = item;
            urlHash[item.path] = reply;

        }
        else if (m_uploadProtocol == UploadInterface::ProtocolTus) {

            QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

            item.file = new QFile(item.path);
            item.file->open(QIODevice::ReadOnly);
            item.stage = 0;

            QHttpPart imagePart;
            imagePart.setHeader(QNetworkRequest::ContentDispositionHeader,
                                QVariant("form-data; name=\"document\"; filename=\""+ item.key +"\""));
            imagePart.setBodyDevice(item.file);
            item.file->setParent(multiPart);
            multiPart->append(imagePart);

            QNetworkRequest request(m_uploadUrl);
            request.setRawHeader("User-Agent", m_userAgent);

            QNetworkReply * reply = networkAccessManager->post(request, multiPart);
            multiPart->setParent(reply);
        }
        else {
            qDebug() << "No one knows";
        }

        startNextUpload();
    }
}

void UploadPlugin::uploadProgress(qint64 bytesReceived, qint64 bytesTotal)
{

}

void UploadPlugin::uploadReadyRead()
{

}

void UploadPlugin::uploadFinished()
{

}

void UploadPlugin::uploadError(QNetworkReply::NetworkError)
{

}

void UploadPlugin::uploadSslErrors(QList<QSslError>)
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    reply->ignoreSslErrors();
}

void UploadPlugin::setBandwidthLimit(int size)
{
}

void UploadPlugin::addSocket(QIODevice *socket)
{
}

void UploadPlugin::removeSocket(QIODevice *socket)
{
}

void UploadPlugin::transfer()
{
}

void UploadPlugin::scheduleTransfer()
{
}

QString UploadPlugin::getStatus() const
{
    return "";
}
