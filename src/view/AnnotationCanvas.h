#ifndef ANNOTATIONCANVAS_H
#define ANNOTATIONCANVAS_H

#include "../model/DataTypes.h"

#include <QPixmap>
#include <QWidget>

class AnnotationCanvas : public QWidget
{
    Q_OBJECT

public:
    explicit AnnotationCanvas(QWidget *parent = nullptr);

    bool loadImage(const QString &imagePath);
    QSize imageSize() const;

    void setBoxes(const QVector<AnnotationBox> &boxes);
    QVector<AnnotationBox> boxes() const;
    int selectedIndex() const;

    void setCurrentClass(int classId, const QString &className);
    bool updateSelectedClass(int classId, const QString &className);
    bool updateBox(int index, const AnnotationBox &box);

public slots:
    void zoomIn();
    void zoomOut();
    void fitToWindow();
    void rotateLeft();
    void rotateRight();
    void deleteSelected();
    void undo();
    void redo();
    void selectBox(int index);

signals:
    void boxesChanged(const QVector<AnnotationBox> &boxes);
    void selectionChanged(int index);
    void logMessage(const QString &message);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    QRectF imagePaintRect() const;
    QRectF normalizedToWidget(const QRectF &rect) const;
    QPointF widgetToNormalized(const QPoint &point) const;
    QRectF widgetDragToNormalizedRect(const QPoint &a, const QPoint &b) const;
    int hitTest(const QPoint &point) const;
    int handleHitTest(const QPoint &point, int *boxIndex) const;
    void selectIndex(int index);
    void pushHistory();
    void commitDragHistoryIfNeeded();
    void setBoxesFromHistory(const QVector<AnnotationBox> &boxes);
    QRectF clampRect(const QRectF &rect) const;
    QRectF transformRectToView(const QRectF &rect) const;
    QPointF transformPointFromView(const QPointF &point) const;

private:
    enum InteractionMode {
        NoInteraction,
        Drawing,
        MovingBox,
        ResizingBox,
        Panning
    };

    enum ResizeHandle {
        NoHandle,
        TopLeftHandle,
        TopRightHandle,
        BottomLeftHandle,
        BottomRightHandle
    };

    QPixmap m_pixmap;
    QString m_imagePath;
    QVector<AnnotationBox> m_boxes;
    int m_selectedIndex = -1;
    int m_currentClassId = 0;
    QString m_currentClassName = "Car";
    double m_zoom = 1.0;
    int m_rotation = 0;
    QPointF m_panOffset;
    bool m_drawing = false;
    InteractionMode m_interaction = NoInteraction;
    ResizeHandle m_resizeHandle = NoHandle;
    QPoint m_dragStart;
    QPoint m_dragCurrent;
    QPointF m_dragStartNormalized;
    QPointF m_panStart;
    QRectF m_editOriginalRect;
    QVector<AnnotationBox> m_dragOriginalBoxes;
    QVector<QVector<AnnotationBox>> m_undoStack;
    QVector<QVector<AnnotationBox>> m_redoStack;
};

#endif // ANNOTATIONCANVAS_H
