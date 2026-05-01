#ifndef BNDBOXITEM_H
#define BNDBOXITEM_H

#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QPen>
#include <QInputDialog>
#include <QGraphicsSceneMouseEvent>

class BndBoxItem : public QGraphicsRectItem {
public:
    QString classId = "0";
    QGraphicsTextItem* textItem;

    BndBoxItem(const QRectF& rect, QGraphicsItem* parent = nullptr)
        : QGraphicsRectItem(rect, parent) {
        setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable);

        // 【修改】线条变细(线宽设为1)，去掉 setBrush 填充
        setPen(QPen(Qt::red, 1));

        textItem = new QGraphicsTextItem(this);
        updateTextLabel();
        updateTextPos();
    }

    void setClassId(const QString& id) {
        classId = id;
        updateTextLabel();
    }

    void updateTextLabel() {
        QString html = QString("<div style='background-color: #E53935; color: white; padding: 2px 4px; font-weight: bold; font-family: Arial; font-size: 11px;'>%1</div>").arg(classId);
        textItem->setHtml(html);
    }

    void updateTextPos() {
        textItem->setPos(rect().topLeft() - QPointF(0, 22));
    }

    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override {
        if (change == ItemSelectedChange) {
            if (value.toBool()) {
                // 选中时线条稍微加粗为2，变成绿色，依然没有填充
                setPen(QPen(Qt::green, 2));
                textItem->setHtml(QString("<div style='background-color: #43A047; color: white; padding: 2px 4px; font-weight: bold; font-family: Arial; font-size: 11px;'>%1</div>").arg(classId));
            } else {
                // 取消选中恢复为红线，线宽为1
                setPen(QPen(Qt::red, 1));
                updateTextLabel();
            }
        }
        return QGraphicsRectItem::itemChange(change, value);
    }

protected:
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override {
        bool ok;
        QString text = QInputDialog::getText(nullptr, "修改标注", "请输入新的类别 ID:", QLineEdit::Normal, classId, &ok);
        if (ok && !text.isEmpty()) setClassId(text);
        QGraphicsRectItem::mouseDoubleClickEvent(event);
    }
};

#endif // BNDBOXITEM_H