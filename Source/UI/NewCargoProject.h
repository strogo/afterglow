#pragma once
#include <QDialog>

namespace Ui {
    class NewCargoProject;
}

class CargoProcessor;

class NewCargoProject : public QDialog {
    Q_OBJECT

public:
    explicit NewCargoProject(CargoProcessor* cargoProcessor, QWidget* parent = 0);
    ~NewCargoProject();

private slots:
    void on_pushButtonDirectory_clicked();
    void on_buttonBox_accepted();
    void adjustAcceptedButton();

private:

    enum class Template {
        Binary,
        Library
    };

    Ui::NewCargoProject* ui;
    CargoProcessor* cargoProcessor;
};
