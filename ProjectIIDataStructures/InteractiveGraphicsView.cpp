#include "InteractiveGraphicsView.h"

#include <QBrush>
#include <QColor>
#include <QKeyEvent>
#include <QLinearGradient>
#include <QScrollBar>
#include <QWheelEvent>

InteractiveGraphicsView::InteractiveGraphicsView(QWidget *parent)
    : QGraphicsView(parent),
      m_currentScale(1.0),
      m_minScale(0.25),
      m_maxScale(5.0),
      m_autoFitEnabled(true),
      m_userAdjusted(false)
{
    setDragMode(QGraphicsView::ScrollHandDrag);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorViewCenter);
    setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform | QPainter::TextAntialiasing);
    setFrameStyle(QFrame::NoFrame);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    applyBackground();
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
    fitInView(target, Qt::KeepAspectRatio);
    m_currentScale = transform().m11();
    m_userAdjusted = false;
}

void InteractiveGraphicsView::wheelEvent(QWheelEvent *event)
{
    if (event->angleDelta().y() == 0)
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
        zoomBy(1.15);
        event->accept();
        return;
    case Qt::Key_Minus:
    case Qt::Key_Underscore:
        zoomBy(1.0 / 1.15);
        event->accept();
        return;
    case Qt::Key_0:
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
    if (m_autoFitEnabled && !m_userAdjusted && !m_lastContentRect.isNull())
    {
        resetToFit();
    }
}

void InteractiveGraphicsView::zoomBy(qreal factor)
{
    qreal newScale = m_currentScale * factor;
    if (newScale < m_minScale)
    {
        factor = m_minScale / m_currentScale;
        newScale = m_minScale;
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

void InteractiveGraphicsView::applyBackground()
{
    QLinearGradient gradient(0.0, 0.0, 0.0, 1.0);
    gradient.setCoordinateMode(QGradient::ObjectBoundingMode);
    gradient.setColorAt(0.0, QColor(30, 39, 46));
    gradient.setColorAt(1.0, QColor(44, 62, 80));
    setBackgroundBrush(QBrush(gradient));
}
