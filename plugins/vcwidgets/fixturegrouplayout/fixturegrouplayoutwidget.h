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
#include <QComboBox>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QJsonObject>
#include <QMap>
#include <QList>
#include <QVector>
#include <QPoint>
#include <QSet>

#include "vcwidget.h"
#include "qlcpoint.h"
#include "grouphead.h"
#include "fixturegroupmask.h"

class FixtureGroup;

struct MaskPreset
{
    QString name;
    QSet<QLCPoint> cells;
};

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
    void slotApplyMaskClicked();
    void slotSavePresetClicked();
    void slotRecallPresetClicked();
    void slotDeletePresetClicked();

private:
    FixtureGroup* fixtureGroup() const;
    void rebuildGrid();
    void updateCaptionLabel();
    void applyColumnHeaderStyles();
    void syncMaskFromDoc();
    void pushMaskToDoc();
    bool isPointMaskedOut(const QLCPoint& pt) const;
    bool isColumnMaskedOut(int column) const;

    QSet<QLCPoint> selectedCells() const;
    QSet<QLCPoint> validatedCells(const QSet<QLCPoint>& cells, const FixtureGroup* grp) const;
    void applyMaskFromCells(const QSet<QLCPoint>& cells);
    void populatePresetCombo();
    void recallPreset(int index);

    QList<QLCPoint> selectedPoints() const;
    void reselectPoints(const QList<QLCPoint>& points);
    QPoint cellAtPos(const QPoint& pos) const;
    void moveSelectedHeads(int deltaX, int deltaY);

    quint32 m_fixtureGroupId;
    QVBoxLayout* m_layout;
    QLabel* m_titleLabel;
    QPushButton* m_applyMaskButton;
    QPushButton* m_clearMaskButton;
    QPushButton* m_savePresetButton;
    QPushButton* m_recallPresetButton;
    QPushButton* m_deletePresetButton;
    QComboBox* m_presetCombo;
    QTableWidget* m_table;
    FixtureGroupMask m_localMask;
    QVector<MaskPreset> m_maskPresets;

    int m_lastRow;
    int m_lastColumn;

    bool m_dragging;
    QPoint m_dragStartCell;
    QPoint m_dragCurrentCell;
    QList<QLCPoint> m_dragOriginalPoints;
};
