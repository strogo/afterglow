#pragma once
#include <QDialog>

namespace Ui {
    class GoToLine;
}

class GoToLine : public QDialog {
    Q_OBJECT

public:
    explicit GoToLine(QWidget* parent = 0);
    ~GoToLine();
    int getLine() const { return line; }

private slots:
    void on_lineEdit_textChanged(const QString& text);

private:
    Ui::GoToLine* ui;
    int line = -1;
};
