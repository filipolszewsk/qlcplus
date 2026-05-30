/*
  Fixture group spatial preview: chase order, offset°, phase.
*/

#pragma once

#include "ptspatialfixtureplan.h"

#include <QWidget>

class PTSpatialFixtureGridWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PTSpatialFixtureGridWidget(QWidget* parent = nullptr);

    void setPreview(const PTSpatialGridPreview& preview);
    void setPlaceholderText(const QString& text);

protected:
    void paintEvent(QPaintEvent* event) override;
    QSize sizeHint() const override;

private:
    PTSpatialGridPreview m_preview;
    QString m_placeholder;
};
