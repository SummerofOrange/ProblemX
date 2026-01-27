#ifndef TEXTNORMALIZE_H
#define TEXTNORMALIZE_H

#include <QString>
#include <QStringList>

namespace TextNormalize {

QString normalizeForSearch(const QString &text);
QStringList makeNGrams(const QString &normalizedText, int n);
QStringList makeSearchGrams(const QString &text);

}

#endif // TEXTNORMALIZE_H
