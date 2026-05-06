/*
  Q Light Controller Plus
  rgbmatrix.h

  Copyright (c) Heikki Junnila
                Massimo Callegari

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0.txt

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

#ifndef RGBMATRIX_H
#define RGBMATRIX_H

#include <QElapsedTimer>
#include <QJsonObject>
#include <QJsonArray>
#include <QVector>
#include <QColor>
#include <QList>
#include <QSize>
#include <QPair>
#include <QMap>
#include <QMutex>

#ifdef QT_QML_LIB
  #include "rgbscriptv4.h"
#else
  #include "rgbscript.h"
#endif
#include "function.h"
#include "qlcpoint.h"

/** Reserved channel index for Virtual Dimmer in RGB Matrix multi mapping.
 *  This is not a physical channel - it targets Universe virtual dimmer scaling. */
#define RGBMATRIX_VIRTUAL_DIMMER_CHANNEL quint32(0xFFFD)
#define RGBMATRIX_VIRTUAL_STROBE_CHANNEL quint32(0xFFFB)

class FixtureGroup;
class GenericFader;
class FadeChannel;
class QLCFixtureDef;
class QLCFixtureMode;
class QDir;

/** @addtogroup engine_functions Functions
 * @{
 */

class RGBMatrixStep
{
public:
    RGBMatrixStep();
    ~RGBMatrixStep() { }

public:
    /** Set/Get the current step index */
    void setCurrentStepIndex(int index);
    int currentStepIndex() const;

    /** Calculate the RGB components delta between $startColor and $endColor */
    void calculateColorDelta(QColor startColor, QColor endColor, RGBAlgorithm *algorithm);

    /** Set/Get the final color of the next step to be reproduced */
    void setStepColor(QColor color);
    QColor stepColor();

    /** Update the color of the next step to be reproduced, considering the step index,
     *  the start color and the steps count */
    void updateStepColor(int step, QColor startColor, int stepsCount);

    /** Initialize the playback direction and set the initial step index and
      * color based on $startColor and $endColor */
    void initializeDirection(Function::Direction direction, QColor startColor, QColor endColor, int stepsCount, RGBAlgorithm *algorithm);

    /** Check the steps progression based on $order and the internal m_direction.
     *  This method returns true if the RGBMatrix can continue to run, otherwise
     *  false is returned and the caller should stop the RGBMatrix */
    bool checkNextStep(Function::RunOrder order, QColor startColor, QColor endColor, int stepsNumber);

public:
    /** Matrix RGB data of the current step */
    RGBMap m_map;

private:
    /** The current direction of the steps playback */
    Function::Direction m_direction;
    /** The index of the algorithm step currently being reproduced */
    int m_currentStepIndex;
    /** The RGB color passed to the currently loaded algorithm */
    QColor m_stepColor;
    /** Color delta values of the RGB components between each step */
    int m_crDelta, m_cgDelta, m_cbDelta;
};

class RGBMatrix : public Function
{
    Q_OBJECT
    Q_DISABLE_COPY(RGBMatrix)

   /*********************************************************************
     * Initialization
     *********************************************************************/
public:
    RGBMatrix(Doc* parent);
    ~RGBMatrix();

    /** @reimp */
    QIcon getIcon() const;

    /*********************************************************************
     * Contents
     *********************************************************************/
public:
    /** @reimp */
    void setTotalDuration(quint32 msec);

    /** @reimp */
    quint32 totalDuration();

    /** Set the matrix to control or not the dimmer channel */
    void setDimmerControl(bool dimmerControl);

    /** Get the matrix ability to control the dimmer channel */
    bool dimmerControl() const;

    /** When enabled, the matrix writes all channels with forceLTP=true,
     *  overriding even HTP channels regardless of their current value. */
    void setOverrideHTP(bool enable);
    bool overrideHTP() const;

    /** When enabled, any channel value of 0 produced by the script is treated
     *  as transparent: the fader channel is released and other functions take
     *  control. Useful with ForceHTP scripts that output 0 in idle state so the
     *  function can be stopped externally without leaving DMX channels frozen. */
    void setZeroIsTransparent(bool enable);
    bool zeroIsTransparent() const;

private:
    // LEGACY: replaced by ControlModeDimmer
    bool m_dimmerControl;
    bool m_overrideHTP;
    bool m_zeroIsTransparent;

    /*********************************************************************
     * Copying
     *********************************************************************/
public:
    /** @reimp */
    virtual Function* createCopy(Doc* doc, bool addToDoc = true);

    /** @reimp */
    virtual bool copyFrom(const Function* function);

    /************************************************************************
     * Fixture Group
     ************************************************************************/
public:
    /** Get/Set the Fixture Group associated to this RGBMatrix */
    quint32 fixtureGroup() const;
    void setFixtureGroup(quint32 id);

    /** @reimp */
    QList<quint32> components();

private:
    quint32 m_fixtureGroupID;
    FixtureGroup *m_group;

    /************************************************************************
     * Algorithm
     ************************************************************************/
public:
    /** Set the current RGB Algorithm. RGBMatrix takes ownership of the pointer. */
    void setAlgorithm(RGBAlgorithm* algo);

    /** Get the current RGB Algorithm. */
    RGBAlgorithm* algorithm() const;

    /** Get the algorithm protection mutex */
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
    QMutex& algorithmMutex();
#else
    QRecursiveMutex& algorithmMutex();
#endif

    /** Get the number of steps of the current algorithm */
    int stepsCount();

    /** Get the preview of the current algorithm at the given step */
    void previewMap(int step, RGBMatrixStep *handler);

private:
    int algorithmStepsCount();

private:
    bool m_requestEngineCreation;
    RGBAlgorithm *m_runAlgorithm;
    RGBAlgorithm *m_algorithm;
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
    QMutex m_algorithmMutex;
#else
    QRecursiveMutex m_algorithmMutex;
#endif

    /************************************************************************
     * Color
     ************************************************************************/
public:
    void setColor(int i, QColor c);
    QColor getColor(int i) const;
    QVector <QColor> getColors() const;

    void updateColorDelta();

    /** Reset animation steps to the beginning based on direction */
    void resetSteps();

    /** Set the colors of the current algorithm */
    void setMapColors(RGBAlgorithm *algorithm);

private:
    QVector<QColor> m_rgbColors;
    RGBMatrixStep *m_stepHandler;

    /************************************************************************
     * Properties
     ************************************************************************/
public:
    /** Set the value of the property with the given name */
    void setProperty(QString propName, QString value);

    /** Retrieve the value of the property with the given name */
    QString property(QString propName);

private:
    /** A map of the custom properties for this matrix */
    QMap<QString, QString>m_properties;

    /************************************************************************
     * JSON Settings Import/Export
     ************************************************************************/
public:
    enum RGBMatrixCopyGroup
    {
        CopyRGBPattern    = (1 << 8),
        CopyRGBProperties = (1 << 9),
        CopyRGBRowFilter  = (1 << 10),
        CopyRGBMultiValue = (1 << 11),
    };

    /** @reimp */
    QList<QPair<int, QString>> copyableParameterGroups() const override;
    /** @reimp */
    void settingsToJson(QJsonObject &obj, int flags, const Doc *doc) const override;
    /** @reimp -- delegates to legacy applySettingsFromJson(root, doc) */
    bool applySettingsFromJson(const QJsonObject &obj, int flags, Doc *doc) override;

    /** Legacy: Apply settings from a JSON object produced by the editor copy button.
     *  Only sections present in the JSON are applied.
     *  Returns true if any settings were applied. */
    bool applySettingsFromJson(const QJsonObject &root, Doc *doc);

    /************************************************************************
     * Load & Save
     ************************************************************************/
public:
    /** @reimp */
    bool loadXML(QXmlStreamReader &root);

    /** @reimp */
    bool saveXML(QXmlStreamWriter *doc);

    /************************************************************************
     * Running
     ************************************************************************/
public:
    /** @reimp */
    void tap();

    /** @reimp */
    void preRun(MasterTimer *timer);

    /** @reimp */
    void write(MasterTimer *timer, QList<Universe*> universes);

    /** @reimp */
    void postRun(MasterTimer *timer, QList<Universe*> universes);

    /** Set a script property to a specific value */
    void setScriptProperty(QString propName, QString value);

private:
    /** Check what should be done when elapsed() >= duration() */
    void roundCheck();

    /** Check if the engine needs to be re-created */
    void checkEngineCreation();

    FadeChannel *getFader(Universe *universe, quint32 fixtureID, quint32 channel);
    bool updateFaderValues(FadeChannel *fc, uchar value, uint fadeTime);

    /** Find the first (top-left) position of a fixture in the group */
    QLCPoint findFirstHeadPosition(const FixtureGroup *grp, quint32 fixtureId) const;

    /** Update FadeChannels when $map has changed since last time */
    void updateMapChannels(const RGBMap& map, const FixtureGroup* grp, QList<Universe *> universes);

public:
    /** Convert color values to fader value */
    static uchar rgbToGrey(uint col);

private:
    /** Reference to a timer counting the time in ms between steps */
    QElapsedTimer m_roundTime;

    /** The number of steps returned by the currently loaded algorithm */
    int m_stepsCount;

     /** The duration of a step based on the current BPM (Beats tempo only) */
     uint m_stepBeatDuration;

     /** Continuous phase (0.0 - 1.0) for accurate phase scaling during runtime speed changes.
      *  This prevents cumulative rounding errors when properties are changed multiple times.
      *  Analogous to EFX's m_currentAngle, but for RGBMatrix step-based animations. */
     double m_continuousPhase;
 
     /*********************************************************************
      * Attributes
      *********************************************************************/
public:
    /** @reimp */
    int adjustAttribute(qreal fraction, int attributeId);

    /*************************************************************************
     * Blend mode
     *************************************************************************/
public:
    /** @reimp */
    void setBlendMode(Universe::BlendMode mode);

    /*************************************************************************
     * Control Mode
     *************************************************************************/
public:
    /** Control modes for the RGB Matrix */
    enum ControlMode
    {
        ControlModeRgb = 0,
        ControlModeWhite,
        ControlModeAmber,
        ControlModeUV,
        ControlModeDimmer,
        ControlModeShutter,
        ControlModeDimmerFullRange,
        ControlModeNone,
        ControlModeRgbw
    };

    /** Get/Set the control mode associated to this RGBMatrix */
    ControlMode controlMode() const;
    void setControlMode(ControlMode mode);

    /** Return a control mode from a string */
    static ControlMode stringToControlMode(QString mode);

    /** Return a string from a control mode, to be saved into a XML */
    static QString controlModeToString(ControlMode mode);

private:
    ControlMode m_controlMode;

    /*************************************************************************
     * Per-Definition Channel Mapping (Multi-Channel Support)
     *************************************************************************/
public:
    /** Structure for a single channel mapping (channel + offset + head mode) */
    struct ChannelMapping {
        QString channelName;  // Which channel to use (empty = Auto)
        int valueIndex;       // Which offset from multi-value array to use
        QString headMode;     // "All" or "Individual" (default: "Individual")
        
        ChannelMapping() : valueIndex(0), headMode("Individual") {}
        ChannelMapping(const QString &ch, int idx, const QString &hm = "Individual") 
            : channelName(ch), valueIndex(idx), headMode(hm) {}
    };

    /** Set all channel mappings for a specific fixture definition */
    void setFixtureDefChannelMappings(const QString &fixtureDefKey, const QList<ChannelMapping> &mappings);

    /** Get all channel mappings for a specific fixture definition */
    QList<ChannelMapping> fixtureDefChannelMappings(const QString &fixtureDefKey) const;
    
    /** Add a new channel mapping to a fixture definition */
    void addChannelMapping(const QString &fixtureDefKey, const QString &channelName, int valueIndex);
    
    /** Remove a channel mapping at specific index */
    void removeChannelMapping(const QString &fixtureDefKey, int index);
    
    /** Remove all channel mappings with a given valueIndex (offset) */
    void removeChannelMappingsByOffset(const QString &fixtureDefKey, int valueIndex);
    
    /** Clear all channel mappings for a fixture definition */
    void clearChannelMappings(const QString &fixtureDefKey);

    /** Generate fixture definition key from definition */
    static QString getFixtureDefKey(const QLCFixtureDef *def);

    /** Find channel index by name in a fixture mode */
    static quint32 findChannelByName(const QLCFixtureMode *mode, const QString &channelName);

    /*********************************************************************
     * Multi-Value Mapping Mode
     *********************************************************************/
public:
    /** Enable/disable per-fixture mapping mode */
    void setEnablePerFixtureMapping(bool enable);
    
    /** Check if per-fixture mapping mode is enabled */
    bool enablePerFixtureMapping() const;

    /*********************************************************************
     * Row filtering
     *********************************************************************/
public:
    /** Set selected rows (which rows from fixture group are affected) */
    void setSelectedRows(const QList<int>& rows);
    
    /** Get selected rows */
    QList<int> selectedRows() const;
    
    /** Check if a specific row is selected */
    bool isRowSelected(int row) const;

private slots:
    void slotFixtureRemoved(quint32 fxi_id);

private:
    /** Map of fixture definition key (manufacturer|model) to list of channel mappings */
    QMap<QString, QList<ChannelMapping>> m_fixtureDefChannelMap;

    /** Pre-resolved channel indices: defKey → vector of channel indices (one per mapping).
     *  Built in preRun() to avoid per-step string scanning in updateMapChannels(). */
    QHash<QString, QVector<quint32>> m_resolvedChannelIndices;

    /** Enable per-fixture mapping mode (false = AUTO/normal RGB) */
    bool m_enablePerFixtureMapping;
    
    /** List of selected row indices (empty = all rows selected) */
    QList<int> m_selectedRows;

    /** Name of a premium script that failed to load (e.g. no active license).
     *  Preserved so the reference survives save/load cycles even without a license. */
    QString m_pendingScriptName;
};

/** @} */

#endif
