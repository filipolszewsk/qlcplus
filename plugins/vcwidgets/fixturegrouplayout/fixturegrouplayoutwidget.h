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
#include <QColor>
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
class MaskGridItemDelegate;

struct MaskPreset
{
    QString name;
    QSet<QLCPoint> cells;
};

/** Persisted on VC widget; runtime policy is pushed to Doc. */
enum class MaskEfxConflictPolicy
{
    None = 0,
    Wait,
    Override
};

struct MaskDisplayColors
{
    QColor visible         = QColor(0x22, 0x22, 0x22);
    QColor hidden          = QColor(0x00, 0x00, 0x00);
    QColor hiddenText      = QColor(0x70, 0x70, 0x70);
    QColor headerVisible   = QColor(0x22, 0x22, 0x22);
    QColor selectionBorder = QColor(0xe6, 0x7e, 0x22);
};

class FixtureGroupLayoutWidget : public VCWidget
{
    Q_OBJECT

public:
    static const quint8 presetSelectInputSourceId = 0;

    FixtureGroupLayoutWidget(QWidget* parent, Doc* doc);
    ~FixtureGroupLayoutWidget() override;

    VCWidget* createCopy(VCWidget* parent) override;

    void updateFeedback() override;

    bool loadXML(QXmlStreamReader& root) override;
    bool saveXML(QXmlStreamWriter* doc) override;

    void editProperties() override;

    void toClipboardJson(QJsonObject& obj, const Doc* doc) const override;
    void fromClipboardJson(const QJsonObject& obj, Doc* doc) override;

    quint32 fixtureGroupId() const { return m_fixtureGroupId; }
    void setFixtureGroupId(quint32 id);

    MaskDisplayColors maskDisplayColors() const { return m_maskColors; }
    void setMaskDisplayColors(const MaskDisplayColors& colors);

    MaskEfxConflictPolicy maskEfxConflictPolicy() const { return m_maskEfxConflictPolicy; }
    void setMaskEfxConflictPolicy(MaskEfxConflictPolicy policy);

    bool isPointMaskedOut(const QLCPoint& pt) const;
    bool isGridCellSelected(int row, int column) const;

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

protected slots:
    void slotModeChanged(Doc::Mode mode) override;
    void slotFixtureGroupChanged(quint32 id);
    void slotFixtureGroupRemoved(quint32 id);
    void slotFixtureGroupMaskChanged(quint32 id);
    void slotCellActivated(int row, int column);
    void slotCellChanged(int row, int column);
    void slotClearMaskClicked();
    void slotApplyMaskClicked();
    void slotSubtractMaskClicked();
    void slotNewPresetClicked();
    void slotOverwritePresetClicked();
    void slotDeletePresetClicked();
    void slotPresetComboChanged(int index);
    void slotEfxConflictComboChanged(int index);
    void slotInputValueChanged(quint32 universe, quint32 channel, uchar value) override;

private:
    FixtureGroup* fixtureGroup() const;
    void rebuildGrid();
    void updateCaptionLabel();
    void applyColumnHeaderStyles();
    void applyTableSelectionStyle();
    void syncMaskFromDoc();
    void pushMaskToDoc();
    bool isColumnMaskedOut(int column) const;

    QSet<QLCPoint> selectedCells() const;
    QSet<QLCPoint> validatedCells(const QSet<QLCPoint>& cells, const FixtureGroup* grp) const;
    void applyMaskFromCells(const QSet<QLCPoint>& cells);
    QSet<QLCPoint> currentMaskCellsForPreset() const;
    void clearGridSelection();
    void deselectCellAt(int row, int column);
    void updatePresetButtonStates();
    void populatePresetCombo();
    void recallPreset(int presetIndex);
    void activatePresetComboIndex(int comboIndex);
    void applyPresetFromChannelValue(uchar value);
    void populateEfxConflictCombo();
    void syncEfxConflictCombo();

    QList<QLCPoint> selectedPoints() const;
    void reselectPoints(const QList<QLCPoint>& points);
    QPoint cellAtPos(const QPoint& pos) const;
    void moveSelectedHeads(int deltaX, int deltaY);

    quint32 m_fixtureGroupId;
    QVBoxLayout* m_layout;
    QLabel* m_titleLabel;
    QPushButton* m_applyMaskButton;
    QPushButton* m_subtractMaskButton;
    QPushButton* m_clearMaskButton;
    QPushButton* m_newPresetButton;
    QPushButton* m_overwritePresetButton;
    QPushButton* m_deletePresetButton;
    QComboBox* m_presetCombo;
    QComboBox* m_efxConflictCombo;
    QTableWidget* m_table;
    MaskGridItemDelegate* m_gridDelegate;
    FixtureGroupMask m_localMask;
    QVector<MaskPreset> m_maskPresets;
    MaskDisplayColors m_maskColors;
    MaskEfxConflictPolicy m_maskEfxConflictPolicy = MaskEfxConflictPolicy::Override;

    int m_lastRow;
    int m_lastColumn;

    bool m_dragging;
    bool m_erasingSelection;
    QPoint m_eraseLastCell;
    QPoint m_dragStartCell;
    QPoint m_dragCurrentCell;
    QList<QLCPoint> m_dragOriginalPoints;
};
