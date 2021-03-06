#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "Core/Global.h"
#include "Core/Constants.h"
#include "Core/Settings.h"
#include "NewProject.h"
#include "GoToLine.h"
#include "Options.h"
#include "Process/CargoManager.h"
#include "ProjectTree.h"
#include "ProjectProperties.h"
#include "TextEditor/TextEditor.h"
#include "TextEditor/AutoCompleter.h"
#include "TextEditor/SyntaxHighlightManager.h"
#include "NewName.h"
#ifdef Q_OS_WIN
    #include <windows.h>
#endif
#include <QtWidgets>

MainWindow::MainWindow() :
        ui(new Ui::MainWindow) {
    new SyntaxHighlightManager(this);

    ui->setupUi(this);

    projectProperties = new ProjectProperties;

    cargoManager = new CargoManager(projectProperties, this);
    connect(cargoManager, &CargoManager::projectCreated, this, &MainWindow::onProjectCreated);
    connect(cargoManager, &CargoManager::consoleMessage, this, &MainWindow::onCargoMessage);

    projectTree = new ProjectTree;
    connect(projectTree, &ProjectTree::openActivated, this, &MainWindow::addSourceTab);
    connect(projectTree, &ProjectTree::newFileActivated, this, &MainWindow::onFileCreated);
    connect(projectTree, &ProjectTree::removeActivated, this, &MainWindow::onFileRemoved);
    connect(projectTree, &ProjectTree::renameActivated, this, &MainWindow::onFileRenamed);

    ui->tabWidgetSide->addTab(projectTree, tr("Project"));
    ui->tabWidgetSide->addTab(projectProperties, tr("Properties"));

    int id = QFontDatabase::addApplicationFont(":/Resources/Font/FontAwesome/Font-Awesome-5-Free-Solid-900.otf");
    if (id < 0) {
        qWarning() << "Failed to load FontAwesome!";
    }

    QFont font;
    font.setFamily(QFontDatabase::applicationFontFamilies(id).at(0));
    font.setPixelSize(16);

    ui->toolButtonCargoBuild->setFont(font);
    ui->toolButtonCargoBuild->setText(Constants::ICON_COG);

    ui->toolButtonCargoRun->setFont(font);
    ui->toolButtonCargoRun->setText(Constants::ICON_PLAY);

    ui->toolButtonCargoStop->setFont(font);
    ui->toolButtonCargoStop->setText(Constants::ICON_STOP);

    ui->toolButtonCargoClear->setFont(font);
    ui->toolButtonCargoClear->setText(Constants::ICON_TRASH_ALT);

    completer = new AutoCompleter(this);
    completer->setModelSorting(QCompleter::CaseInsensitivelySortedModel);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setWrapAround(false);

    loadSettings();
    updateMenuState();
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent* event) {
    saveSettings();
    closeProject();
    event->accept();
}

void MainWindow::on_actionNewProject_triggered() {
    NewProject newCargoProject(this);
    newCargoProject.exec();

    if (!newCargoProject.getProjectPath().isEmpty()) {
        cargoManager->createProject(newCargoProject.getProjectTemplate(), newCargoProject.getProjectPath());
    }
}

void MainWindow::on_actionNewRustFile_triggered() {
    projectTree->onNewRustFile();
}

void MainWindow::on_actionCloseProject_triggered() {
    closeProject();
}

void MainWindow::on_actionSaveAs_triggered() {
    QString filePath = QFileDialog::getSaveFileName(this, tr("Save File"), editor->getFilePath(), "Rust (*.rs);;All Files(*.*)");
    if (!filePath.isEmpty()) {
        bool result = QFile::copy(editor->getFilePath(), filePath);
        if (result) {
            addSourceTab(filePath);
        } else {
            QMessageBox::critical(this, tr("Duplicate"), tr("Error creating copy of file"));
        }
    }
}

void MainWindow::on_actionSaveAll_triggered() {
    for (int i = 0; i < ui->tabWidgetSource->count(); i++) {
        TextEditor* editor = static_cast<TextEditor*>(ui->tabWidgetSource->widget(i));
        editor->saveFile();
    }
}

void MainWindow::on_actionClearMenuRecentFiles_triggered() {
    for (int i = ui->menuRecentFiles->actions().size() - Constants::SEPARATOR_AND_MENU_CLEAR_COUNT - 1; i >= 0; i--) {
        ui->menuRecentFiles->removeAction(ui->menuRecentFiles->actions().at(i));
    }

    updateMenuState();
}

void MainWindow::on_actionClearMenuRecentProjects_triggered() {
    for (int i = ui->menuRecentProjects->actions().size() - Constants::SEPARATOR_AND_MENU_CLEAR_COUNT - 1; i >= 0; i--) {
        ui->menuRecentProjects->removeAction(ui->menuRecentProjects->actions().at(i));
    }

    updateMenuState();
}

void MainWindow::on_actionClose_triggered() {
    on_tabWidgetSource_tabCloseRequested(ui->tabWidgetSource->currentIndex());
}

void MainWindow::on_actionCloseAll_triggered() {
    while (ui->tabWidgetSource->count()) {
        on_tabWidgetSource_tabCloseRequested(0);
    }
}

void MainWindow::on_actionCloseOther_triggered() {
    int i = 0;
    while (ui->tabWidgetSource->count() > 1) {
        if (i != ui->tabWidgetSource->currentIndex()) {
            on_tabWidgetSource_tabCloseRequested(i);
        } else {
            i++;
        }
    }
}

void MainWindow::on_actionNewFile_triggered() {
    projectTree->onNewFile();
}

void MainWindow::on_actionNewDirectory_triggered() {
    projectTree->onNewDirectory();
}

void MainWindow::on_actionOpen_triggered() {
    QString dirPath = projectPath.isEmpty() ? Global::getWorkspacePath() : projectPath;
    QString filePath = QFileDialog::getOpenFileName(this, tr("Open File or Project"), dirPath, "All Files(*.*)");
    if (filePath.isEmpty()) return;

    QFileInfo fi(filePath);
    if (fi.fileName() == "Cargo.toml") {
        QFileInfo fi(filePath);
        openProject(fi.absolutePath());
    } else {
        addSourceTab(filePath);
    }
}

void MainWindow::on_actionSave_triggered() {
    editor->saveFile();
}

void MainWindow::on_actionUndo_triggered() {
    editor->undo();
}

void MainWindow::on_actionRedo_triggered() {
    editor->redo();
}

void MainWindow::on_actionCut_triggered() {
    editor->cut();
}

void MainWindow::on_actionCopy_triggered() {
    editor->copy();
}

void MainWindow::on_actionPaste_triggered() {
    editor->paste();
}

void MainWindow::on_actionSelectAll_triggered() {
    editor->selectAll();
}

void MainWindow::on_actionToggleSingleLineComment_triggered() {
    editor->toggleSingleLineComment();
}

void MainWindow::on_actionToggleBlockComment_triggered() {
    editor->toggleBlockComment();
}

void MainWindow::on_actionAutoCompleter_triggered() {
    editor->openAutoCompleter();
}

void MainWindow::on_actionJoinLines_triggered() {
    editor->joinLines();
}

void MainWindow::on_actionDuplicateLine_triggered() {
    editor->duplicateLine();
}

void MainWindow::on_actionCutLine_triggered() {
    editor->cutLine();
}

void MainWindow::on_actionIncreaseIndent_triggered() {
    editor->increaseIndent();
}

void MainWindow::on_actionDecreaseIndent_triggered() {
    editor->decreaseIndent();
}

void MainWindow::on_actionGoToLine_triggered() {
    GoToLine goToLine(this);
    goToLine.exec();

    if (goToLine.isValid()) {
        editor->goToLine(goToLine.getLine());
    }
}

void MainWindow::on_actionCleanTrailingWhitespace_triggered() {
    editor->cleanTrailingWhitespace();
}

void MainWindow::on_actionBuild_triggered() {
    on_actionSaveAll_triggered();
    cargoManager->build();
}

void MainWindow::on_actionRun_triggered() {
    on_actionSaveAll_triggered();
    cargoManager->run();
}

void MainWindow::on_actionStop_triggered() {
    cargoManager->stop();
}

void MainWindow::on_actionClean_triggered() {
    cargoManager->clean();
}

void MainWindow::on_actionOptions_triggered() {
    Options options(this);
    options.exec();
}

void MainWindow::on_actionAbout_triggered() {
    QMessageBox::about(this, tr("About %1").arg(Constants::APP_NAME),
        tr("<h3>%1 %2 %3</h3>\
           Based on Qt %4<br> \
           Build on %5<br><br> \
           <a href=%6>%6</a><br><br>%7")
            .arg(Constants::APP_NAME)
            .arg(Constants::APP_VERSION).arg(Constants::APP_STATUS)
            .arg(QT_VERSION_STR)
            .arg(__DATE__)
            .arg(Constants::APP_URL).arg(Constants::APP_COPYRIGHT));
}

void MainWindow::on_tabWidgetSource_tabCloseRequested(int index) {
    QWidget* widget = ui->tabWidgetSource->widget(index);
    ui->tabWidgetSource->removeTab(index);
    delete widget;
}

void MainWindow::on_tabWidgetSource_currentChanged(int index) {
    if (index >= 0) {
        editor = static_cast<TextEditor*>(ui->tabWidgetSource->widget(index));
        editor->setAutoCompleter(completer);
        editor->setFocus();
        QString filePath = editor->getFilePath();
        projectTree->selectFile(filePath);
        changeWindowTitle(filePath);
    } else {
        editor = nullptr;
        projectTree->setCurrentIndex(QModelIndex());
        changeWindowTitle();
    }

    updateMenuState();
}

void MainWindow::on_toolButtonCargoBuild_clicked() {
    cargoManager->build();
}

void MainWindow::on_toolButtonCargoRun_clicked() {
    cargoManager->run();
}

void MainWindow::on_toolButtonCargoClear_clicked() {
    ui->plainTextEditCargo->clear();
}

void MainWindow::on_toolButtonCargoStop_clicked() {
    cargoManager->stop();
}

void MainWindow::onProjectCreated(const QString& path) {
    openProject(path, true);
}

void MainWindow::onCargoMessage(const QString& message, bool html, bool start) {
    int index = static_cast<int>(OutputPane::Cargo);
    ui->tabWidgetOutput->setCurrentIndex(index);

    if (start) {
        ui->plainTextEditCargo->clear();
    }

    if (html) {
        ui->plainTextEditCargo->appendHtml(message);
    } else {
        ui->plainTextEditCargo->insertPlainText(message);
    }

    ui->plainTextEditCargo->verticalScrollBar()->setValue(ui->plainTextEditCargo->verticalScrollBar()->maximum());
}

void MainWindow::onFileCreated(const QString& filePath) {
    addNewFile(filePath);
}

void MainWindow::onFileRemoved(const QString& filePath) {
    QVector<int> indices;
    for (int i = 0; i < ui->tabWidgetSource->count(); i++) {
        TextEditor* editor = static_cast<TextEditor*>(ui->tabWidgetSource->widget(i));
        if (editor->getFilePath().contains(filePath)) {
            indices.append(i);
        }
    }

    // Remove tabs in reverse order
    for (int i = indices.count() - 1; i >= 0; i--) {
        on_tabWidgetSource_tabCloseRequested(indices.at(i));
    }
}

void MainWindow::onFileRenamed(const QString& oldPath, const QString& newPath) {
    for (int i = 0; i < ui->tabWidgetSource->count(); i++) {
        TextEditor* editor = static_cast<TextEditor*>(ui->tabWidgetSource->widget(i));
        if (editor->getFilePath().contains(oldPath)) {
            QFileInfo fi(newPath);
            if (fi.isDir()) {
                editor->setFilePath(editor->getFilePath().replace(oldPath, newPath));
            } else {
                editor->setFilePath(newPath);
                onDocumentModified(editor);
            }
        }
    }
}

int MainWindow::addSourceTab(const QString& filePath) {
    int tabIndex = findSource(filePath);
    if (tabIndex != -1) {
        ui->tabWidgetSource->setCurrentIndex(tabIndex);
        return tabIndex;
    } else {
        QFileInfo fi(filePath);
        TextEditor* editor = new TextEditor(filePath);
        connect(editor, &TextEditor::documentModified, this, &MainWindow::onDocumentModified);
        int index = ui->tabWidgetSource->addTab(editor, fi.fileName());
        ui->tabWidgetSource->setTabToolTip(index, filePath);
        ui->tabWidgetSource->setCurrentIndex(index);

        addRecentFile(filePath);

        return index;
    }
}

void MainWindow::addNewFile(const QString& filePath) {
    if (!filePath.isEmpty()) {
        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly)) {
            qWarning() << "Failed to open session file for writing" << filePath;
            return;
        }
        file.write("");
        addSourceTab(filePath);
    }
}

void MainWindow::onDocumentModified(TextEditor* editor) {
    ui->tabWidgetSource->setTabText(ui->tabWidgetSource->indexOf(editor), editor->getModifiedName());
}

void MainWindow::addRecentFile(const QString& filePath) {
    addRecentFileOrProject(ui->menuRecentFiles, filePath, [=] {
        addSourceTab(filePath);
    });
}

void MainWindow::addRecentProject(const QString& projectPath) {
    if (!QFileInfo::exists(projectPath + "/Cargo.toml")) return;

    addRecentFileOrProject(ui->menuRecentProjects, projectPath, [=] {
        openProject(projectPath);
    });
}

void MainWindow::addRecentFileOrProject(QMenu* menu, const QString& filePath, const std::function<void()>& callback) {
    for (QAction* action : menu->actions()) {
        if (action->text() == filePath) {
            menu->removeAction(action);
        }
    }

    QAction* fileAction = new QAction(filePath);
    connect(fileAction, &QAction::triggered, callback);
    menu->insertAction(menu->actions().first(), fileAction);

    if (menu->actions().size() > Constants::MAX_RECENT_FILES + Constants::SEPARATOR_AND_MENU_CLEAR_COUNT) {
        menu->removeAction(menu->actions().at(menu->actions().size() - Constants::SEPARATOR_AND_MENU_CLEAR_COUNT - 1));
    }

    updateMenuState();
}

void MainWindow::saveProjectProperties() {
    if (projectPath.isEmpty()) return;

    QString path = projectPath + "/" + Constants::PROJECT_DATA_DIRECTORY + "/" + Constants::PROJECT_PROPERTIES_FILE;
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Failed to open project properties file for writing" << path;
        return;
    }

    QJsonObject obj;
    obj["target"] = static_cast<int>(projectProperties->getBuildTarget());
    obj["arguments"] = projectProperties->getArguments();

    QJsonDocument doc(obj);
    file.write(doc.toJson());
}

void MainWindow::loadProjectProperties() {
    if (projectPath.isEmpty()) return;

    QString path = projectPath + "/" + Constants::PROJECT_DATA_DIRECTORY + "/" + Constants::PROJECT_PROPERTIES_FILE;
    if (!QFileInfo::exists(path)) {
        return;
    }

    projectProperties->setProject(projectPath);

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open project properties file for reading" << path;
        return;
    }

    QJsonDocument doc(QJsonDocument::fromJson(file.readAll()));
    QJsonObject obj = doc.object();

    CargoManager::BuildTarget target = static_cast<CargoManager::BuildTarget>(obj["target"].toInt());
    projectProperties->setBuildTarget(target);
    projectProperties->setArguments(obj["arguments"].toString());
}

void MainWindow::loadSettings() {
    // Window geometry
    int width = Settings::getValue("window.geometry.width").toInt();
    int height = Settings::getValue("window.geometry.height").toInt();

    QJsonValue x = Settings::getValue("window.geometry.x");
    QJsonValue y = Settings::getValue("window.geometry.y");

    if (x.isNull() || y.isNull()) {
        // Center window in screen.
        resize(width, height);
        setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, size(), qApp->desktop()->availableGeometry()));
    } else {
        setGeometry(x.toInt(), y.toInt(), width, height);
    }

    // Window state
    Qt::WindowStates state = static_cast<Qt::WindowStates>(Settings::getValue("window.state").toInt());

    if (state == Qt::WindowMaximized || state == Qt::WindowFullScreen) {
        setWindowState(state);
    }

    // Splitter sizes
    QList<int> sizes;
    QJsonArray sizesArray = Settings::getValue("gui.splitters.main").toArray();
    for (const auto& size : sizesArray) {
        sizes.append(size.toInt());
    }
    ui->splitterMain->setSizes(sizes);

    sizesArray = Settings::getValue("gui.splitters.side").toArray();
    sizes.clear();

    for (const auto& size : sizesArray) {
        sizes.append(size.toInt());
    }
    ui->splitterSide->setSizes(sizes);

    // Sidebar
    ui->actionShowSidebar->setChecked(Settings::getValue("gui.sidebar.visible").toBool());
    ui->tabWidgetSide->setCurrentIndex(Settings::getValue("gui.sidebar.tab").toInt());

    // Output pane
    ui->actionShowOutput->setChecked(Settings::getValue("gui.output.visible").toBool());
    ui->tabWidgetOutput->setCurrentIndex(Settings::getValue("gui.output.tab").toInt());

    // Recent projects
    QJsonArray recentProjects = Settings::getValue("gui.recent.projects").toArray();
    for (int i = recentProjects.size() - 1; i >= 0; --i) {
        QString projectPath = recentProjects.at(i).toString();
        addRecentProject(projectPath);
    }

    // Recent files
    QJsonArray recentFiles = Settings::getValue("gui.recent.files").toArray();
    for (int i = recentFiles.size() - 1; i >= 0; --i) {
        QString filePath = recentFiles.at(i).toString();
        addRecentFile(filePath);
    }

    // Last project
    QString lastProject = Settings::getValue("gui.session.project").toString();
    if (QFileInfo::exists(lastProject + "/Cargo.toml")) {
        openProject(lastProject);
    }
}

void MainWindow::saveSettings() {
    // Window geometry
    if (windowState() == Qt::WindowNoState) {
        Settings::setValue("window.geometry.x", geometry().x());
        Settings::setValue("window.geometry.y", geometry().y());
        Settings::setValue("window.geometry.width", geometry().width());
        Settings::setValue("window.geometry.height", geometry().height());
    }

    // Window state
    Settings::setValue("window.state", static_cast<int>(windowState()));

    // Splitter sizes
    if (ui->actionShowSidebar->isChecked()) {
        QJsonArray sizesArray;
        for (int size : ui->splitterMain->sizes()) {
            sizesArray.append(QJsonValue(size));
        }
        Settings::setValue("gui.splitters.main", sizesArray);
    }

    if (ui->actionShowOutput->isChecked()) {
        QJsonArray sizesArray;
        for (int size : ui->splitterSide->sizes()) {
            sizesArray.append(QJsonValue(size));
        }
        Settings::setValue("gui.splitters.side", sizesArray);
    }

    // Sidebar
    Settings::setValue("gui.sidebar.visible", ui->actionShowSidebar->isChecked());
    Settings::setValue("gui.sidebar.tab", ui->tabWidgetSide->currentIndex());

    // Output pane
    Settings::setValue("gui.output.visible", ui->actionShowOutput->isChecked());
    Settings::setValue("gui.output.tab", ui->tabWidgetOutput->currentIndex());

    // Recent projects
    QJsonArray recentProjects;
    for (int i = 0; i < ui->menuRecentProjects->actions().size() - Constants::SEPARATOR_AND_MENU_CLEAR_COUNT; ++i) {
        recentProjects.append(ui->menuRecentProjects->actions().at(i)->text());
    }
    Settings::setValue("gui.recent.projects", recentProjects);

    // Recent files
    QJsonArray recentFiles;
    for (int i = 0; i < ui->menuRecentFiles->actions().size() - Constants::SEPARATOR_AND_MENU_CLEAR_COUNT; ++i) {
        recentFiles.append(ui->menuRecentFiles->actions().at(i)->text());
    }
    Settings::setValue("gui.recent.files", recentFiles);

    // Last project
    Settings::setValue("gui.session.project", projectPath);

    Settings::flush();
}

void MainWindow::saveSession() {
    if (!Settings::getValue("gui.session.restore").toBool() || projectPath.isEmpty()) {
        return;
    }

    QDir dir(projectPath);
    bool result = dir.mkdir(Constants::PROJECT_DATA_DIRECTORY);
#ifdef Q_OS_WIN
    // Set hidden attribute on created directory (need only for Windows).
    if (result) {
        QString directory = projectPath + "/" + Constants::PROJECT_DATA_DIRECTORY;
        wchar_t* charText = new wchar_t[directory.length() + 1];
        directory.toWCharArray(charText);
        charText[directory.length()] = 0; // append null terminator
        int attr = GetFileAttributes(charText);
        if ((attr & FILE_ATTRIBUTE_HIDDEN) == 0) {
            SetFileAttributes(charText, attr | FILE_ATTRIBUTE_HIDDEN);
         }
    }
#else
    Q_UNUSED(result)
#endif

    QString path = projectPath + "/" + Constants::PROJECT_DATA_DIRECTORY + "/" + Constants::PROJECT_SESSION_FILE;
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Failed to open session file for writing" << path;
        return;
    }

    QJsonArray openFiles;
    for (int i = 0; i < ui->tabWidgetSource->count(); i++) {
        TextEditor* editor = static_cast<TextEditor*>(ui->tabWidgetSource->widget(i));

        QJsonObject obj;
        obj["path"] = editor->getFilePath();

        QJsonArray cursorPosArray;
        QPoint pos = editor->getCursorPosition();
        cursorPosArray.append(pos.x());
        cursorPosArray.append(pos.y());
        obj["cursor"] = cursorPosArray;

        openFiles.append(obj);
    }

    QJsonObject obj;
    obj["openFiles"] = openFiles;
    obj["selectedTab"] = ui->tabWidgetSource->currentIndex();

    QJsonDocument doc(obj);
    file.write(doc.toJson());
}

void MainWindow::loadSession() {
    if (!Settings::getValue("gui.session.restore").toBool() || projectPath.isEmpty()) {
        return;
    }

    QString path = projectPath + "/" + Constants::PROJECT_DATA_DIRECTORY + "/" + Constants::PROJECT_SESSION_FILE;
    if (!QFileInfo::exists(path)) {
        return;
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open session file for reading" << path;
        return;
    }

    QJsonDocument doc(QJsonDocument::fromJson(file.readAll()));
    QJsonArray array = doc.object()["openFiles"].toArray();
    int selectedTab = doc.object()["selectedTab"].toInt();
    QString selectedFilePath = array.at(selectedTab).toObject()["path"].toString();

    for (int i = 0; i < array.count(); i++) {
        QJsonObject obj = array.at(i).toObject();
        QString filePath = obj["path"].toString();
        if (QFileInfo::exists(filePath)) {
            int index = addSourceTab(filePath);
            QJsonArray cursorPosArray = obj["cursor"].toArray();
            TextEditor* editor = static_cast<TextEditor*>(ui->tabWidgetSource->widget(index));
            editor->setCursorPosition(QPoint(cursorPosArray.at(0).toInt(), cursorPosArray.at(1).toInt()));
        }
    }

    int index = findSource(selectedFilePath);
    ui->tabWidgetSource->setCurrentIndex(index);
}

void MainWindow::openProject(const QString& path, bool isNew) {
    closeProject();

    projectPath = path;
    projectTree->setRootPath(path);
    cargoManager->setProjectPath(path);

    if (isNew) {
        QString filePath = projectPath + "/src/main.rs";
        if (QFileInfo::exists(filePath)) {
            addSourceTab(filePath);
        } else {
            filePath = projectPath + "/src/lib.rs";
            if (QFileInfo::exists(filePath)) {
                addSourceTab(filePath);
            }
        }
        projectProperties->setProject(projectPath);
    } else {
        loadProjectProperties();
        loadSession();
    }

    if (!ui->tabWidgetSource->count()) {
        changeWindowTitle();
    }

    updateMenuState();
    addRecentProject(path);
}

void MainWindow::closeProject() {
    if (projectPath.isNull()) return;

    saveSession();
    saveProjectProperties();

    on_actionCloseAll_triggered();

    projectProperties->reset();
    projectTree->setRootPath(QString());
    projectPath = QString();
    changeWindowTitle();
    updateMenuState();
    ui->plainTextEditCargo->clear();
}

void MainWindow::changeWindowTitle(const QString& filePath) {
    QString title = QApplication::applicationName();

    if (!projectPath.isEmpty()) {
        QFileInfo fi(projectPath);
        title = fi.baseName() + " - " + title;
    }

    if (!filePath.isEmpty()) {
        QFileInfo fi(filePath);
        title = fi.fileName() + " - " + title;
    }

    setWindowTitle(title);
}

int MainWindow::findSource(const QString& filePath) {
    for (int i = 0; i < ui->tabWidgetSource->count(); i++) {
        TextEditor* editor = static_cast<TextEditor*>(ui->tabWidgetSource->widget(i));
        if (editor->getFilePath() == filePath) {
            return i;
        }
    }

    return -1;
}

void MainWindow::updateMenuState() {
    ui->menuEdit->menuAction()->setVisible(!projectPath.isNull());
    ui->menuCargo->menuAction()->setVisible(!projectPath.isNull());

    int index = ui->tabWidgetSource->currentIndex();

    ui->actionSave->setEnabled(index >= 0);
    ui->actionSaveAs->setEnabled(index >= 0);
    ui->actionSaveAll->setEnabled(index >= 0);

    ui->actionClose->setEnabled(index >= 0);
    ui->actionCloseOther->setEnabled(index >= 0);
    ui->actionCloseAll->setEnabled(index >= 0);

    ui->menuRecentProjects->menuAction()->setEnabled(ui->menuRecentProjects->actions().size() > Constants::SEPARATOR_AND_MENU_CLEAR_COUNT);
    ui->menuRecentFiles->menuAction()->setEnabled(ui->menuRecentFiles->actions().size() > Constants::SEPARATOR_AND_MENU_CLEAR_COUNT);

    ui->menuEdit->menuAction()->setEnabled(index >= 0);
}
