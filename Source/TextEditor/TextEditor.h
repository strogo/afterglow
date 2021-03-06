#pragma once
#include <QPlainTextEdit>
#include <QPoint>

class Highlighter;
class AutoCompleter;

class TextEditor : public QPlainTextEdit {
    Q_OBJECT
public:
    explicit TextEditor(QString filePath, QWidget* parent = nullptr);

    QString getFilePath() const { return filePath; }
    void setFilePath(const QString& filePath);

    void setAutoCompleter(AutoCompleter* completer);
    AutoCompleter* getAutoCompleter() const { return completer; }

    void saveFile();
    QString getModifiedName() const;

    QPoint getCursorPosition();
    void setCursorPosition(const QPoint& pos);

    void lineNumberAreaPaintEvent(QPaintEvent *event);
    int getLineNumberAreaWidth();

    QString textUnderCursor() const;
    int leftMargin() const;

    void joinLines();
    void duplicateLine();
    void cutLine();

    void increaseIndent();
    void decreaseIndent();

    void toggleSingleLineComment();
    void toggleBlockComment();

    void openAutoCompleter();
    void goToLine(int line);
    void cleanTrailingWhitespace();

signals:
    void documentModified(TextEditor* editor);
    void focusChanged(bool focus);

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void resizeEvent(QResizeEvent *event) override;
    void focusInEvent(QFocusEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void highlightCurrentLine();
    void updateLineNumberArea(const QRect &rect, int dy);

private:
    void readFile();
    void autoindent();
    void extendSelectionToBeginOfComment();

    QWidget* lineNumberArea;
    Highlighter* highlighter;
    QString filePath;
    AutoCompleter* completer;
};
