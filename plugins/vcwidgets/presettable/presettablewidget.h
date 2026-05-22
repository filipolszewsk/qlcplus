/*
  QLC+ VC Widget Plugin — Preset Table
  presettablewidget.h — Apache 2.0 / public domain
*/

#pragma once

#include <QMutex>
#include <QHash>
#include <QVector>
#include <QList>
#include <QTableWidget>
#include <QHeaderView>
#include <QToolBar>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStyledItemDelegate>
#include <QSharedPointer>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QPaintEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QApplication>
#include <QClipboard>

#include "vcwidget.h"
#include "dmxsource.h"
#include "genericfader.h"
#include "qlcinputsource.h"

class Doc;
class FixtureGroup;

// ---------------------------------------------------------------------------
// Widget mode
// ---------------------------------------------------------------------------

enum class PTMode { Legacy = 0, FixtureGroup = 1 };

// ---------------------------------------------------------------------------
// Data structures
// ---------------------------------------------------------------------------

struct PTOption {
    QString name;
    uchar   value    = 0;
    QString resource; // image path (Picture cap) or "#RRGGBB" (SingleColor); empty = no icon
};

struct PTColumnTypeBinding {
    QString  manufacturer;
    QString  model;
    QString  modeName;
    qint32   channelIndex = -1;  // 0-based index within the fixture mode channels
    bool isValid() const { return channelIndex >= 0 && !manufacturer.isEmpty(); }
};

struct PTColumn {
    enum Type { Numeric, Dropdown };

    QString        name;
    Type           type    = Numeric;
    bool           fade    = true;   // true=interpolate, false=snap at 127
    QVector<PTOption> options;  // used when type == Dropdown
    int            width   = -1;     // persisted pixel width; -1 = Qt default
    PTColumnTypeBinding binding;     // used only in PTMode::FixtureGroup
};

struct PTRow {
    QString        name;
    QVector<uchar> values;  // size == number of value columns
};

struct PTOutput {
    QString    name;
    quint32    fixtureId = UINT_MAX;    // used in PTMode::Legacy
    QList<int> groupRows;               // used in PTMode::FixtureGroup: y-coords in group grid
};

// ---------------------------------------------------------------------------
// Delegate — SpinBox for Numeric, ComboBox for Dropdown
// ---------------------------------------------------------------------------

class PresetTableDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit PresetTableDelegate(QObject* parent = nullptr);

    // Column definitions provided by the widget (pointer, not owned)
    void setColumns(const QVector<PTColumn>* columns);

    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option,
                          const QModelIndex& index) const override;
    void setEditorData(QWidget* editor, const QModelIndex& index) const override;
    void setModelData(QWidget* editor, QAbstractItemModel* model,
                      const QModelIndex& index) const override;
    void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option,
                              const QModelIndex& index) const override;
    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option,
                   const QModelIndex& index) const override;

private:
    const QVector<PTColumn>* m_columns = nullptr;
};

// ---------------------------------------------------------------------------
// Main widget
// ---------------------------------------------------------------------------

class PresetTableWidget : public VCWidget, public DMXSource
{
    Q_OBJECT

public:
    explicit PresetTableWidget(QWidget* parent, Doc* doc);
    ~PresetTableWidget() override;

    // ---- Data accessors (GUI thread) -------------------------------------
    int  numValueColumns() const { return m_columns.size(); }

    const QVector<PTColumn>& columns() const { return m_columns; }
    const QVector<PTRow>&    rows()    const { return m_rows;    }
    const QVector<PTOutput>& outputs() const { return m_outputs; }

    PTMode   widgetMode()      const { return m_mode; }
    quint32  fixtureGroupId()  const { return m_fixtureGroupId; }

    void setColumns(const QVector<PTColumn>& cols);
    void setRows(const QVector<PTRow>& rows);
    void setOutputs(const QVector<PTOutput>& outs);

    // ---- VCWidget overrides -----------------------------------------------
    VCWidget* createCopy(VCWidget* parent) override;
    void      updateFeedback() override;
    bool      loadXML(QXmlStreamReader& root) override;
    bool      saveXML(QXmlStreamWriter* doc) override;
    void      editProperties() override;

    // ---- DMXSource --------------------------------------------------------
    void writeDMX(MasterTimer* timer, QList<Universe*> universes) override;

protected slots:
    void slotModeChanged(Doc::Mode mode) override;
    void slotInputValueChanged(quint32 universe, quint32 channel, uchar value) override;

protected:
    void paintEvent(QPaintEvent* e) override;
    void keyPressEvent(QKeyEvent* e) override;
    bool eventFilter(QObject* obj, QEvent* ev) override;

private slots:
    void slotAddRow();
    void slotRemoveRow();
    void slotAddColumn();
    void slotRemoveColumn();
    void slotProperties();
    void slotCellChanged(int row, int col);
    void slotColumnHeaderDoubleClicked(int logicalIndex);

    // ---- Multi-column resize ---------------------------------------------
    void slotHeaderSectionResized(int logicalIndex, int oldSize, int newSize);

    // ---- Copy / paste ----------------------------------------------------
    void slotCopySelection();
    void slotPasteSelection();

private:
    void rebuildTable();
    void refreshTableFromData();
    void setActiveRow(int outputIdx, int rowIdx);   // -1 = off
    void refreshRowHighlights();
    void syncDataFromTable();
    void syncAllDataFromTable();

    // Helper: paste a raw string value into a table item and update m_rows
    void pasteValueToItem(QTableWidgetItem* item, const QString& raw);

    // writeDMX helpers
    void writeDMXLegacy(QList<Universe*>& universes, uchar xfEffective);
    void writeDMXFixtureGroup(QList<Universe*>& universes, uchar xfEffective);

    // ---- Shared state (mutex-protected, read in writeDMX) ----------------
    mutable QMutex   m_stateMutex;
    QVector<PTColumn> m_columns;
    QVector<PTRow>    m_rows;
    QVector<PTOutput> m_outputs;
    QVector<int>      m_activeRow;           // per output, -1 = off
    QVector<int>      m_stagedRow;           // per output, -1 = no staging (only used when crossfade enabled)
    bool              m_crossfadeEnabled   = false;  // widget-level toggle
    uchar             m_crossfadeGlobalPos = 0;      // physical fader position 0-255
    uchar             m_crossfadeStartPos  = 0;      // fader position when first staging was set

    PTMode            m_mode            = PTMode::Legacy;
    quint32           m_fixtureGroupId  = UINT_MAX;  // valid only when m_mode == FixtureGroup

    // ---- DMX faders (per universe, lazy) ---------------------------------
    QHash<quint32, QSharedPointer<GenericFader>> m_faders;

    // ---- UI (GUI thread only) --------------------------------------------
    QVBoxLayout*          m_layout     = nullptr;
    QToolBar*             m_toolbar    = nullptr;
    QTableWidget*         m_table      = nullptr;
    QLabel*               m_statusBar  = nullptr;
    PresetTableDelegate*  m_delegate   = nullptr;

    // Actions hidden in Operate mode (structural/column/props)
    QAction* m_actAddCol  = nullptr;
    QAction* m_actRemCol  = nullptr;
    QAction* m_actProps   = nullptr;
    QAction* m_actColSep  = nullptr;
    QAction* m_actPropSep = nullptr;

    bool m_rebuildingTable  = false;   // guard against recursive slotCellChanged
    bool m_resizingColumns  = false;
    int  m_nameColWidth     = -1;      // persisted width of Name column (col 0)

    // Badge colors for up to 8 outputs
    static const QColor s_outputColors[8];
};
