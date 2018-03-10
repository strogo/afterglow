#include "Global.h"
#include "Constants.h"
#include <QtCore>

namespace Global {

namespace {
    QString portableSettingsPath;
}

void init() {
    portableSettingsPath = QCoreApplication::applicationDirPath() + "/" + APP_SETTINGS_NAME;
}

QString getPortableSettingsPath() {
    return portableSettingsPath;
}

} // Global
