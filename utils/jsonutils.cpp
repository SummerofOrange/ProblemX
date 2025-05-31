#include "jsonutils.h"
#include <QFile>
#include <QJsonParseError>
#include <QDebug>
#include <QDir>

QString JsonUtils::s_lastError;

QJsonObject JsonUtils::loadJsonFromFile(const QString &filePath)
{
    s_lastError.clear();
    
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        s_lastError = QString("Cannot open file: %1").arg(filePath);
        qWarning() << s_lastError;
        return QJsonObject();
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    
    if (error.error != QJsonParseError::NoError) {
        s_lastError = QString("JSON parse error in file %1: %2")
                      .arg(filePath)
                      .arg(error.errorString());
        qWarning() << s_lastError;
        return QJsonObject();
    }
    
    if (!doc.isObject()) {
        s_lastError = QString("JSON file %1 does not contain an object").arg(filePath);
        qWarning() << s_lastError;
        return QJsonObject();
    }
    
    return doc.object();
}

bool JsonUtils::saveJsonToFile(const QJsonObject &json, const QString &filePath)
{
    s_lastError.clear();
    
    // Ensure directory exists
    QDir dir = QFileInfo(filePath).absoluteDir();
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            s_lastError = QString("Cannot create directory: %1").arg(dir.absolutePath());
            qWarning() << s_lastError;
            return false;
        }
    }
    
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        s_lastError = QString("Cannot open file for writing: %1").arg(filePath);
        qWarning() << s_lastError;
        return false;
    }
    
    QJsonDocument doc(json);
    QByteArray data = doc.toJson(QJsonDocument::Indented);
    
    qint64 bytesWritten = file.write(data);
    file.close();
    
    if (bytesWritten == -1) {
        s_lastError = QString("Failed to write to file: %1").arg(filePath);
        qWarning() << s_lastError;
        return false;
    }
    
    return true;
}

QJsonObject JsonUtils::parseJsonString(const QString &jsonString)
{
    s_lastError.clear();
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(jsonString.toUtf8(), &error);
    
    if (error.error != QJsonParseError::NoError) {
        s_lastError = QString("JSON parse error: %1").arg(error.errorString());
        qWarning() << s_lastError;
        return QJsonObject();
    }
    
    if (!doc.isObject()) {
        s_lastError = "JSON string does not contain an object";
        qWarning() << s_lastError;
        return QJsonObject();
    }
    
    return doc.object();
}

QString JsonUtils::jsonToString(const QJsonObject &json, bool compact)
{
    QJsonDocument doc(json);
    QJsonDocument::JsonFormat format = compact ? QJsonDocument::Compact : QJsonDocument::Indented;
    return QString::fromUtf8(doc.toJson(format));
}

bool JsonUtils::isValidJsonFile(const QString &filePath)
{
    s_lastError.clear();
    
    QFile file(filePath);
    if (!file.exists()) {
        s_lastError = QString("File does not exist: %1").arg(filePath);
        return false;
    }
    
    if (!file.open(QIODevice::ReadOnly)) {
        s_lastError = QString("Cannot open file: %1").arg(filePath);
        return false;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonParseError error;
    QJsonDocument::fromJson(data, &error);
    
    if (error.error != QJsonParseError::NoError) {
        s_lastError = QString("JSON parse error: %1").arg(error.errorString());
        return false;
    }
    
    return true;
}

QString JsonUtils::getLastError()
{
    return s_lastError;
}