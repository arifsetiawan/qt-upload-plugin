#include "uploadplugin.h"

#include <QDebug>
#include <QFile>
#include <QUuid>
#include <QFileInfo>
#include <QTimer>
#include <QBuffer>

#include "json.h"
using QtJson::JsonObject;
using QtJson::JsonArray;

UploadPlugin::UploadPlugin(QObject * parent)
    : UploadInterface (parent)
{

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

void UploadPlugin::setDefaultParameters()
{
    m_uploadProtocol == UploadInterface::ProtocolTus;
    m_userAgent = "UploadPlugin/0.0.2";
    m_chunkSize = 50*1024;
    m_bandwidthLimit = 30*1024;
    m_queueSize = 2;
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

void UploadPlugin::uploadChunk(QNetworkReply *reply)
{
    UploadItem item = uploadHash[reply];

    if (item.chunkCounter == 0) {
        item.time.start();
        item.start = 0;
        item.end = 0;
    }

    if (item.start < item.size)
    {
        item.end = item.start + m_chunkSize;
        if (item.end > item.size)
        {
            item.end = item.size;
        }

        qint64 size = item.end - item.start;
        item.sent = item.start;

        qDebug() << item.start << item.end;

        QByteArray offset;
        offset.setNum(item.start);

        QByteArray length;
        length.setNum(item.end - item.start);

        uchar *buf = item.file->map(item.start, size);
        QBuffer * buffer = new QBuffer();
        buffer->setData(reinterpret_cast<const char*>(buf), size);
        buffer->close();

        QNetworkRequest request(item.submitUrl);
        request.setRawHeader("User-Agent", m_userAgent);
        request.setRawHeader("Connection", "keep-alive");
        request.setRawHeader("Offset", offset);
        request.setRawHeader("Content-Length", length);
        request.setRawHeader("Content-Type", "application/offset+octet-stream");

        // remove previous reply
        uploadHash.remove(reply);

        QNetworkReply * reply = manager.sendCustomRequest(request, "PATCH", buffer);
        connectSignals(reply);

        item.file->unmap(buf);
        item.start += size;

        uploadHash[reply] = item;
        urlHash[item.path] = reply;
    }
    else
    {
        qDebug() << "DONE. Me think";
        item.file->close();
        item.file->deleteLater();

        uploadHash.remove(reply);
        urlHash.remove(item.path);

        startNextUpload();
    }
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
            item.file->open(QIODevice::ReadOnly);
            item.size = item.file->size();
            qDebug() << "size" << item.size;
            item.stage = 0;
            item.final = false;
            item.sent = 0;
            item.start = 0;

            QNetworkRequest request(m_uploadUrl);
            request.setRawHeader("User-Agent", m_userAgent);
            QByteArray fileLength;
            fileLength.setNum(item.size);
            request.setRawHeader("Final-Length", fileLength);
            QByteArray contentLength;
            contentLength.setNum(0);
            request.setRawHeader("Content-Length", contentLength);
            request.setRawHeader("Content-Type", "application/octet-stream");

            QNetworkReply * reply = manager.post(request, "");
            connectSignals(reply);

            emit status(item.path, "Register", "Start registering file", m_uploadUrl.toString());

            uploadHash[reply] = item;
            urlHash[item.path] = reply;

        }
        else if (m_uploadProtocol == UploadInterface::ProtocolMultipart) {

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

            QNetworkReply * reply = manager.post(request, multiPart);
            multiPart->setParent(reply);

            item.time.start();
            uploadHash[reply] = item;
            urlHash[item.path] = reply;
        }
        else
        {
            qDebug() << m_uploadProtocol;
            qDebug() << "No one knows";
        }

        startNextUpload();
    }
}

void UploadPlugin::uploadProgress(qint64 bytesSent, qint64 bytesTotal)
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    UploadItem item = uploadHash[reply];
    qint64 actualSent = item.sent + bytesSent;
    double speed = actualSent * 1000.0 / item.time.elapsed();
    QString unit;
    if (speed < 1024) {
        unit = "bytes/sec";
    } else if (speed < 1024*1024) {
        speed /= 1024;
        unit = "kB/s";
    } else {
        speed /= 1024*1024;
        unit = "MB/s";
    }
    int percent = actualSent * 100 / item.size;

    if (item.stage > 0) {
        qDebug() << "upload" << item.submitUrl << actualSent << item.size << percent << speed << unit;
        emit progress(item.path, actualSent, item.size, percent, speed, unit);
    }
}

void UploadPlugin::uploadFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    UploadItem item = uploadHash[reply];
    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QByteArray customVerb = reply->request().attribute(QNetworkRequest::CustomVerbAttribute).toByteArray();

    //qDebug() << statusCode << customVerb;

    if(reply->error() == QNetworkReply::NoError)
    {
        if (m_uploadProtocol == UploadInterface::ProtocolTus)
        {
            if (item.stage == 0)
            {
                if (statusCode == 201)
                {
                    QByteArray location = reply->rawHeader("Location");

                    qDebug() << "Get submit url" << location;

                    item.submitUrl = QUrl(location);
                    item.stage = 1;
                    item.chunkCounter = 0;

                    uploadHash[reply] = item;
                    //qDebug() << "uploadFinished" << reply;
                    uploadChunk(reply);
                }
                else
                {
                    qDebug() << "statusCode" << statusCode;
                }
            }
            else
            {
                //qDebug() << "uploadFinished" << "chunk" << reply;

                item.chunkCounter = item.chunkCounter + 1;
                uploadHash[reply] = item;
                uploadChunk(reply);
            }
        }
        else if (m_uploadProtocol == UploadInterface::ProtocolMultipart) {

        }
        else {
            qDebug() << "No one knows";
        }
    }

    reply->deleteLater();
}

void UploadPlugin::uploadError(QNetworkReply::NetworkError)
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    UploadItem item = uploadHash[reply];
    qDebug() << "uploadError: " << item.submitUrl << reply->errorString();
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
