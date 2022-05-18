#ifndef ICONFIGURABLEPLUGIN_H
#define ICONFIGURABLEPLUGIN_H

#include <QObject>
#include <QSettings>
#include <extensionsystem/iplugin.h>
#include <coreplugin/uavconfiginfo.h>

#include <coreplugin/core_global.h>

namespace Core {
class CORE_EXPORT IConfigurablePlugin : public ExtensionSystem::IPlugin {
    Q_OBJECT
public:
    virtual ~IConfigurablePlugin() {}
    virtual void readConfig(QSettings &settings, UAVConfigInfo *configInfo) = 0;
    virtual void saveConfig(QSettings &settings, UAVConfigInfo *configInfo) const = 0;
};
} // namespace Core

#endif // ICONFIGURABLEPLUGIN_H
