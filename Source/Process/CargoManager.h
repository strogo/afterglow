#pragma once
#include "ProcessManager.h"
#include <QTime>

class ProjectProperties;

class CargoManager : public ProcessManager {
    Q_OBJECT
public:
    enum class ProjectTemplate {
        Binary,
        Library
    };

    enum class BuildTarget {
        Debug,
        Release
    };

    explicit CargoManager(ProjectProperties* projectProperties, QObject* parent = nullptr);
    ~CargoManager();

    void createProject(ProjectTemplate projectTemplate, const QString& path);
    void build();
    void run();
    void clean();

    void setProjectPath(const QString& path);

signals:
    void consoleMessage(const QString& message, bool html = false, bool start = false);
    void projectCreated(const QString& path);

private slots:
    void onReadyReadStandardOutput(const QString& data) override;
    void onReadyReadStandardError(const QString& data) override;
    void onFinished(int exitCode, QProcess::ExitStatus exitStatus) override;

private:
    void prepareAndStart(const QStringList& arguments);

    enum class CommandStatus {
        None,
        New,
        Build,
        Run
    };

    void coloredOutputMessage(const QString& message, bool start = false);

    ProjectProperties* projectProperties;
    CommandStatus commandStatus = CommandStatus::None;
    QString projectPath;
};
