#include "AnnotationCanvas.h"

#include <QCursor>
#include <QMouseEvent>
#include <QPainter>
#include <QTransform>
#include <QWheelEvent>

namespace {
constexpr double kMinBoxSize = 0.005;
constexpr double kHandleSize = 9.0;

bool sameBox(const AnnotationBox &a, const AnnotationBox &b)
{
    return a.classId == b.classId
        && a.className == b.className
        && a.normalizedRect == b.normalizedRect
        && qFuzzyCompare(a.confidence + 1.0, b.confidence + 1.0);
}

bool sameBoxes(const QVector<AnnotationBox> &a, const QVector<AnnotationBox> &b)
{
    if (a.size() != b.size()) {
        return false;
    }
    for (int i = 0; i < a.size(); ++i) {
        if (!sameBox(a.at(i), b.at(i))) {
            return false;
        }
    }
    return true;
}
}

AnnotationCanvas::AnnotationCanvas(QWidget *parent)
    : QWidget(parent)
{
    setMinimumSize(720, 520);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
}

bool AnnotationCanvas::loadImage(const QString &imagePath)
{
    QPixmap pixmap(imagePath);
    if (pixmap.isNull()) {
        m_pixmap = QPixmap();
        m_imagePath.clear();
        m_boxes.clear();
        m_undoStack.clear();
        m_redoStack.clear();
        update();
        emit logMessage("图片加载失败：" + imagePath);
        return false;
    }

    m_pixmap = pixmap;
    m_imagePath = imagePath;
    m_selectedIndex = -1;
    m_undoStack.clear();
    m_redoStack.clear();
    fitToWindow();
    emit logMessage("当前图片：" + imagePath);
    return true;
}

QSize AnnotationCanvas::imageSize() const
{
    return m_pixmap.size();
}

void AnnotationCanvas::setBoxes(const QVector<AnnotationBox> &boxes)
{
    m_boxes = boxes;
    if (m_selectedIndex >= m_boxes.size()) {
        m_selectedIndex = -1;
    }
    m_undoStack.clear();
    m_redoStack.clear();
    update();
    emit boxesChanged(m_boxes);
}

QVector<AnnotationBox> AnnotationCanvas::boxes() const
{
    return m_boxes;
}

int AnnotationCanvas::selectedIndex() const
{
    return m_selectedIndex;
}

void AnnotationCanvas::setCurrentClass(int classId, const QString &className)
{
    m_currentClassId = qMax(0, classId);
    m_currentClassName = className.isEmpty() ? QString("class_%1").arg(classId) : className;
}

bool AnnotationCanvas::updateSelectedClass(int classId, const QString &className)
{
    if (m_selectedIndex < 0 || m_selectedIndex >= m_boxes.size()) {
        emit logMessage("请先选中一个标注框。");
        return false;
    }

    pushHistory();
    m_boxes[m_selectedIndex].classId = qMax(0, classId);
    m_boxes[m_selectedIndex].className = className.isEmpty() ? QString("class_%1").arg(classId) : className;
    update();
    emit boxesChanged(m_boxes);
    return true;
}

bool AnnotationCanvas::updateBox(int index, const AnnotationBox &box)
{
    if (index < 0 || index >= m_boxes.size()) {
        return false;
    }

    AnnotationBox normalizedBox = box;
    normalizedBox.normalizedRect = clampRect(box.normalizedRect);
    if (normalizedBox.normalizedRect.width() < kMinBoxSize || normalizedBox.normalizedRect.height() < kMinBoxSize) {
        emit logMessage("标注框太小，已忽略本次编辑。");
        return false;
    }

    pushHistory();
    m_boxes[index] = normalizedBox;
    selectIndex(index);
    update();
    emit boxesChanged(m_boxes);
    return true;
}

void AnnotationCanvas::zoomIn()
{
    m_zoom = qMin(8.0, m_zoom * 1.2);
    update();
}

void AnnotationCanvas::zoomOut()
{
    m_zoom = qMax(0.12, m_zoom / 1.2);
    update();
}

void AnnotationCanvas::fitToWindow()
{
    m_zoom = 1.0;
    m_panOffset = QPointF();
    update();
}

void AnnotationCanvas::rotateLeft()
{
    m_rotation = (m_rotation + 270) % 360;
    update();
}

void AnnotationCanvas::rotateRight()
{
    m_rotation = (m_rotation + 90) % 360;
    update();
}

void AnnotationCanvas::deleteSelected()
{
    if (m_selectedIndex < 0 || m_selectedIndex >= m_boxes.size()) {
        return;
    }
    pushHistory();
    m_boxes.removeAt(m_selectedIndex);
    selectIndex(-1);
    update();
    emit boxesChanged(m_boxes);
}

void AnnotationCanvas::undo()
{
    if (m_undoStack.isEmpty()) {
        emit logMessage("没有可撤销的标注操作。");
        return;
    }
    m_redoStack.append(m_boxes);
    setBoxesFromHistory(m_undoStack.takeLast());
}

void AnnotationCanvas::redo()
{
    if (m_redoStack.isEmpty()) {
        emit logMessage("没有可重做的标注操作。");
        return;
    }
    m_undoStack.append(m_boxes);
    setBoxesFromHistory(m_redoStack.takeLast());
}

void AnnotationCanvas::selectBox(int index)
{
    if (index < -1 || index >= m_boxes.size()) {
        return;
    }
    selectIndex(index);
    update();
}

void AnnotationCanvas::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.fillRect(rect(), QColor("#242832"));

    if (m_pixmap.isNull()) {
        painter.setPen(QColor("#d7dce5"));
        painter.drawText(rect(), Qt::AlignCenter, "选择图片文件夹后开始标注");
        return;
    }

    const QRectF target = imagePaintRect();
    const QPixmap displayPixmap = m_rotation == 0
                                      ? m_pixmap
                                      : m_pixmap.transformed(QTransform().rotate(m_rotation), Qt::SmoothTransformation);
    painter.drawPixmap(target, displayPixmap, QRectF(QPointF(0, 0), displayPixmap.size()));

    painter.setRenderHint(QPainter::Antialiasing, true);
    for (int i = 0; i < m_boxes.size(); ++i) {
        const AnnotationBox &box = m_boxes.at(i);
        const QRectF boxRect = normalizedToWidget(box.normalizedRect);
        const bool selected = (i == m_selectedIndex);
        painter.setPen(QPen(selected ? QColor("#00d2ff") : QColor("#ffcc33"), selected ? 3 : 2));
        painter.drawRect(boxRect);

        if (selected) {
            painter.setBrush(QColor("#ffffff"));
            painter.setPen(QPen(QColor("#1f2937"), 1));
            const QList<QPointF> handles = {
                boxRect.topLeft(), boxRect.topRight(), boxRect.bottomLeft(), boxRect.bottomRight()
            };
            for (const QPointF &handle : handles) {
                painter.drawRect(QRectF(handle - QPointF(kHandleSize / 2.0, kHandleSize / 2.0),
                                        QSizeF(kHandleSize, kHandleSize)));
            }
            painter.setBrush(Qt::NoBrush);
        }

        const QString label = QString("%1 %2").arg(box.classId).arg(box.className);
        const QRect textRect = painter.fontMetrics().boundingRect(label).adjusted(-6, -4, 6, 4);
        QRectF bg(boxRect.topLeft() + QPointF(0, -qMax(22, textRect.height())), QSizeF(qMax(70, textRect.width()), 22));
        if (bg.top() < target.top()) {
            bg.moveTop(boxRect.top());
        }
        painter.fillRect(bg, selected ? QColor("#007c91") : QColor("#111820"));
        painter.setPen(Qt::white);
        painter.drawText(bg.adjusted(5, 0, -4, 0), Qt::AlignVCenter | Qt::AlignLeft, label);
    }

    if (m_interaction == Drawing) {
        const QRectF drawRect = normalizedToWidget(widgetDragToNormalizedRect(m_dragStart, m_dragCurrent));
        painter.setPen(QPen(QColor("#42ff9d"), 2, Qt::DashLine));
        painter.drawRect(drawRect);
    }
}

void AnnotationCanvas::mousePressEvent(QMouseEvent *event)
{
    if (m_pixmap.isNull()) {
        QWidget::mousePressEvent(event);
        return;
    }

    if (event->button() == Qt::MiddleButton || event->button() == Qt::RightButton) {
        m_interaction = Panning;
        m_dragStart = event->pos();
        m_panStart = m_panOffset;
        setCursor(Qt::ClosedHandCursor);
        return;
    }

    if (event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }

    int handleBox = -1;
    const int handle = handleHitTest(event->pos(), &handleBox);
    if (handle != NoHandle && handleBox >= 0) {
        selectIndex(handleBox);
        m_interaction = ResizingBox;
        m_resizeHandle = static_cast<ResizeHandle>(handle);
        m_dragStart = event->pos();
        m_dragStartNormalized = widgetToNormalized(event->pos());
        m_editOriginalRect = m_boxes.at(handleBox).normalizedRect;
        m_dragOriginalBoxes = m_boxes;
        return;
    }

    const int hit = hitTest(event->pos());
    if (hit >= 0) {
        selectIndex(hit);
        m_interaction = MovingBox;
        m_dragStart = event->pos();
        m_dragStartNormalized = widgetToNormalized(event->pos());
        m_editOriginalRect = m_boxes.at(hit).normalizedRect;
        m_dragOriginalBoxes = m_boxes;
        setCursor(Qt::SizeAllCursor);
        update();
        return;
    }

    if (!imagePaintRect().contains(event->pos())) {
        selectIndex(-1);
        update();
        return;
    }

    m_interaction = Drawing;
    m_drawing = true;
    m_dragStart = event->pos();
    m_dragCurrent = event->pos();
    selectIndex(-1);
}

void AnnotationCanvas::mouseMoveEvent(QMouseEvent *event)
{
    if (m_interaction == Drawing) {
        m_dragCurrent = event->pos();
        update();
        return;
    }

    if (m_interaction == Panning) {
        m_panOffset = m_panStart + (event->pos() - m_dragStart);
        update();
        return;
    }

    if (m_interaction == MovingBox && m_selectedIndex >= 0 && m_selectedIndex < m_boxes.size()) {
        const QPointF current = widgetToNormalized(event->pos());
        const QPointF delta = current - m_dragStartNormalized;
        QRectF moved = m_editOriginalRect.translated(delta);
        if (moved.left() < 0.0) {
            moved.moveLeft(0.0);
        }
        if (moved.top() < 0.0) {
            moved.moveTop(0.0);
        }
        if (moved.right() > 1.0) {
            moved.moveRight(1.0);
        }
        if (moved.bottom() > 1.0) {
            moved.moveBottom(1.0);
        }
        m_boxes[m_selectedIndex].normalizedRect = clampRect(moved);
        update();
        return;
    }

    if (m_interaction == ResizingBox && m_selectedIndex >= 0 && m_selectedIndex < m_boxes.size()) {
        const QPointF current = widgetToNormalized(event->pos());
        QRectF resized = m_editOriginalRect;
        if (m_resizeHandle == TopLeftHandle || m_resizeHandle == BottomLeftHandle) {
            resized.setLeft(current.x());
        }
        if (m_resizeHandle == TopRightHandle || m_resizeHandle == BottomRightHandle) {
            resized.setRight(current.x());
        }
        if (m_resizeHandle == TopLeftHandle || m_resizeHandle == TopRightHandle) {
            resized.setTop(current.y());
        }
        if (m_resizeHandle == BottomLeftHandle || m_resizeHandle == BottomRightHandle) {
            resized.setBottom(current.y());
        }
        m_boxes[m_selectedIndex].normalizedRect = clampRect(resized.normalized());
        update();
        return;
    }

    int handleBox = -1;
    const int handle = handleHitTest(event->pos(), &handleBox);
    if (handle == TopLeftHandle || handle == BottomRightHandle) {
        setCursor(Qt::SizeFDiagCursor);
    } else if (handle == TopRightHandle || handle == BottomLeftHandle) {
        setCursor(Qt::SizeBDiagCursor);
    } else if (hitTest(event->pos()) >= 0) {
        setCursor(Qt::SizeAllCursor);
    } else {
        unsetCursor();
    }
}

void AnnotationCanvas::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_interaction == Drawing && event->button() == Qt::LeftButton) {
        m_interaction = NoInteraction;
        m_drawing = false;
        QRectF rect = widgetDragToNormalizedRect(m_dragStart, event->pos());
        if (rect.width() >= kMinBoxSize && rect.height() >= kMinBoxSize) {
            pushHistory();
            AnnotationBox box;
            box.classId = m_currentClassId;
            box.className = m_currentClassName;
            box.normalizedRect = rect;
            m_boxes << box;
            selectIndex(m_boxes.size() - 1);
            emit boxesChanged(m_boxes);
        }
        update();
        return;
    }

    if ((m_interaction == MovingBox || m_interaction == ResizingBox) && event->button() == Qt::LeftButton) {
        commitDragHistoryIfNeeded();
        m_interaction = NoInteraction;
        m_resizeHandle = NoHandle;
        unsetCursor();
        update();
        return;
    }

    if (m_interaction == Panning && (event->button() == Qt::MiddleButton || event->button() == Qt::RightButton)) {
        m_interaction = NoInteraction;
        unsetCursor();
        return;
    }

    QWidget::mouseReleaseEvent(event);
}

void AnnotationCanvas::wheelEvent(QWheelEvent *event)
{
    if (event->modifiers() & Qt::ControlModifier) {
        if (event->angleDelta().y() > 0) {
            zoomIn();
        } else {
            zoomOut();
        }
        event->accept();
        return;
    }
    QWidget::wheelEvent(event);
}

QRectF AnnotationCanvas::imagePaintRect() const
{
    if (m_pixmap.isNull()) {
        return QRectF();
    }

    const QSizeF canvasSize = size();
    QSizeF imageSize = m_pixmap.size();
    if (m_rotation == 90 || m_rotation == 270) {
        imageSize = QSizeF(imageSize.height(), imageSize.width());
    }

    const double baseScale = qMin(canvasSize.width() / imageSize.width(), canvasSize.height() / imageSize.height());
    const QSizeF scaled = imageSize * baseScale * m_zoom;
    return QRectF((canvasSize.width() - scaled.width()) / 2.0 + m_panOffset.x(),
                  (canvasSize.height() - scaled.height()) / 2.0 + m_panOffset.y(),
                  scaled.width(),
                  scaled.height());
}

QRectF AnnotationCanvas::normalizedToWidget(const QRectF &rect) const
{
    const QRectF imageRect = imagePaintRect();
    const QRectF viewRect = transformRectToView(rect);
    return QRectF(imageRect.left() + viewRect.left() * imageRect.width(),
                  imageRect.top() + viewRect.top() * imageRect.height(),
                  viewRect.width() * imageRect.width(),
                  viewRect.height() * imageRect.height());
}

QPointF AnnotationCanvas::widgetToNormalized(const QPoint &point) const
{
    const QRectF imageRect = imagePaintRect();
    if (imageRect.isEmpty()) {
        return QPointF();
    }
    const double viewX = qBound(0.0, (point.x() - imageRect.left()) / imageRect.width(), 1.0);
    const double viewY = qBound(0.0, (point.y() - imageRect.top()) / imageRect.height(), 1.0);
    return transformPointFromView(QPointF(viewX, viewY));
}

QRectF AnnotationCanvas::widgetDragToNormalizedRect(const QPoint &a, const QPoint &b) const
{
    const QPointF first = widgetToNormalized(a);
    const QPointF second = widgetToNormalized(b);
    return clampRect(QRectF(first, second).normalized());
}

int AnnotationCanvas::hitTest(const QPoint &point) const
{
    for (int i = m_boxes.size() - 1; i >= 0; --i) {
        QRectF rect = normalizedToWidget(m_boxes.at(i).normalizedRect).adjusted(-4, -4, 4, 4);
        if (rect.contains(point)) {
            return i;
        }
    }
    return -1;
}

int AnnotationCanvas::handleHitTest(const QPoint &point, int *boxIndex) const
{
    if (boxIndex) {
        *boxIndex = -1;
    }
    for (int i = m_boxes.size() - 1; i >= 0; --i) {
        const QRectF rect = normalizedToWidget(m_boxes.at(i).normalizedRect);
        const QList<QPair<ResizeHandle, QPointF>> handles = {
            {TopLeftHandle, rect.topLeft()},
            {TopRightHandle, rect.topRight()},
            {BottomLeftHandle, rect.bottomLeft()},
            {BottomRightHandle, rect.bottomRight()}
        };
        for (const auto &entry : handles) {
            const QRectF handleRect(entry.second - QPointF(kHandleSize, kHandleSize),
                                    QSizeF(kHandleSize * 2.0, kHandleSize * 2.0));
            if (handleRect.contains(point)) {
                if (boxIndex) {
                    *boxIndex = i;
                }
                return entry.first;
            }
        }
    }
    return NoHandle;
}

void AnnotationCanvas::selectIndex(int index)
{
    if (m_selectedIndex == index) {
        return;
    }
    m_selectedIndex = index;
    emit selectionChanged(index);
}

void AnnotationCanvas::pushHistory()
{
    m_undoStack.append(m_boxes);
    if (m_undoStack.size() > 80) {
        m_undoStack.removeFirst();
    }
    m_redoStack.clear();
}

void AnnotationCanvas::commitDragHistoryIfNeeded()
{
    if (sameBoxes(m_dragOriginalBoxes, m_boxes)) {
        return;
    }
    m_undoStack.append(m_dragOriginalBoxes);
    if (m_undoStack.size() > 80) {
        m_undoStack.removeFirst();
    }
    m_redoStack.clear();
    m_dragOriginalBoxes.clear();
    emit boxesChanged(m_boxes);
}

void AnnotationCanvas::setBoxesFromHistory(const QVector<AnnotationBox> &boxes)
{
    m_boxes = boxes;
    if (m_selectedIndex >= m_boxes.size()) {
        m_selectedIndex = -1;
    }
    update();
    emit boxesChanged(m_boxes);
}

QRectF AnnotationCanvas::clampRect(const QRectF &rect) const
{
    QRectF normalized = rect.normalized();
    normalized.setLeft(qBound(0.0, normalized.left(), 1.0));
    normalized.setTop(qBound(0.0, normalized.top(), 1.0));
    normalized.setRight(qBound(0.0, normalized.right(), 1.0));
    normalized.setBottom(qBound(0.0, normalized.bottom(), 1.0));
    return normalized.normalized();
}

QRectF AnnotationCanvas::transformRectToView(const QRectF &rect) const
{
    const QRectF r = clampRect(rect);
    if (m_rotation == 90) {
        return QRectF(1.0 - r.bottom(), r.left(), r.height(), r.width());
    }
    if (m_rotation == 180) {
        return QRectF(1.0 - r.right(), 1.0 - r.bottom(), r.width(), r.height());
    }
    if (m_rotation == 270) {
        return QRectF(r.top(), 1.0 - r.right(), r.height(), r.width());
    }
    return r;
}

QPointF AnnotationCanvas::transformPointFromView(const QPointF &point) const
{
    const double x = qBound(0.0, point.x(), 1.0);
    const double y = qBound(0.0, point.y(), 1.0);
    if (m_rotation == 90) {
        return QPointF(y, 1.0 - x);
    }
    if (m_rotation == 180) {
        return QPointF(1.0 - x, 1.0 - y);
    }
    if (m_rotation == 270) {
        return QPointF(1.0 - y, x);
    }
    return QPointF(x, y);
}
