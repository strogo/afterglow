#include "ProjectTreeView.h"
#include "NewName.h"
#include "Rename.h"
#include <QtWidgets>

ProjectTreeView::ProjectTreeView(QWidget* parent) : QTreeView(parent) {
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QTreeView::customContextMenuRequested, this, &ProjectTreeView::onCustomContextMenu);
    connect(this, &QTreeView::doubleClicked, this, &ProjectTreeView::onDoubleClicked);

    fsModel = new QFileSystemModel(this);

    contextMenu = new QMenu(this);

    QMenu* newMenu = new QMenu(tr("New"), this);

    QAction* newRustFileAction = newMenu->addAction(tr("Rust File..."));
    connect(newRustFileAction, &QAction::triggered, this, &ProjectTreeView::onNewRustFile);

    QAction* newFileAction = newMenu->addAction(tr("File..."));
    connect(newFileAction, &QAction::triggered, this, &ProjectTreeView::onNewFile);

    QAction* newDirectoryAction = newMenu->addAction(tr("Directory..."));
    connect(newDirectoryAction, &QAction::triggered, this, &ProjectTreeView::onNewDirectory);

    contextMenu->addMenu(newMenu);

    QAction* openAction = contextMenu->addAction(tr("Open"));
    connect(openAction, &QAction::triggered, [=]() {
        QModelIndex index = selectedIndexes().first();
        if (fsModel->isDir(index)) {
            setExpanded(index, true);
        } else {
            openActivated(fsModel->filePath(index));
        }
    });

    QAction* removeAction = contextMenu->addAction(tr("Remove..."));
    connect(removeAction, &QAction::triggered, this, &ProjectTreeView::onFileRemove);

    QAction* renameAction = contextMenu->addAction(tr("Rename..."));
    connect(renameAction, &QAction::triggered, this, &ProjectTreeView::onFileRename);

    setFrameShape(QFrame::NoFrame);
    setHeaderHidden(true);
}

ProjectTreeView::~ProjectTreeView() {
}

void ProjectTreeView::setRootPath(const QString& path) {
    if (path.isNull()) {
        setModel(nullptr);
    } else {
        setModel(fsModel);
        setRootIndex(fsModel->setRootPath(path));

        // Hide columns exclude first
        for (int i = 1; i < fsModel->columnCount(); ++i) {
            hideColumn(i);
        }
    }
}

void ProjectTreeView::onCustomContextMenu(const QPoint& point) {
    QModelIndex index = indexAt(point);
    if (index.isValid()) {
        contextMenu->exec(mapToGlobal(point));
    }
}

void ProjectTreeView::onDoubleClicked(const QModelIndex& index) {
    QFileInfo fi = qobject_cast<QFileSystemModel*>(model())->fileInfo(index);
    if (!fi.isDir()) {
        emit openActivated(fi.absoluteFilePath());
    }
}

void ProjectTreeView::onNewRustFile() {
    NewName newName(tr("New Rust File"), this);
    newName.exec();
    QString name = newName.getName();
    if (!name.isEmpty()) {
        QFileInfo fi(name);
        QString newFilePath = getCurrentDirectory() + "/" + fi.baseName() + ".rs";
        newFileActivated(newFilePath);
    }
}

void ProjectTreeView::onNewFile() {
    NewName newName(tr("New File"), this);
    newName.exec();
    QString name = newName.getName();
    if (!name.isEmpty()) {
        newFileActivated(getCurrentDirectory() + "/" + name);
    }
}

void ProjectTreeView::onNewDirectory() {
    NewName newName(tr("New Directory"), this);
    newName.exec();
    QString name = newName.getName();
    if (!name.isEmpty()) {
        QModelIndex selectedIndex = selectedIndexes().first();
        QModelIndex index = fsModel->mkdir(fsModel->isDir(selectedIndex) ? selectedIndex : fsModel->parent(selectedIndex), name);
        setCurrentIndex(index);
    }
}

void ProjectTreeView::onFileRemove() {
    QModelIndex index = selectedIndexes().first();
    bool isDir = fsModel->isDir(index);
    QString text = QString("Remove %1 \"%2\"?")
            .arg(isDir ? tr("directory") : tr("file"))
            .arg(fsModel->fileName(index));
    int result = QMessageBox::question(this, tr("Remove"), text);
    if (result == QMessageBox::Yes) {
        if (isDir) {
            fsModel->rmdir(index);
        }
        emit removeActivated(fsModel->filePath(index));
    }
}

void ProjectTreeView::onFileRename() {
    QModelIndex index = selectedIndexes().first();
    QString oldPath = fsModel->filePath(index);
    Rename rename(oldPath, this);
    rename.exec();
    QString name = rename.getName();
    if (!name.isEmpty()) {
        QString newPath = getCurrentDirectory() + "/" + name;
        if (QFile::rename(oldPath, newPath)) {
            QModelIndex modelIndex = fsModel->index(newPath);
            setCurrentIndex(modelIndex);
            renameActivated(oldPath, newPath);
        } else {
            qWarning() << QString("Failed to rename %1 to %2").arg(oldPath).arg(newPath);
        }
    }
}

QString ProjectTreeView::getCurrentDirectory() const {
    QModelIndex index = selectedIndexes().first();
    return fsModel->isDir(index) ? fsModel->filePath(index)
                                 : fsModel->fileInfo(index).absolutePath();
}
