#ifndef APPCONFIG_H
#define APPCONFIG_H

#include <QString>

class AppConfig
{
public:
    static QString projectRoot();
    static QString defaultDatasetPath();
    static QString databasePath();
    static QString scriptsDir();
    static QString modelsDir();
    static QString runsDir();
    static QString pythonProgram();
};

#endif // APPCONFIG_H
