#ifndef COMMON_H
#define COMMON_H

#include <qcoreapplication.h>
#include <QStandardPaths>
#include <QString>

namespace CommonFunctions {

inline auto configPath(const QString &suffix = QString()) -> QString
{
    return QStringLiteral("%1/%2").arg(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation), suffix);
}

inline auto applicationPath(const QString &suffix = QString()) -> QString
{
    return QStringLiteral("%1/%2").arg(QCoreApplication::applicationDirPath(), suffix);
}

inline auto handleTilde(const QString &path) -> QString
{
    if (path.startsWith(QStringLiteral("~/")))
    {
        return QStringLiteral("%1/%2").arg(QStandardPaths::writableLocation(QStandardPaths::HomeLocation), path.sliced(2));
    }

    return path;
}

}

#endif
