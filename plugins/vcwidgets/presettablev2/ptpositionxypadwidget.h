/*
  ptpositionxypadwidget.h — XY pad for Position Mode editing (degrees)
*/

#pragma once

#include <QWidget>

class Fixture;

class PTPositionXYPadWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PTPositionXYPadWidget(QWidget* parent = nullptr);

    void setFixture(Fixture* fxi, int head);
    void setNormalizedPosition(qreal xNorm, qreal yNorm);
    qreal xNorm() const { return m_xNorm; }
    qreal yNorm() const { return m_yNorm; }
    bool hasFixture() const { return m_fixture != nullptr; }

signals:
    void positionChanged(qreal xNorm, qreal yNorm);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    QSize sizeHint() const override;

private:
    void updateFromMouse(const QPoint& pos);
    QRect padRect() const;

    Fixture* m_fixture = nullptr;
    int      m_head = 0;
    qreal    m_xNorm = 0.5;
    qreal    m_yNorm = 0.5;
    bool     m_dragging = false;
};
