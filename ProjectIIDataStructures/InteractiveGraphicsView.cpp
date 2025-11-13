#include "InteractiveGraphicsView.h"

#include <QBrush>
#include <QColor>
#include <QGraphicsItem>
#include <QKeyEvent>
#include <QLinearGradient>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QScrollBar>
#include <QWheelEvent>
#include <QTransform>
#include <algorithm>
#include <cmath>

InteractiveGraphicsView::InteractiveGraphicsView(QWidget *parent)
    : QGraphicsView(parent),
      m_currentScale(1.0),
      m_minScale(0.1),
      m_maxScale(25.0),
      m_autoFitEnabled(true),
      m_userAdjusted(false),
      m_preserveContentScale(false)
{
    setDragMode(QGraphicsView::ScrollHandDrag);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorViewCenter);
    setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform | QPainter::TextAntialiasing);
    setFrameStyle(QFrame::NoFrame);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    updateBackgroundBrush();
}

void InteractiveGraphicsView::setAutoFitEnabled(bool enabled)
{
    m_autoFitEnabled = enabled;
    if (m_autoFitEnabled && !m_lastContentRect.isNull())
    {
        resetToFit();
    }
}

bool InteractiveGraphicsView::autoFitEnabled() const
{
    return m_autoFitEnabled;
}

bool InteractiveGraphicsView::hasUserAdjusted() const
{
    return m_userAdjusted;
}

void InteractiveGraphicsView::setPreserveContentScale(bool preserve)
{
    if (m_preserveContentScale == preserve)
    {
        return;
    }

    m_preserveContentScale = preserve;

    if (m_preserveContentScale)
    {
        resetTransform();
        m_currentScale = 1.0;
        m_userAdjusted = false;
        if (!m_lastContentRect.isNull())
        {
            centerOn(m_lastContentRect.center());
        }
    }
}

bool InteractiveGraphicsView::preserveContentScale() const
{
    return m_preserveContentScale;
}

void InteractiveGraphicsView::setBackgroundImage(const QPixmap &pixmap)
{
    m_customBackground = pixmap;
    updateBackgroundBrush();
}

void InteractiveGraphicsView::clearBackgroundImage()
{
    if (m_customBackground.isNull())
    {
        return;
    }
    m_customBackground = QPixmap();
    updateBackgroundBrush();
}

void InteractiveGraphicsView::setContentRect(const QRectF &rect, bool forceFit)
{
    m_lastContentRect = rect;
    if (scene())
    {
        scene()->setSceneRect(rect);
    }
    if ((m_autoFitEnabled && !m_userAdjusted) || forceFit)
    {
        resetToFit();
    }
}

void InteractiveGraphicsView::resetToFit()
{
    if (!scene())
    {
        return;
    }
    QRectF target = m_lastContentRect.isNull() ? scene()->itemsBoundingRect() : m_lastContentRect;
    if (target.isNull())
    {
        return;
    }
    if (!viewport())
    {
        return;
    }

    QSize viewSize = viewport()->size();
    if (viewSize.isEmpty())
    {
        return;
    }

    resetTransform();
    m_currentScale = 1.0;

    if (target.width() <= 0.0 || target.height() <= 0.0)
    {
        centerOn(target.center());
        m_userAdjusted = false;
        return;
    }

    if (m_preserveContentScale)
    {
        centerOn(target.center());
        m_userAdjusted = false;
        return;
    }

    // Añadir margen extra al rectángulo objetivo para mostrar desde más lejos
    // Reducir el margen para mostrar el mapa con más zoom (más cerca)
    QRectF expandedTarget = target.adjusted(
        -target.width() * 0.08,   // 8% de margen (antes 15%)
        -target.height() * 0.08,  // 8% de margen
        target.width() * 0.08,    // 8% de margen
        target.height() * 0.08    // 8% de margen
    );

    qreal scaleX = static_cast<qreal>(viewSize.width()) / expandedTarget.width();
    qreal scaleY = static_cast<qreal>(viewSize.height()) / expandedTarget.height();
    qreal desiredScale = std::min(scaleX, scaleY);

    if (std::isfinite(desiredScale) && desiredScale > 0.0)
    {
        qreal fitScale = std::min(desiredScale, m_maxScale);
        if (!qFuzzyCompare(fitScale, 1.0))
        {
            QTransform transform;
            transform.scale(fitScale, fitScale);
            setTransform(transform);
        }
        m_currentScale = fitScale;
    }

    centerOn(target.center());
    m_userAdjusted = false;
}

void InteractiveGraphicsView::zoomIn()
{
    zoomBy(1.15);
}

void InteractiveGraphicsView::zoomOut()
{
    zoomBy(1.0 / 1.15);
}

void InteractiveGraphicsView::wheelEvent(QWheelEvent *event)
{
    if (event->angleDelta().y() == 0)
    {
        QGraphicsView::wheelEvent(event);
        return;
    }

    if (m_preserveContentScale)
    {
        QGraphicsView::wheelEvent(event);
        return;
    }

    constexpr qreal stepFactor = 1.15;
    qreal factor = event->angleDelta().y() > 0 ? stepFactor : 1.0 / stepFactor;
    zoomBy(factor);
    event->accept();
}

void InteractiveGraphicsView::keyPressEvent(QKeyEvent *event)
{
    switch (event->key())
    {
    case Qt::Key_Plus:
    case Qt::Key_Equal:
        if (m_preserveContentScale)
        {
            event->accept();
            return;
        }
        zoomBy(1.15);
        event->accept();
        return;
    case Qt::Key_Minus:
    case Qt::Key_Underscore:
        if (m_preserveContentScale)
        {
            event->accept();
            return;
        }
        zoomBy(1.0 / 1.15);
        event->accept();
        return;
    case Qt::Key_0:
        if (m_preserveContentScale)
        {
            resetToFit();
            event->accept();
            return;
        }
        resetToFit();
        event->accept();
        return;
    case Qt::Key_Left:
        panByPixels(-40, 0);
        event->accept();
        return;
    case Qt::Key_Right:
        panByPixels(40, 0);
        event->accept();
        return;
    case Qt::Key_Up:
        panByPixels(0, -40);
        event->accept();
        return;
    case Qt::Key_Down:
        panByPixels(0, 40);
        event->accept();
        return;
    default:
        break;
    }
    QGraphicsView::keyPressEvent(event);
}

void InteractiveGraphicsView::resizeEvent(QResizeEvent *event)
{
    QGraphicsView::resizeEvent(event);
    updateBackgroundBrush();
    if (m_autoFitEnabled && !m_userAdjusted && !m_lastContentRect.isNull())
    {
        resetToFit();
    }
}

void InteractiveGraphicsView::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        QPointF scenePoint = mapToScene(event->pos());
        QGraphicsItem *item = scene() ? scene()->itemAt(scenePoint, QTransform()) : nullptr;
        
        if (!item || !item->data(1).isValid())
        {
            emit scenePointActivated(scenePoint);
            event->accept();
            return;
        }
    }
    QGraphicsView::mouseDoubleClickEvent(event);
}

void InteractiveGraphicsView::zoomBy(qreal factor)
{
    if (m_preserveContentScale)
    {
        return;
    }
    qreal newScale = m_currentScale * factor;
    qreal minAllowed = std::min(m_minScale, m_currentScale);
    if (newScale < minAllowed)
    {
        factor = minAllowed / m_currentScale;
        newScale = minAllowed;
    }
    else if (newScale > m_maxScale)
    {
        factor = m_maxScale / m_currentScale;
        newScale = m_maxScale;
    }

    if (!qFuzzyCompare(factor, 1.0))
    {
        scale(factor, factor);
        m_currentScale = newScale;
        m_userAdjusted = true;
    }
}

void InteractiveGraphicsView::panByPixels(int dx, int dy)
{
    horizontalScrollBar()->setValue(horizontalScrollBar()->value() + dx);
    verticalScrollBar()->setValue(verticalScrollBar()->value() + dy);
    m_userAdjusted = true;
}

void InteractiveGraphicsView::updateBackgroundBrush()
{
    if (!m_customBackground.isNull())
    {
        QSize viewSize = viewport() ? viewport()->size() : QSize();
        if (!viewSize.isEmpty())
        {
            QPixmap scaled = m_customBackground.scaled(viewSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            setBackgroundBrush(QBrush(scaled));
        }
        else
        {
            setBackgroundBrush(QBrush(m_customBackground));
        }
        return;
    }

    QLinearGradient gradient(0.0, 0.0, 0.0, 1.0);
    gradient.setCoordinateMode(QGradient::ObjectBoundingMode);
    gradient.setColorAt(0.0, QColor(30, 39, 46));
    gradient.setColorAt(1.0, QColor(44, 62, 80));
    setBackgroundBrush(QBrush(gradient));
}
