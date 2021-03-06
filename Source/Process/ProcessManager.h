#pragma once
#include "Core/Singleton.h"
#include <QProcess>
#include <QTextCodec>

class ProcessManager : public QObject {
    Q_OBJECT
public:
    explicit ProcessManager(QObject* parent = nullptr);
    QProcess* getProcess();
    void stop();

protected slots:
    virtual void onReadyReadStandardOutput(const QString& data);
    virtual void onReadyReadStandardError(const QString& data);
    virtual void onFinished(int exitCode, QProcess::ExitStatus exitStatus);
    virtual void onErrorOccurred(QProcess::ProcessError error);

protected:
    QString errorToString(QProcess::ProcessError error);

private:
    QProcess* process;
    QTextCodec* outputCodec = QTextCodec::codecForLocale();
    QTextCodec::ConverterState outputCodecState;
    QTextCodec::ConverterState errorCodecState;
};
