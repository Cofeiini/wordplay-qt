#ifndef COMMON_H
#define COMMON_H

#include <QApplication>
#include <QCoreApplication>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
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

namespace ConfigFunctions {

[[nodiscard]] inline auto readConfig() -> QJsonObject
{
    QFile file(CommonFunctions::configPath(QStringLiteral("config.json")));
    if (!file.open(QFile::Text | QFile::ReadOnly))
    {
        return {};
    }

    return QJsonDocument::fromJson(file.readAll()).object();
}

[[nodiscard]] inline auto writeConfig(const QJsonObject &json) -> QString
{
    QFile file(CommonFunctions::configPath(QStringLiteral("config.json")));
    if (!file.open(QFile::Text | QFile::WriteOnly | QFile::Truncate))
    {
        return file.errorString();
    }

    const qint64 written = file.write(QJsonDocument(json).toJson(QJsonDocument::Indented));
    if (written == -1)
    {
        return QApplication::translate("Config", "Error while writing the config");
    }

    if (written == 0)
    {
        return QApplication::translate("Config", "Wrote nothing to the config");
    }

    return {};
}

}

#endif
