#pragma once
#include <QPlainTextEdit>

class Editor : public QPlainTextEdit {
    Q_OBJECT
public:
    explicit Editor(QString filePath, QWidget* parent = nullptr);
    QString getFilePath() const { return filePath; }
    void saveFile();
    QString getModifiedName() const;

signals:
    void documentModified(Editor* editor);

protected:
    void keyPressEvent(QKeyEvent* event) override;

private:
    void readFile();

    QString filePath;
};
