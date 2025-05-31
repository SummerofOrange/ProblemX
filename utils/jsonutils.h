#ifndef JSONUTILS_H
#define JSONUTILS_H

#include <QString>
#include <QJsonObject>
#include <QJsonDocument>

class JsonUtils
{
public:
    // Load JSON from file
    static QJsonObject loadJsonFromFile(const QString &filePath);
    
    // Save JSON to file
    static bool saveJsonToFile(const QJsonObject &json, const QString &filePath);
    
    // Parse JSON string
    static QJsonObject parseJsonString(const QString &jsonString);
    
    // Convert JSON to formatted string
    static QString jsonToString(const QJsonObject &json, bool compact = false);
    
    // Validate JSON file
    static bool isValidJsonFile(const QString &filePath);
    
    // Get JSON parse error message
    static QString getLastError();
    
private:
    static QString s_lastError;
};

#endif // JSONUTILS_H