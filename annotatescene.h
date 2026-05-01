#ifndef ANNOTATESCENE_H
#define ANNOTATESCENE_H

#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include "bndboxitem.h"

class AnnotateScene : public QGraphicsScene {
    Q_OBJECT
public:
    explicit AnnotateScene(QObject *parent = nullptr);
    void setImage(const QPixmap& pixmap);
    void setDrawMode(bool enable); // 切换绘制模式开关

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    QGraphicsPixmapItem* bgImageItem;
    BndBoxItem* currentDrawingBox;
    QPointF startPoint;
    bool isDrawing;
    bool drawMode;                    // 是否处于绘制模式
};

#endif // ANNOTATESCENE_H