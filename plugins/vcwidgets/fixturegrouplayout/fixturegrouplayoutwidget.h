/*
  QLC+ VC Widget Plugin — Fixture Group Layout
  fixturegrouplayoutwidget.h — Apache 2.0 / public domain
*/

#pragma once

#include <QTableWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QJsonObject>
#include <QMap>
#include <QList>
#include <QPoint>

#include "vcwidget.h"
#include "qlcpoint.h"
#include "grouphead.h"
#include "fixturegroupmask.h"

class FixtureGroup;

class FixtureGroupLayoutWidget : public VCWidget
{
    Q_OBJECT

public:
    FixtureGroupLayoutWidget(QWidget* parent, Doc* doc);
    ~FixtureGroupLayoutWidget() override;

    VCWidget* createCopy(VCWidget* parent) override;

    void updateFeedback() override {}

    bool loadXML(QXmlStreamReader& root) override;
    bool saveXML(QXmlStreamWriter* doc) override;

    void editProperties() override;

    void toClipboardJson(QJsonObject& obj, const Doc* doc) const override;
    void fromClipboardJson(const QJsonObject& obj, Doc* doc) override;

    quint32 fixtureGroupId() const { return m_fixtureGroupId; }
    void setFixtureGroupId(quint32 id);

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

protected slots:
    void slotModeChanged(Doc::Mode mode) override;
    void slotFixtureGroupChanged(quint32 id);
    void slotFixtureGroupRemoved(quint32 id);
    void slotFixtureGroupMaskChanged(quint32 id);
    void slotCellActivated(int row, int column);
    void slotCellChanged(int row, int column);
    void slotColumnHeaderClicked(int column);
    void slotClearMaskClicked();

private:
    FixtureGroup* fixtureGroup() const;
    void rebuildGrid();
    void updateCaptionLabel();
    void applyColumnHeaderStyles();
    void syncMaskFromDoc();
    void pushMaskToDoc();
    bool isColumnMaskedOut(int column) const;

    QList<QLCPoint> selectedPoints() const;
    void reselectPoints(const QList<QLCPoint>& points);
    QPoint cellAtPos(const QPoint& pos) const;
    void moveSelectedHeads(int deltaX, int deltaY);

    quint32 m_fixtureGroupId;
    QVBoxLayout* m_layout;
    QLabel* m_titleLabel;
    QPushButton* m_clearMaskButton;
    QTableWidget* m_table;
    FixtureGroupMask m_localMask;

    int m_lastRow;
    int m_lastColumn;

    bool m_dragging;
    QPoint m_dragStartCell;
    QPoint m_dragCurrentCell;
    QList<QLCPoint> m_dragOriginalPoints;
};
