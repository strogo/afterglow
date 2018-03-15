#include "Editor.h"
#include "LineNumberArea.h"
#include <QtGui>

Editor::Editor(QString filePath, QWidget* parent) :
        QPlainTextEdit(parent),
        filePath(filePath) {
    lineNumberArea = new LineNumberArea(this);

    connect(this, &Editor::blockCountChanged, this, &Editor::updateLineNumberAreaWidth);
    connect(this, &Editor::updateRequest, this, &Editor::updateLineNumberArea);
    connect(this, &Editor::cursorPositionChanged, this, &Editor::highlightCurrentLine);

    updateLineNumberAreaWidth(0);
    highlightCurrentLine();

    connect(this, &Editor::textChanged, [=](){
        emit documentModified(this);
    });

    readFile();
}

void Editor::saveFile() {
    QFile file(filePath);
    if (file.open(QFile::WriteOnly | QFile::Text)) {
        QTextStream out(&file);
        out << toPlainText();
        document()->setModified(false);
        documentModified(this);
    } else {
        qWarning() << "Failed to open file for writing" << filePath;
    }
}

QString Editor::getModifiedName() const {
    QFileInfo fi(filePath);
    return fi.fileName() + (document()->isModified() ? "*" : "");
}

void Editor::lineNumberAreaPaintEvent(QPaintEvent* event) {
    QPainter painter(lineNumberArea);
    painter.fillRect(event->rect(), QColor(240, 240, 240));

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = (int) blockBoundingGeometry(block).translated(contentOffset()).top();
    int bottom = top + (int) blockBoundingRect(block).height();

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            QString number = QString::number(blockNumber + 1);
            painter.setPen(QColor(170, 170, 170));
            painter.drawText(0, top, lineNumberArea->width(), fontMetrics().height(),
                             Qt::AlignRight, number);
        }

        block = block.next();
        top = bottom;
        bottom = top + (int) blockBoundingRect(block).height();
        ++blockNumber;
    }
}

int Editor::getLineNumberAreaWidth() {
    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }

    const int DEFAULT_DIGITS = 4;
    digits = qMax(DEFAULT_DIGITS, digits);

    int space = 3 + fontMetrics().width(QLatin1Char('9')) * digits;

    return space;
}

void Editor::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Tab) {
        insertPlainText(QString(4, ' '));
    } else if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        QPlainTextEdit::keyPressEvent(event);
        QTextCursor cursor = textCursor();
        int row = cursor.blockNumber();
//        int column = cursor.positionInBlock();
        if (row > 0) {
            QTextBlock block = document()->findBlockByLineNumber(row - 1);
            int count = 0;
            while (block.text().size() && block.text().at(count) == ' ') {
                count++;
            }

            if (count > 0) {
                insertPlainText(QString(count, ' '));
            }
        }
    } else {
        QPlainTextEdit::keyPressEvent(event);
    }
}

void Editor::resizeEvent(QResizeEvent* event) {
    QPlainTextEdit::resizeEvent(event);

    QRect cr = contentsRect();
    lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), getLineNumberAreaWidth(), cr.height()));
}

void Editor::updateLineNumberAreaWidth(int newBlockCount) {
    Q_UNUSED(newBlockCount)
    setViewportMargins(getLineNumberAreaWidth(), 0, 0, 0);
}

void Editor::highlightCurrentLine() {
    QList<QTextEdit::ExtraSelection> extraSelections;

    if (!isReadOnly()) {
        QTextEdit::ExtraSelection selection;

        QColor lineColor = QColor(Qt::yellow).lighter(160);

        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }

    setExtraSelections(extraSelections);
}

void Editor::updateLineNumberArea(const QRect& rect, int dy) {
    if (dy)
        lineNumberArea->scroll(0, dy);
    else
        lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());

    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth(0);
}

void Editor::readFile() {
    QFile file(filePath);
    if (file.open(QFile::ReadOnly | QFile::Text)) {
        setPlainText(file.readAll());
    } else {
        qWarning() << "Failed to open file for reading" << filePath;
    }
}
