#include "graphicsTextItem.h"

#include <QEvent>
#include <QFont>
#include <QKeyEvent>
#include <QTextCursor>
#include <QTextDocument>

#include <QDebug>

namespace geometry
{

GraphicsTextItem::GraphicsTextItem(TextInfo info)
    : QGraphicsTextItem(0)
{
    setTextInfo(info);

    // флаги
    setTextInteractionFlags(Qt::TextEditorInteraction);
    document()->setDocumentMargin(0);
}

// форсируем в том случае, если необходимо установить
// пололжение текста, поэтому при force выполняются
// вызовы, изменяющие положение текста. Сам текст не меняется
// поскольку он может быть в процессе изменения
void GraphicsTextItem::setTextInfo(TextInfo info, bool force)
{
    // содержимое
    if(info.text != info_.text || force)
    {
        // запоминаем текущее выделение
        QTextCursor c0 = textCursor();
        int         ss = c0.selectionStart();
        int         se = c0.selectionEnd();

        setPlainText(info.text);

        QTextCursor c1 = textCursor();
        if(ss != -1)
            c1.setPosition(ss);

        if(info_.alignment)
        {
            QTextBlockFormat format;
            format.setAlignment(info_.alignment == 1 ? Qt::AlignCenter : Qt::AlignRight);

            QTextCursor cursor = textCursor();
            cursor.select(QTextCursor::Document);
            cursor.mergeBlockFormat(format);
            cursor.clearSelection();
            setTextCursor(cursor);
        }

        if(ss != se)
            c1.setPosition(se, QTextCursor::KeepAnchor);
        setTextCursor(c1);
    }

    // высота текста
    if(info.height != info_.height || force)
    {
        Q_ASSERT(info.height > 0 && "font size >=0");
        QFont font("Tahoma", 12);
        font.setKerning(0);
        font.setPixelSize(info.height);
        setFont(font);
    }

    // наклон теста
    if(fabs(info.rotationAngleDegrees - info_.rotationAngleDegrees) > 1e-5 || force)
    {
        setRotation(info.rotationAngleDegrees);
    }

    // TODO нужно использовать при изменении alignment
    // adjustSize();

    if(!force)
        info_ = info;
}

void GraphicsTextItem::keyPressEvent(QKeyEvent *event)
{
    QGraphicsTextItem::keyPressEvent(event);
    checkForSpecialChars();
}

void GraphicsTextItem::keyReleaseEvent(QKeyEvent *event)
{
    QGraphicsTextItem::keyReleaseEvent(event);
    checkForSpecialChars();
}

void GraphicsTextItem::checkForSpecialChars()
{
    auto    diamChar = QChar(216);
    QString t        = toPlainText();
    bool    replace  = false;
    int     index;

    // замена @10 -> Ø10
    static QRegExp ex1{QString("(^|\\s|\\()(@)\\d+")};
    if(index = ex1.indexIn(t), index != -1)
    {
        index = t.indexOf("@", index);
        if(index != -1)
            t.replace(index, 1, diamChar);
        replace = true;
    }

    // замена "10 -> Ø10
    static QRegExp ex2{QString("(^|\\s)(\")\\d+")};
    if(index = ex2.indexIn(t), index != -1)
    {
        index = t.indexOf("\"", index);
        if(index != -1)
            t.replace(index, 1, diamChar);
        replace = true;
    }

    // замена 10@ -> 10"
    static QRegExp ex3{QString("\\d+(@)")};
    if(index = ex3.indexIn(t), index != -1)
    {
        index = t.indexOf("@", index);
        if(index != -1)
            t.replace(index, 1, "\"");
        replace = true;
    }

    if(replace)
    {
        auto tc = textCursor();
        if(tc.selectedText().isEmpty())
        {
            int p = tc.position();
            setPlainText(t);
            tc.setPosition(p);
            setTextCursor(tc);
        }
    }
}

} // namespace geometry
