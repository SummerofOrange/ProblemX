#include "textnormalize.h"

#include <QChar>

namespace TextNormalize {

static bool isCjk(const QChar &c)
{
    const uint u = c.unicode();
    if (u >= 0x4E00 && u <= 0x9FFF) return true;
    if (u >= 0x3400 && u <= 0x4DBF) return true;
    if (u >= 0x20000 && u <= 0x2A6DF) return true;
    if (u >= 0x2A700 && u <= 0x2B73F) return true;
    if (u >= 0x2B740 && u <= 0x2B81F) return true;
    if (u >= 0x2B820 && u <= 0x2CEAF) return true;
    return false;
}

QString normalizeForSearch(const QString &text)
{
    if (text.isEmpty()) {
        return QString();
    }

    QString out;
    out.reserve(text.size());

    for (const QChar &c : text) {
        if (c.isLetterOrNumber() || isCjk(c)) {
            out.append(c.toLower());
        }
    }

    return out;
}

QStringList makeNGrams(const QString &normalizedText, int n)
{
    QStringList grams;
    if (normalizedText.isEmpty() || n <= 0) {
        return grams;
    }

    if (normalizedText.size() <= n) {
        grams.append(normalizedText);
        return grams;
    }

    grams.reserve(normalizedText.size() - n + 1);
    for (int i = 0; i + n <= normalizedText.size(); ++i) {
        grams.append(normalizedText.mid(i, n));
    }
    return grams;
}

QStringList makeSearchGrams(const QString &text)
{
    const QString normalized = normalizeForSearch(text);
    if (normalized.isEmpty()) {
        return QStringList();
    }

    const int n = (normalized.size() < 4) ? 1 : 2;
    return makeNGrams(normalized, n);
}

}

