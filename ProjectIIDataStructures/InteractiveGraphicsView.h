#pragma once

#include <QGraphicsView>
#include <QRectF>

class InteractiveGraphicsView : public QGraphicsView
{
    Q_OBJECT

public:
    explicit InteractiveGraphicsView(QWidget *parent = nullptr);

    void setAutoFitEnabled(bool enabled);
    bool autoFitEnabled() const;
    bool hasUserAdjusted() const;

    void setContentRect(const QRectF &rect, bool forceFit = false);
    void resetToFit();

protected:
    void wheelEvent(QWheelEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void zoomBy(qreal factor);
    void panByPixels(int dx, int dy);
    void applyBackground();

    qreal m_currentScale;
    qreal m_minScale;
    qreal m_maxScale;
    bool m_autoFitEnabled;
    bool m_userAdjusted;
    QRectF m_lastContentRect;
};
