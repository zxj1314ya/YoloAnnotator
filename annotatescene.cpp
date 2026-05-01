#include "annotatescene.h"
#include <QKeyEvent>
#include <QInputDialog>

AnnotateScene::AnnotateScene(QObject *parent) : QGraphicsScene(parent), bgImageItem(nullptr), currentDrawingBox(nullptr), isDrawing(false), drawMode(false) {}

void AnnotateScene::setImage(const QPixmap& pixmap) {
    this->clear();
    bgImageItem = this->addPixmap(pixmap);
    bgImageItem->setZValue(-1);
    this->setSceneRect(pixmap.rect());
}

void AnnotateScene::setDrawMode(bool enable) {
    drawMode = enable;
}

void AnnotateScene::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    // 只有在左键点击，且处于“绘制模式”时，才画框
    if (event->button() == Qt::LeftButton && drawMode && bgImageItem) {
        QGraphicsItem* item = itemAt(event->scenePos(), QTransform());
        // 如果点到了已有的框，优先执行选中和拖拽
        if (item && item != bgImageItem && item->type() == BndBoxItem::Type) {
            QGraphicsScene::mousePressEvent(event);
            return;
        }
        isDrawing = true;
        startPoint = event->scenePos();
        currentDrawingBox = new BndBoxItem(QRectF(startPoint, QSizeF(0, 0)));
        this->addItem(currentDrawingBox);
    } else {
        QGraphicsScene::mousePressEvent(event); // 非绘制模式，交给父类处理（平移/选中）
    }
}

void AnnotateScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
    if (isDrawing && currentDrawingBox) {
        QRectF rect = QRectF(startPoint, event->scenePos()).normalized();
        rect = rect.intersected(bgImageItem->boundingRect());
        currentDrawingBox->setRect(rect);
        currentDrawingBox->updateTextPos();
    } else {
        QGraphicsScene::mouseMoveEvent(event);
        // 动态更新所有选中框的标签位置（拖拽框的时候标签跟着动）
        for(QGraphicsItem* item : selectedItems()) {
            BndBoxItem* box = dynamic_cast<BndBoxItem*>(item);
            if(box) box->updateTextPos();
        }
    }
}

void AnnotateScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    if (isDrawing && event->button() == Qt::LeftButton) {
        isDrawing = false;
        if (currentDrawingBox && currentDrawingBox->rect().width() > 5 && currentDrawingBox->rect().height() > 5) {
            bool ok;
            QString text = QInputDialog::getText(nullptr, "新建标注", "输入类别 ID:", QLineEdit::Normal, "0", &ok);
            if (ok && !text.isEmpty()) {
                currentDrawingBox->setClassId(text);
            } else {
                removeItem(currentDrawingBox);
                delete currentDrawingBox;
            }
        } else if (currentDrawingBox) {
            removeItem(currentDrawingBox);
            delete currentDrawingBox;
        }
        currentDrawingBox = nullptr;
    } else {
        QGraphicsScene::mouseReleaseEvent(event);
    }
}

void AnnotateScene::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
        const QList<QGraphicsItem*> selected = selectedItems();
        for (QGraphicsItem* item : selected) {
            removeItem(item);
            delete item;
        }
    }
    QGraphicsScene::keyPressEvent(event);
}