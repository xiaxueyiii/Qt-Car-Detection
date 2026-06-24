#include "AppConfig.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QProcessEnvironment>

namespace {
QString normalize(const QString &path)
{
    return QDir::fromNativeSeparators(QDir(path).absolutePath());
}

bool looksLikeProjectRoot(const QString &path)
{
    QDir dir(path);
    return dir.exists("Vehicles-dataset") && (dir.exists("scripts") || dir.exists("CarDet.pro"));
}
}

QString AppConfig::projectRoot()
{
    const QString envRoot = QProcessEnvironment::systemEnvironment().value("CARDET_ROOT").trimmed();
    if (!envRoot.isEmpty() && QFileInfo::exists(envRoot)) {
        return normalize(envRoot);
    }

    const QString courseRoot = "E:/Desktop/FriD/suanfa/CarDet";
    if (looksLikeProjectRoot(courseRoot)) {
        return normalize(courseRoot);
    }

    QDir dir(QCoreApplication::applicationDirPath());
    for (int i = 0; i < 8; ++i) {
        if (looksLikeProjectRoot(dir.absolutePath())) {
            return normalize(dir.absolutePath());
        }
        if (!dir.cdUp()) {
            break;
        }
    }

    return normalize(QDir::currentPath());
}

QString AppConfig::defaultDatasetPath()
{
    return projectRoot() + "/Vehicles-dataset";
}

QString AppConfig::databasePath()
{
    return projectRoot() + "/database/cardet.db";
}

QString AppConfig::scriptsDir()
{
    return projectRoot() + "/scripts";
}

QString AppConfig::modelsDir()
{
    return projectRoot() + "/models";
}

QString AppConfig::runsDir()
{
    return projectRoot() + "/runs";
}

QString AppConfig::pythonProgram()
{
    const QString envPython = QProcessEnvironment::systemEnvironment().value("CARDET_PYTHON").trimmed();
    if (!envPython.isEmpty()) {
        return envPython;
    }

    const QString venvPython = projectRoot() + "/.venv/Scripts/python.exe";
    if (QFileInfo::exists(venvPython)) {
        return venvPython;
    }

    return QStringLiteral("python");
}
