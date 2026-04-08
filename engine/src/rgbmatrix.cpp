/*
  Q Light Controller Plus
  rgbmatrix.cpp

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

#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QDebug>
#include <cmath>
#include <QDir>

#include "rgbscriptscache.h"
#include "qlcfixturehead.h"
#include "qlcfixturemode.h"
#include "qlcfixturedef.h"
#include "fixturegroup.h"
#include "genericfader.h"
#include "fadechannel.h"
#include "rgbmatrix.h"
#include "rgbimage.h"
#include "fixture.h"
#include "doc.h"

#define KXMLQLCRGBMatrixStartColor      QStringLiteral("MonoColor")
#define KXMLQLCRGBMatrixEndColor        QStringLiteral("EndColor")
#define KXMLQLCRGBMatrixColor           QStringLiteral("Color")
#define KXMLQLCRGBMatrixColorIndex      QStringLiteral("Index")

#define KXMLQLCRGBMatrixFixtureGroup    QStringLiteral("FixtureGroup")
#define KXMLQLCRGBMatrixDimmerControl   QStringLiteral("DimmerControl")

#define KXMLQLCRGBMatrixProperty        QStringLiteral("Property")
#define KXMLQLCRGBMatrixPropertyName    QStringLiteral("Name")
#define KXMLQLCRGBMatrixPropertyValue   QStringLiteral("Value")

#define KXMLQLCRGBMatrixControlMode         QStringLiteral("ControlMode")
#define KXMLQLCRGBMatrixControlModeRgb      QStringLiteral("RGB")
#define KXMLQLCRGBMatrixControlModeAmber    QStringLiteral("Amber")
#define KXMLQLCRGBMatrixControlModeWhite    QStringLiteral("White")
#define KXMLQLCRGBMatrixControlModeUV       QStringLiteral("UV")
#define KXMLQLCRGBMatrixControlModeDimmer   QStringLiteral("Dimmer")
#define KXMLQLCRGBMatrixControlModeShutter  QStringLiteral("Shutter")
#define KXMLQLCRGBMatrixControlModeDimmerFullRange  QStringLiteral("DimmerFullRange")
#define KXMLQLCRGBMatrixControlModeNone             QStringLiteral("None")
#define KXMLQLCRGBMatrixControlModeRgbw             QStringLiteral("RGBW")

#define KXMLQLCRGBMatrixFixtureDefChannelMap        QStringLiteral("FixtureDefChannelMap")
#define KXMLQLCRGBMatrixFixtureDefChannelMapKey     QStringLiteral("FixtureDefKey")
#define KXMLQLCRGBMatrixChannelMapping              QStringLiteral("ChannelMapping")
#define KXMLQLCRGBMatrixFixtureDefChannelMapName    QStringLiteral("ChannelName")
#define KXMLQLCRGBMatrixFixtureDefChannelMapIndex   QStringLiteral("ValueIndex")
#define KXMLQLCRGBMatrixFixtureDefChannelMapHeadMode QStringLiteral("HeadMode")

#define KXMLQLCRGBMatrixEnablePerFixtureMapping     QStringLiteral("EnablePerFixtureMapping")
#define KXMLQLCRGBMatrixSelectedRows                QStringLiteral("SelectedRows")
#define KXMLQLCRGBMatrixOverrideHTP                 QStringLiteral("OverrideHTP")
#define KXMLQLCRGBMatrixZeroTransparent             QStringLiteral("ZeroIsTransparent")

/****************************************************************************
 * Initialization
 ****************************************************************************/

RGBMatrix::RGBMatrix(Doc *doc)
    : Function(doc, Function::RGBMatrixType)
    , m_dimmerControl(false)
    , m_overrideHTP(false)
    , m_zeroIsTransparent(false)
    , m_fixtureGroupID(FixtureGroup::invalidId())
    , m_group(NULL)
    , m_requestEngineCreation(true)
    , m_runAlgorithm(NULL)
    , m_algorithm(NULL)
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
    , m_algorithmMutex(QMutex::Recursive)
#endif
     , m_stepHandler(new RGBMatrixStep())
     , m_stepsCount(0)
     , m_stepBeatDuration(0)
     , m_continuousPhase(0.0)
     , m_controlMode(RGBMatrix::ControlModeRgb)
     , m_enablePerFixtureMapping(false)
 {
     setName(tr("New RGB Matrix"));
    setDuration(500);

    m_rgbColors.fill(QColor(), RGBAlgorithmColorDisplayCount);
    setColor(0, Qt::red);

    setAlgorithm(RGBAlgorithm::algorithm(doc, "Stripes"));
}

RGBMatrix::~RGBMatrix()
{
    //if (m_runAlgorithm != NULL)
    //    delete m_runAlgorithm;
    delete m_algorithm;
    delete m_stepHandler;
}

QIcon RGBMatrix::getIcon() const
{
    return QIcon(":/rgbmatrix.png");
}

void RGBMatrix::setTotalDuration(quint32 msec)
{
    QMutexLocker algorithmLocker(&m_algorithmMutex);

    if (m_algorithm == NULL)
        return;

    FixtureGroup *grp = doc()->fixtureGroup(fixtureGroup());
    if (grp == NULL)
        return;

    int steps = m_algorithm->rgbMapStepCount(grp->size());
    setDuration(msec / steps);
}

quint32 RGBMatrix::totalDuration()
{
    QMutexLocker algorithmLocker(&m_algorithmMutex);

    if (m_algorithm == NULL)
        return 0;

    FixtureGroup *grp = doc()->fixtureGroup(fixtureGroup());
    if (grp == NULL)
        return 0;

    //qDebug () << "Algorithm steps:" << m_algorithm->rgbMapStepCount(grp->size());
    return m_algorithm->rgbMapStepCount(grp->size()) * duration();
}

void RGBMatrix::setDimmerControl(bool dimmerControl)
{
    m_dimmerControl = dimmerControl;
}

bool RGBMatrix::dimmerControl() const
{
    return m_dimmerControl;
}

void RGBMatrix::setOverrideHTP(bool enable)
{
    m_overrideHTP = enable;
}

bool RGBMatrix::overrideHTP() const
{
    return m_overrideHTP;
}

void RGBMatrix::setZeroIsTransparent(bool enable)
{
    m_zeroIsTransparent = enable;
}

bool RGBMatrix::zeroIsTransparent() const
{
    return m_zeroIsTransparent;
}

/****************************************************************************
 * Copying
 ****************************************************************************/

Function* RGBMatrix::createCopy(Doc* doc, bool addToDoc)
{
    Q_ASSERT(doc != NULL);

    Function *copy = new RGBMatrix(doc);
    if (copy->copyFrom(this) == false)
    {
        delete copy;
        copy = NULL;
    }
    if (addToDoc == true && doc->addFunction(copy) == false)
    {
        delete copy;
        copy = NULL;
    }

    return copy;
}

bool RGBMatrix::copyFrom(const Function* function)
{
    const RGBMatrix *mtx = qobject_cast<const RGBMatrix*> (function);
    if (mtx == NULL)
        return false;

    setDimmerControl(mtx->dimmerControl());
    setOverrideHTP(mtx->overrideHTP());
    setZeroIsTransparent(mtx->zeroIsTransparent());
    setFixtureGroup(mtx->fixtureGroup());

    m_rgbColors.clear();
    foreach (QColor col, mtx->getColors())
        m_rgbColors.append(col);

    if (mtx->algorithm() != NULL)
        setAlgorithm(mtx->algorithm()->clone());
    else
        setAlgorithm(NULL);

    setControlMode(mtx->controlMode());

    QMapIterator<QString, QString> it(mtx->m_properties);
    while (it.hasNext())
    {
        it.next();
        setProperty(it.key(), it.value());
    }

    // Copy fixture definition channel mappings (now includes valueIndex)
    m_fixtureDefChannelMap = mtx->m_fixtureDefChannelMap;

    // Copy multi-value mapping mode flag
    m_enablePerFixtureMapping = mtx->m_enablePerFixtureMapping;

    // Copy selected rows
    m_selectedRows = mtx->m_selectedRows;

    return Function::copyFrom(function);
}

void RGBMatrix::setScriptProperty(QString propName, QString value)
{
    QMutexLocker algoLocker(&m_algorithmMutex);
    if (m_runAlgorithm != NULL && m_runAlgorithm->type() == RGBAlgorithm::Script)
    {
        RGBScript *script = static_cast<RGBScript*> (m_runAlgorithm);
        script->setProperty(propName, value);
    }
}

/****************************************************************************
 * Fixtures
 ****************************************************************************/

quint32 RGBMatrix::fixtureGroup() const
{
    return m_fixtureGroupID;
}

void RGBMatrix::setFixtureGroup(quint32 id)
{
    m_fixtureGroupID = id;
    {
        QMutexLocker algoLocker(&m_algorithmMutex);
        m_group = doc()->fixtureGroup(m_fixtureGroupID);
    }
    m_stepsCount = algorithmStepsCount();
}

QList<quint32> RGBMatrix::components()
{
    if (m_group != NULL)
        return m_group->fixtureList();

    return QList<quint32>();
}

/****************************************************************************
 * Algorithm
 ****************************************************************************/

void RGBMatrix::setAlgorithm(RGBAlgorithm *algo)
{
    {
        QMutexLocker algorithmLocker(&m_algorithmMutex);
        delete m_algorithm;
        m_algorithm = algo;

        // Clear pending name whenever a real algorithm is assigned
        if (algo != NULL)
            m_pendingScriptName.clear();

        m_requestEngineCreation = true;

        /** If there's been a change of Script algorithm "on the fly",
         *  then re-apply the properties currently set in this RGBMatrix */
        if (m_algorithm != NULL && m_algorithm->type() == RGBAlgorithm::Script)
        {
            RGBScript *script = static_cast<RGBScript*> (m_algorithm);
            QMapIterator<QString, QString> it(m_properties);
            while (it.hasNext())
            {
                it.next();
                if (script->setProperty(it.key(), it.value()) == false)
                {
                    /** If the new algorithm doesn't expose a property,
                     *  then remove it from the cached list, otherwise
                     *  it would be carried around forever (and saved on XML) */
                    m_properties.take(it.key());
                }
            }

            QVector<uint> colors = script->rgbMapGetColors();
            for (int i = 0; i < colors.count(); i++)
                m_rgbColors.replace(i, QColor::fromRgb(colors.at(i)));
        }
    }
    m_stepsCount = algorithmStepsCount();

    emit changed(id());
}

RGBAlgorithm *RGBMatrix::algorithm() const
{
    return m_algorithm;
}

#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
QMutex& RGBMatrix::algorithmMutex()
{
    return m_algorithmMutex;
}
#else
QRecursiveMutex& RGBMatrix::algorithmMutex()
{
    return m_algorithmMutex;
}
#endif


int RGBMatrix::stepsCount()
{
    return m_stepsCount;
}

int RGBMatrix::algorithmStepsCount()
{
    QMutexLocker algorithmLocker(&m_algorithmMutex);

    if (m_algorithm == NULL)
        return 0;

    FixtureGroup *grp = doc()->fixtureGroup(fixtureGroup());
    if (grp != NULL)
        return m_algorithm->rgbMapStepCount(grp->size());

    return 0;
}

void RGBMatrix::previewMap(int step, RGBMatrixStep *handler)
{
    QMutexLocker algorithmLocker(&m_algorithmMutex);
    if (m_algorithm == NULL || handler == NULL)
        return;

    if (m_group == NULL)
        m_group = doc()->fixtureGroup(fixtureGroup());

    if (m_group != NULL)
    {
        setMapColors(m_algorithm);
        m_algorithm->rgbMap(m_group->size(), handler->stepColor().rgb(), step, handler->m_map);
    }
}

/****************************************************************************
 * Color
 ****************************************************************************/

void RGBMatrix::setColor(int i, QColor c)
{
    if (i < 0)
        return;

    if (i >= m_rgbColors.count())
        m_rgbColors.resize(i + 1);

    m_rgbColors.replace(i, c);
    {
        QMutexLocker algorithmLocker(&m_algorithmMutex);
        if (m_algorithm != NULL)
        {
            m_algorithm->setColors(m_rgbColors);
            updateColorDelta();
        }
    }
    setMapColors(m_algorithm);
    emit changed(id());
}

QColor RGBMatrix::getColor(int i) const
{
    if (i < 0 || i >= m_rgbColors.count())
        return QColor();

    return m_rgbColors.at(i);
}

QVector<QColor> RGBMatrix::getColors() const
{
    return m_rgbColors;
}

void RGBMatrix::updateColorDelta()
{
    if (m_rgbColors.count() > 1)
        m_stepHandler->calculateColorDelta(m_rgbColors[0], m_rgbColors[1], m_algorithm);
}

void RGBMatrix::resetSteps()
{
    QMutexLocker algorithmLocker(&m_algorithmMutex);
    if (m_algorithm != NULL && m_stepHandler != NULL)
    {
        QColor endColor = m_rgbColors.count() > 1 ? m_rgbColors[1] : QColor();
        m_stepHandler->initializeDirection(direction(), m_rgbColors[0], endColor, m_stepsCount, m_algorithm);
        
        // Update continuous phase when steps are reset
        if (m_stepsCount > 0)
            m_continuousPhase = double(m_stepHandler->currentStepIndex()) / double(m_stepsCount);
    }
}

void RGBMatrix::setMapColors(RGBAlgorithm *algorithm)
{
    QMutexLocker algorithmLocker(&m_algorithmMutex);
    if (algorithm == NULL)
        return;

    if (algorithm->apiVersion() < 3)
        return;

    if (m_group == NULL)
        m_group = doc()->fixtureGroup(fixtureGroup());

    QVector<unsigned int> rawColors;
    const int acceptColors = algorithm->acceptColors();
    rawColors.reserve(acceptColors);
    for (int i = 0; i < acceptColors; i++)
    {
        if (m_rgbColors.count() > i)
        {
            QColor col = m_rgbColors.at(i);
            rawColors.append(col.isValid() ? col.rgb() : 0);
        }
        else
        {
            rawColors.append(0);
        }
    }

    algorithm->rgbMapSetColors(rawColors);
}

/************************************************************************
 * Properties
 ************************************************************************/

void RGBMatrix::setProperty(QString propName, QString value)
{
    QMutexLocker algoLocker(&m_algorithmMutex);
    
    // ŁATKA: Zapisz stary stepsCount przed zmianą (dla skalowania step index)
    int oldStepsCount = m_stepsCount;
    
    m_properties[propName] = value;
    if (m_algorithm != NULL && m_algorithm->type() == RGBAlgorithm::Script)
    {
        RGBScript *script = static_cast<RGBScript*> (m_algorithm);
        script->setProperty(propName, value);

        QVector<uint> colors = script->rgbMapGetColors();
        for (int i = 0; i < colors.count(); i++)
            setColor(i, QColor::fromRgb(colors.at(i)));
    }
    m_stepsCount = algorithmStepsCount();
    
    // ŁATKA: Skaluj currentStepIndex do nowego stepsCount (zachowaj fazę)
    // Używamy m_continuousPhase (ciągła wartość 0.0-1.0) zamiast dyskretnego stepIndex
    // aby uniknąć kumulacji błędów zaokrągleń przy wielokrotnych zmianach prędkości.
    // Analogicznie do EFXFixture::durationChanged() używającego m_currentAngle.
    if (m_stepHandler != NULL && m_stepsCount > 0 && oldStepsCount != m_stepsCount)
    {
        // Jeśli m_continuousPhase nie jest jeszcze zaktualizowany (pierwsza zmiana),
        // oblicz go z aktualnego stepIndex
        if (oldStepsCount > 0)
        {
            int currentStepIndex = m_stepHandler->currentStepIndex();
            m_continuousPhase = double(currentStepIndex) / double(oldStepsCount);
        }
        
        // Przeskaluj fazę do nowego stepsCount (zachowaj ciągłą fazę, nie dyskretny index)
        int newStepIndex = int(round(m_continuousPhase * double(m_stepsCount)));
        // Upewnij się że nie przekracza zakresu
        if (newStepIndex >= m_stepsCount)
            newStepIndex = m_stepsCount - 1;
        if (newStepIndex < 0)
            newStepIndex = 0;
        m_stepHandler->setCurrentStepIndex(newStepIndex);
    }
}

QString RGBMatrix::property(QString propName)
{
    QMutexLocker algoLocker(&m_algorithmMutex);

    /** If the property is cached, then return it right away */
    QMap<QString, QString>::iterator it = m_properties.find(propName);
    if (it != m_properties.end())
        return it.value();

    /** Otherwise, let's retrieve it from the Script */
    if (m_algorithm != NULL && m_algorithm->type() == RGBAlgorithm::Script)
    {
        RGBScript *script = static_cast<RGBScript*> (m_algorithm);
        return script->property(propName);
    }

    return QString();
}

/****************************************************************************
 * JSON Settings Import/Export
 ****************************************************************************/

bool RGBMatrix::applySettingsFromJson(const QJsonObject &root, Doc *doc)
{
    bool applied = false;

    /* Pattern */
    if (root.contains("pattern"))
    {
        QJsonObject pattern = root["pattern"].toObject();
        QString algoName = pattern["algorithmName"].toString();
        RGBAlgorithm *algo = RGBAlgorithm::algorithm(doc, algoName);
        if (algo != NULL)
        {
            setAlgorithm(algo);
            applied = true;
        }
        if (pattern.contains("controlMode"))
        {
            setControlMode(static_cast<ControlMode>(pattern["controlMode"].toInt()));
            applied = true;
        }
    }

    /* Properties (script parameters) */
    if (root.contains("properties") && algorithm() != NULL &&
        algorithm()->type() == RGBAlgorithm::Script)
    {
        QJsonObject props = root["properties"].toObject();
        for (auto it = props.constBegin(); it != props.constEnd(); ++it)
        {
            setProperty(it.key(), it.value().toString());
        }
        applied = true;
    }

    /* Run Order */
    if (root.contains("runOrder"))
    {
        setRunOrder(static_cast<Function::RunOrder>(root["runOrder"].toInt()));
        applied = true;
    }

    /* Direction */
    if (root.contains("direction"))
    {
        setDirection(static_cast<Function::Direction>(root["direction"].toInt()));
        applied = true;
    }

    /* Row Filter */
    if (root.contains("rowFilter"))
    {
        QJsonArray rows = root["rowFilter"].toArray();
        QList<int> selectedRows;
        foreach (const QJsonValue &v, rows)
            selectedRows.append(v.toInt());
        setSelectedRows(selectedRows);
        applied = true;
    }

    /* Multi-Value Mapping */
    if (root.contains("multiValueMapping"))
    {
        QJsonObject mv = root["multiValueMapping"].toObject();
        setEnablePerFixtureMapping(mv["enabled"].toBool());

        if (mv.contains("fixtureMappings"))
        {
            QJsonObject fixtureMappings = mv["fixtureMappings"].toObject();
            for (auto it = fixtureMappings.constBegin(); it != fixtureMappings.constEnd(); ++it)
            {
                QString defKey = it.key();
                QJsonArray mappingArr = it.value().toArray();
                QList<ChannelMapping> mappings;
                foreach (const QJsonValue &val, mappingArr)
                {
                    QJsonObject obj = val.toObject();
                    ChannelMapping cm;
                    cm.channelName = obj["channelName"].toString();
                    cm.valueIndex = obj["valueIndex"].toInt();
                    cm.headMode = obj["headMode"].toString("Individual");
                    mappings.append(cm);
                }
                setFixtureDefChannelMappings(defKey, mappings);
            }
        }
        applied = true;
    }

    return applied;
}

/****************************************************************************
 * Load & Save
 ****************************************************************************/

bool RGBMatrix::loadXML(QXmlStreamReader &root)
{
    if (root.name() != KXMLQLCFunction)
    {
        qWarning() << Q_FUNC_INFO << "Function node not found";
        return false;
    }

    if (root.attributes().value(KXMLQLCFunctionType).toString() != typeToString(Function::RGBMatrixType))
    {
        qWarning() << Q_FUNC_INFO << "Function is not an RGB matrix";
        return false;
    }

    /* Load matrix contents */
    while (root.readNextStartElement())
    {
        if (root.name() == KXMLQLCFunctionSpeed)
        {
            loadXMLSpeed(root);
        }
        else if (root.name() == KXMLQLCRGBAlgorithm)
        {
            // For Script-type algorithms, read the name first so we can preserve
            // the reference even when the script cannot be loaded (e.g. no active license).
            QString algoType = root.attributes().value(KXMLQLCRGBAlgorithmType).toString();
            if (algoType == KXMLQLCRGBScript)
            {
                QString scriptName = root.readElementText();
                RGBScript *scr = doc()->rgbScriptsCache()->script(scriptName);
                if (scr != nullptr && scr->apiVersion() > 0 && !scr->name().isEmpty())
                {
                    m_pendingScriptName.clear();
                    setAlgorithm(scr);
                }
                else
                {
                    delete scr;
                    // Keep reference alive - will be written back on saveXML
                    m_pendingScriptName = scriptName;
                    setAlgorithm(NULL);
                }
            }
            else
            {
                m_pendingScriptName.clear();
                setAlgorithm(RGBAlgorithm::loader(doc(), root));
            }
        }
        else if (root.name() == KXMLQLCRGBMatrixFixtureGroup)
        {
            setFixtureGroup(root.readElementText().toUInt());
        }
        else if (root.name() == KXMLQLCFunctionDirection)
        {
            loadXMLDirection(root);
        }
        else if (root.name() == KXMLQLCFunctionRunOrder)
        {
            loadXMLRunOrder(root);
        }
        // Legacy support
        else if (root.name() == KXMLQLCRGBMatrixStartColor)
        {
            setColor(0, QColor::fromRgb(QRgb(root.readElementText().toUInt())));
        }
        else if (root.name() == KXMLQLCRGBMatrixEndColor)
        {
            setColor(1, QColor::fromRgb(QRgb(root.readElementText().toUInt())));
        }
        else if (root.name() == KXMLQLCRGBMatrixColor)
        {
            int colorIdx = root.attributes().value(KXMLQLCRGBMatrixColorIndex).toInt();
            setColor(colorIdx, QColor::fromRgb(QRgb(root.readElementText().toUInt())));
        }
        else if (root.name() == KXMLQLCRGBMatrixControlMode)
        {
            setControlMode(stringToControlMode(root.readElementText()));
        }
        else if (root.name() == KXMLQLCRGBMatrixProperty)
        {
            QString name = root.attributes().value(KXMLQLCRGBMatrixPropertyName).toString();
            QString value = root.attributes().value(KXMLQLCRGBMatrixPropertyValue).toString();
            setProperty(name, value);
            root.skipCurrentElement();
        }
        else if (root.name() == KXMLQLCRGBMatrixDimmerControl)
        {
            setDimmerControl(root.readElementText().toInt());
        }
        else if (root.name() == KXMLQLCRGBMatrixOverrideHTP)
        {
            setOverrideHTP(root.readElementText().toInt() != 0);
        }
        else if (root.name() == KXMLQLCRGBMatrixZeroTransparent)
        {
            setZeroIsTransparent(root.readElementText().toInt() != 0);
        }
        else if (root.name() == KXMLQLCRGBMatrixEnablePerFixtureMapping)
        {
            setEnablePerFixtureMapping(root.readElementText().toInt() != 0);
        }
        else if (root.name() == KXMLQLCRGBMatrixFixtureDefChannelMap)
        {
            // Read fixture definition key
            QString key = root.attributes().value(KXMLQLCRGBMatrixFixtureDefChannelMapKey).toString();
            
            // Check for old format (attributes on parent element) vs new format (child elements)
            QString oldChannelName = root.attributes().value(KXMLQLCRGBMatrixFixtureDefChannelMapName).toString();
            int oldValueIndex = root.attributes().value(KXMLQLCRGBMatrixFixtureDefChannelMapIndex).toInt();
            
            QList<ChannelMapping> mappings;
            
            // If old format (has ChannelName attribute), convert to new format
            if (!oldChannelName.isEmpty())
            {
                QString oldHeadMode = root.attributes().value(KXMLQLCRGBMatrixFixtureDefChannelMapHeadMode, "Individual").toString();
                mappings.append(ChannelMapping(oldChannelName, oldValueIndex, oldHeadMode));
                root.skipCurrentElement();
            }
            else
            {
                // New format: read child <ChannelMapping> elements
                while (root.readNextStartElement())
                {
                    if (root.name() == KXMLQLCRGBMatrixChannelMapping)
                    {
                        QString channelName = root.attributes().value(KXMLQLCRGBMatrixFixtureDefChannelMapName).toString();
                        int valueIndex = root.attributes().value(KXMLQLCRGBMatrixFixtureDefChannelMapIndex).toInt();
                        QString headMode = root.attributes().value(KXMLQLCRGBMatrixFixtureDefChannelMapHeadMode, "Individual").toString();
                        
                        mappings.append(ChannelMapping(channelName, valueIndex, headMode));
                        
                        root.skipCurrentElement();
                    }
                    else
                    {
                        root.skipCurrentElement();
                    }
                }
            }
            
            // Set all mappings for this fixture definition
            if (!key.isEmpty() && !mappings.isEmpty())
                setFixtureDefChannelMappings(key, mappings);
        }
        else if (root.name() == KXMLQLCRGBMatrixSelectedRows)
        {
            /* Selected Rows */
            QString rowsStr = root.readElementText();
            QStringList rowsList = rowsStr.split(",", Qt::SkipEmptyParts);
            QList<int> rows;
            foreach (QString rowStr, rowsList)
            {
                bool ok;
                int row = rowStr.toInt(&ok);
                if (ok)
                    rows.append(row);
            }
            setSelectedRows(rows);
        }
        else
        {
            qWarning() << Q_FUNC_INFO << "Unknown RGB matrix tag:" << root.name();
            root.skipCurrentElement();
        }
    }

    return true;
}

bool RGBMatrix::saveXML(QXmlStreamWriter *doc)
{
    Q_ASSERT(doc != NULL);

    /* Function tag */
    doc->writeStartElement(KXMLQLCFunction);

    /* Common attributes */
    saveXMLCommon(doc);

    /* Speeds */
    saveXMLSpeed(doc);

    /* Direction */
    saveXMLDirection(doc);

    /* Run order */
    saveXMLRunOrder(doc);

    /* Algorithm */
    if (m_algorithm != NULL)
        m_algorithm->saveXML(doc);
    else if (!m_pendingScriptName.isEmpty())
    {
        // Premium script reference preserved when license is inactive
        doc->writeStartElement(KXMLQLCRGBAlgorithm);
        doc->writeAttribute(KXMLQLCRGBAlgorithmType, KXMLQLCRGBScript);
        doc->writeCharacters(m_pendingScriptName);
        doc->writeEndElement();
    }

    /* LEGACY - Dimmer Control */
    if (dimmerControl())
        doc->writeTextElement(KXMLQLCRGBMatrixDimmerControl, QString::number(dimmerControl()));

    /* Override HTP */
    if (m_overrideHTP)
        doc->writeTextElement(KXMLQLCRGBMatrixOverrideHTP, QString::number(1));

    /* Zero Is Transparent */
    if (m_zeroIsTransparent)
        doc->writeTextElement(KXMLQLCRGBMatrixZeroTransparent, QString::number(1));

    /* Colors */
    for (int i = 0; i < m_rgbColors.count(); i++)
    {
        if (m_rgbColors.at(i).isValid() == false)
            continue;

        doc->writeStartElement(KXMLQLCRGBMatrixColor);
        doc->writeAttribute(KXMLQLCRGBMatrixColorIndex, QString::number(i));
        doc->writeCharacters(QString::number(m_rgbColors.at(i).rgb()));
        doc->writeEndElement();
    }

    /* Control Mode */
    doc->writeTextElement(KXMLQLCRGBMatrixControlMode, RGBMatrix::controlModeToString(m_controlMode));

    /* Fixture Group */
    doc->writeTextElement(KXMLQLCRGBMatrixFixtureGroup, QString::number(fixtureGroup()));

    /* Properties */
    QMapIterator<QString, QString> it(m_properties);
    while (it.hasNext())
    {
        it.next();
        doc->writeStartElement(KXMLQLCRGBMatrixProperty);
        doc->writeAttribute(KXMLQLCRGBMatrixPropertyName, it.key());
        doc->writeAttribute(KXMLQLCRGBMatrixPropertyValue, it.value());
        doc->writeEndElement();
    }

    /* Enable Per-Fixture Mapping flag */
    if (m_enablePerFixtureMapping)
    {
        doc->writeTextElement(KXMLQLCRGBMatrixEnablePerFixtureMapping, QString::number(1));
    }

    /* Fixture Definition Channel Mappings (Multi-Channel Support) */
    QMapIterator<QString, QList<ChannelMapping>> channelMapIt(m_fixtureDefChannelMap);
    while (channelMapIt.hasNext())
    {
        channelMapIt.next();
        const QString &fixtureDefKey = channelMapIt.key();
        const QList<ChannelMapping> &mappings = channelMapIt.value();
        
        // Write fixture definition group tag
        doc->writeStartElement(KXMLQLCRGBMatrixFixtureDefChannelMap);
        doc->writeAttribute(KXMLQLCRGBMatrixFixtureDefChannelMapKey, fixtureDefKey);
        
        // Write each channel mapping as a sub-element
        foreach (const ChannelMapping &mapping, mappings)
        {
            doc->writeStartElement(KXMLQLCRGBMatrixChannelMapping);
            doc->writeAttribute(KXMLQLCRGBMatrixFixtureDefChannelMapName, mapping.channelName);
            doc->writeAttribute(KXMLQLCRGBMatrixFixtureDefChannelMapIndex, QString::number(mapping.valueIndex));
            doc->writeAttribute(KXMLQLCRGBMatrixFixtureDefChannelMapHeadMode, mapping.headMode);
            doc->writeEndElement(); // </ChannelMapping>
        }
        
        doc->writeEndElement(); // </FixtureDefChannelMap>
    }

    // Save selected rows (if not all rows)
    if (!m_selectedRows.isEmpty())
    {
        QStringList rowStrings;
        foreach (int row, m_selectedRows)
            rowStrings << QString::number(row);
        doc->writeTextElement(KXMLQLCRGBMatrixSelectedRows, rowStrings.join(","));
    }

    /* End the <Function> tag */
    doc->writeEndElement();

    return true;
}

/****************************************************************************
 * Running
 ****************************************************************************/

void RGBMatrix::tap()
{
    if (stopped() == false)
    {
        FixtureGroup *grp = doc()->fixtureGroup(fixtureGroup());
        // Filter out taps that are too close to each other
        if (grp != NULL && uint(m_roundTime.elapsed()) >= (duration() / 4))
        {
            roundCheck();
            resetElapsed();
        }
    }
}

void RGBMatrix::checkEngineCreation()
{
    m_runAlgorithm = m_algorithm;
    m_requestEngineCreation = false;
}

void RGBMatrix::preRun(MasterTimer *timer)
{
    {
        QMutexLocker algorithmLocker(&m_algorithmMutex);

        m_group = doc()->fixtureGroup(m_fixtureGroupID);
        if (m_group == NULL)
        {
            // No fixture group to control
            stop(FunctionParent::master());
            return;
        }

        if (m_algorithm != NULL)
        {
            checkEngineCreation();

            // Copy direction from parent class direction
            m_stepHandler->initializeDirection(direction(), m_rgbColors[0], m_rgbColors[1], m_stepsCount, m_runAlgorithm);
            
            // Update continuous phase when starting playback
            if (m_stepsCount > 0)
                m_continuousPhase = double(m_stepHandler->currentStepIndex()) / double(m_stepsCount);

            if (m_runAlgorithm->type() == RGBAlgorithm::Script)
            {
                RGBScript *script = static_cast<RGBScript*> (m_runAlgorithm);
                QMapIterator<QString, QString> it(m_properties);
                while (it.hasNext())
                {
                    it.next();
                    script->setProperty(it.key(), it.value());
                }
            }
            else if (m_runAlgorithm->type() == RGBAlgorithm::Image)
            {
                RGBImage *image = static_cast<RGBImage*> (m_runAlgorithm);
                if (image->animatedSource())
                    image->rewindAnimation();
            }
        }
    }

    m_roundTime.restart();

    Function::preRun(timer);
}

void RGBMatrix::write(MasterTimer *timer, QList<Universe *> universes)
{
    Q_UNUSED(timer);

    {
        QMutexLocker algorithmLocker(&m_algorithmMutex);
        if (m_group == NULL)
        {
            // No fixture group to control
            stop(FunctionParent::master());
            return;
        }

        // No time to do anything.
        if (duration() == 0)
            return;

        if (m_algorithm != NULL && m_requestEngineCreation)
            checkEngineCreation();

        // Invalid/nonexistent script
        if (m_runAlgorithm == NULL || m_runAlgorithm->apiVersion() == 0)
            return;

        if (isPaused() == false)
        {
            // Get a new map every time elapsed is reset to zero
            if (elapsed() < MasterTimer::tick())
            {
                if (tempoType() == Beats)
                    m_stepBeatDuration = beatsToTime(duration(), timer->beatTimeDuration());

                //qDebug() << "RGBMatrix step" << m_stepHandler->currentStepIndex() << ", color:" << QString::number(m_stepHandler->stepColor().rgb(), 16);
                
                // Determine script size based on row filtering and paramCount
                QSize scriptSize = m_group->size();
                
                // Apply row filtering FIRST (if any rows are filtered)
                if (!m_selectedRows.isEmpty())
                {
                    // Script gets only the height of filtered rows
                    scriptSize.setHeight(m_selectedRows.count());
                }
                
                // Multiply height by paramCount if Multi-Value mode enabled
                if (m_enablePerFixtureMapping && m_runAlgorithm != NULL)
                {
                    int paramCount = m_runAlgorithm->paramCount();
                    if (paramCount > 1)
                    {
                        scriptSize.setHeight(scriptSize.height() * paramCount);
                    }
                }
                
                m_runAlgorithm->rgbMap(scriptSize, m_stepHandler->stepColor().rgb(),
                                       m_stepHandler->currentStepIndex(), m_stepHandler->m_map);
                updateMapChannels(m_stepHandler->m_map, m_group, universes);
            }
        }
    }

    if (isPaused() == false)
    {
        // Increment the ms elapsed time
        incrementElapsed();

        /* Check if we need to change direction, stop completely or go to next step
         * The cases are:
         * 1- time tempo type: act normally, on ms elapsed time
         * 2- beat tempo type, beat occurred: check if the elapsed beats is a multiple of
         *    the step beat duration. If so, proceed to the next step
         * 3- beat tempo type, not beat: if the ms elapsed time reached the step beat
         *    duration in ms, and the ms time to the next beat is not less than 1/16 of
         *    the step beat duration in ms, then proceed to the next step. If the ms time to the
         *    next beat is less than 1/16 of the step beat duration in ms, then defer the step
         *    change to case #2, to resync the matrix to the next beat
         */
        if (tempoType() == Time && elapsed() >= duration())
        {
            roundCheck();
        }
        else if (tempoType() == Beats)
        {
            if (timer->isBeat())
            {
                incrementElapsedBeats();
                qDebug() << "Elapsed beats:" << elapsedBeats() << ", time elapsed:" << elapsed() << ", step time:" << m_stepBeatDuration;
                if (elapsedBeats() % duration() == 0)
                {
                    roundCheck();
                    resetElapsed();
                }
            }
            else if (elapsed() >= m_stepBeatDuration && (uint)timer->timeToNextBeat() > m_stepBeatDuration / 16)
            {
                qDebug() << "Elapsed exceeded";
                roundCheck();
            }
        }
    }
}

void RGBMatrix::postRun(MasterTimer *timer, QList<Universe *> universes)
{
    uint fadeout = overrideFadeOutSpeed() == defaultSpeed() ? fadeOutSpeed() : overrideFadeOutSpeed();

    /* If no fade out is needed, dismiss all the requested faders.
     * Otherwise, set all the faders to fade out and let Universe dismiss them
     * when done */
    if (fadeout == 0)
    {
        dismissAllFaders();
    }
    else
    {
        if (tempoType() == Beats)
            fadeout = beatsToTime(fadeout, timer->beatTimeDuration());

        foreach (QSharedPointer<GenericFader> fader, m_fadersMap)
        {
            if (!fader.isNull())
                fader->setFadeOut(true, fadeout);
        }
    }

    m_fadersMap.clear();

    {
        QMutexLocker algorithmLocker(&m_algorithmMutex);
        checkEngineCreation();
        if (m_runAlgorithm != NULL)
            m_runAlgorithm->postRun();
    }

    Function::postRun(timer, universes);
}

void RGBMatrix::roundCheck()
{
    QMutexLocker algorithmLocker(&m_algorithmMutex);
    if (m_algorithm == NULL)
        return;

    if (m_stepHandler->checkNextStep(runOrder(), m_rgbColors[0], m_rgbColors[1], m_stepsCount) == false)
        stop(FunctionParent::master());

    // Update continuous phase based on current step index (prevents cumulative rounding errors)
    // This is analogous to how EFX uses m_currentAngle for phase scaling
    if (m_stepsCount > 0)
        m_continuousPhase = double(m_stepHandler->currentStepIndex()) / double(m_stepsCount);

    m_roundTime.restart();

    if (tempoType() == Beats)
        roundElapsed(m_stepBeatDuration);
    else
        roundElapsed(duration());
}

FadeChannel *RGBMatrix::getFader(Universe *universe, quint32 fixtureID, quint32 channel)
{
    // get the universe Fader first. If doesn't exist, create it
    if (universe == NULL)
        return NULL;

    QSharedPointer<GenericFader> fader = m_fadersMap.value(universe->id(), QSharedPointer<GenericFader>());
    if (fader.isNull())
    {
        Universe::FaderPriority prio = (blendMode() == Universe::MaskBlend ||
                                        blendMode() == Universe::SubtractiveBlend ||
                                        m_overrideHTP)
                                       ? Universe::Override
                                       : Universe::Auto;
        fader = universe->requestFader(prio);
        fader->adjustIntensity(getAttributeValue(Intensity));
        fader->setBlendMode(blendMode());
        fader->setName(name());
        fader->setParentFunctionID(id());
        m_fadersMap[universe->id()] = fader;
    }

    return fader->getChannelFader(doc(), universe, fixtureID, channel);
}

bool RGBMatrix::updateFaderValues(FadeChannel *fc, uchar value, uint fadeTime)
{
    // Zero Is Transparent: completely release the channel so other functions
    // can take control. Returning true signals the caller to remove this
    // FadeChannel from the GenericFader entirely - this is correct for both
    // HTP and LTP channels. LTP channels (e.g. color wheels) would otherwise
    // receive value 0 as their "last LTP value" and stay there indefinitely.
    if (m_zeroIsTransparent && value == 0)
    {
        fc->removeFlag(FadeChannel::Override);
        return true;
    }

    if (m_overrideHTP)
        fc->addFlag(FadeChannel::Override);
    else
        fc->removeFlag(FadeChannel::Override);

    fc->setStart(fc->current());
    fc->setTarget(value);
    fc->setElapsed(0);
    fc->setReady(false);
    // fade in/out depends on target value
    if (value == 0)
        fc->setFadeTime(fadeOutSpeed());
    else
        fc->setFadeTime(fadeTime);

    return false;
}

QLCPoint RGBMatrix::findFirstHeadPosition(const FixtureGroup *grp, quint32 fixtureId) const
{
    QMapIterator<QLCPoint, GroupHead> it(grp->headsMap());
    QLCPoint firstPos;
    bool found = false;
    
    while (it.hasNext())
    {
        it.next();
        if (it.value().fxi == fixtureId)
        {
            if (!found || it.key().y() < firstPos.y() || 
                (it.key().y() == firstPos.y() && it.key().x() < firstPos.x()))
            {
                firstPos = it.key();
                found = true;
            }
        }
    }
    
    return found ? firstPos : QLCPoint();
}

void RGBMatrix::updateMapChannels(const RGBMap& map, const FixtureGroup *grp, QList<Universe *> universes)
{
    uint fadeTime = (overrideFadeInSpeed() == defaultSpeed()) ? fadeInSpeed() : overrideFadeInSpeed();

    // Create/modify fade channels for ALL heads in the group
    QMapIterator<QLCPoint, GroupHead> it(grp->headsMap());
    while (it.hasNext())
    {
        it.next();
        QLCPoint pt = it.key();
        GroupHead grpHead = it.value();
        Fixture *fxi = doc()->fixture(grpHead.fxi);
        if (fxi == NULL)
            continue;

        QLCFixtureHead head = fxi->head(grpHead.head);

        // Skip this fixture if its row is not selected
        if (!isRowSelected(pt.y()))
            continue;

        // Map logical script row to physical fixture row
        int scriptRow = pt.y();
        if (!m_selectedRows.isEmpty())
        {
            // If row filtering is active, map logical script row to selected physical row
            int logicalRow = m_selectedRows.indexOf(pt.y());
            if (logicalRow >= 0)
                scriptRow = logicalRow;
            else
                continue; // Skip if not in selected rows
        }

        // Don't check scriptRow against map size - fixtures can read from any row via valueIndex
        // Just verify X position is valid (assuming all rows have same width)
        if (map.isEmpty() || pt.x() >= map[0].count())
            continue;

        // Check if this fixture should use "All Heads" mode
        QLCPoint scriptPos = pt;  // Default: use individual head position
        if (m_enablePerFixtureMapping && fxi->fixtureDef() != NULL)
        {
            QString defKey = getFixtureDefKey(fxi->fixtureDef());
            QList<ChannelMapping> mappings = m_fixtureDefChannelMap.value(defKey);
            
            // Check if any mapping has headMode="All"
            foreach (const ChannelMapping &mapping, mappings)
            {
                if (mapping.headMode == "All")
                {
                    scriptPos = findFirstHeadPosition(grp, grpHead.fxi);
                    break;  // Use first head position for all heads of this fixture
                }
            }
        }

        uint col = map[scriptRow < map.count() ? scriptRow : 0][scriptPos.x()];
        QVector<quint32> channelList;
        QVector<uchar> valueList;

        // Check if per-fixture mapping mode is ENABLED and there are mappings for this fixture
        bool usePerDefinitionMapping = false;
        bool hasAutoMapping = false;
        if (m_enablePerFixtureMapping && fxi->fixtureDef() != NULL && m_runAlgorithm != NULL)
        {
            QString defKey = getFixtureDefKey(fxi->fixtureDef());
            QList<ChannelMapping> mappings = m_fixtureDefChannelMap.value(defKey);
            int paramCount = m_runAlgorithm->paramCount();

            foreach (const ChannelMapping &mapping, mappings)
            {
                if (mapping.channelName.isEmpty())
                {
                    // "Auto" entry: use its offset to select the source row for the
                    // standard ControlMode path (RGB/RGBW/Dimmer/…) that runs below.
                    // The last Auto entry wins if there are multiple.
                    int sourceRow = scriptRow * paramCount + mapping.valueIndex;
                    if (sourceRow >= 0 && sourceRow < map.count() && scriptPos.x() < map[sourceRow].count())
                        col = map[sourceRow][scriptPos.x()];
                    hasAutoMapping = true;
                    continue;
                }

                // Named channel: append it with its own offset value
                quint32 channelIdx = findChannelByName(fxi->fixtureMode(), mapping.channelName);
                if (channelIdx == RGBMATRIX_VIRTUAL_DIMMER_CHANNEL)
                {
                    // Use Virtual Dimmer as normal FadeChannel with special channel 0xFFFE
                    int offset = mapping.valueIndex;
                    int sourceRow = scriptRow * paramCount + offset;

                    uint sourceCol = col;
                    if (sourceRow >= 0 && sourceRow < map.count() && scriptPos.x() < map[sourceRow].count())
                        sourceCol = map[sourceRow][scriptPos.x()];

                    uchar dimmerValue = rgbToGrey(sourceCol);
                    
                    // Get fixture's universe and use normal FadeChannel system
                    quint32 universeIndex = floor(fxi->universeAddress() / 512);
                    Universe *uni = universes.at(universeIndex);
                    
                    // Get FadeChannel for Virtual Dimmer (special channel 0xFFFE)
                    FadeChannel *fc = getFader(uni, fxi->id(), VIRTUAL_DIMMER_CHANNEL);
                    if (fc != NULL)
                    {
                        // Use updateFaderValues for Zero Transparent support
                        if (updateFaderValues(fc, dimmerValue, fadeTime))
                        {
                            // Zero Transparent: remove the channel entirely
                            QSharedPointer<GenericFader> fader = m_fadersMap.value(uni->id(), QSharedPointer<GenericFader>());
                            if (!fader.isNull())
                                fader->remove(fc);
                        }
                    }
                    usePerDefinitionMapping = true;
                }
                else if (channelIdx != QLCChannel::invalid())
                {
                    int offset = mapping.valueIndex;
                    int sourceRow = scriptRow * paramCount + offset;

                    uint sourceCol = col;
                    if (sourceRow >= 0 && sourceRow < map.count() && scriptPos.x() < map[sourceRow].count())
                        sourceCol = map[sourceRow][scriptPos.x()];

                    channelList.append(channelIdx);
                    valueList.append(rgbToGrey(sourceCol));
                    usePerDefinitionMapping = true;
                }
            }

            // If there was an Auto entry, let the standard ControlMode path below
            // handle the RGB/RGBW/Dimmer channels using the updated col.
            // Named channels (e.g. a separate Dimmer offset) are already in channelList.
            if (hasAutoMapping)
                usePerDefinitionMapping = false;
        }

        if (!usePerDefinitionMapping && m_controlMode == ControlModeNone)
            continue;

        if (!usePerDefinitionMapping && m_controlMode == ControlModeRgb)
        {
            QVector<quint32> rgbCh = head.rgbChannels();

            if (rgbCh.size() == 3)
            {
                channelList += rgbCh;
                valueList.append(qRed(col));
                valueList.append(qGreen(col));
                valueList.append(qBlue(col));
            }
            else
            {
                rgbCh = head.cmyChannels();

                if (rgbCh.size() == 3)
                {
                    channelList += rgbCh;
                    // CMY color mixing
                    QColor cmyCol(col);
                    valueList.append(cmyCol.cyan());
                    valueList.append(cmyCol.magenta());
                    valueList.append(cmyCol.yellow());
                }
            }
        }
        else if (!usePerDefinitionMapping && m_controlMode == ControlModeRgbw)
        {
            // RGBW mode: RGB from bits 0-23, White (W) from alpha bits 24-31
            QVector<quint32> rgbwCh = head.rgbwChannels();

            if (rgbwCh.size() >= 3)
            {
                channelList += rgbwCh;
                valueList.append(qRed(col));
                valueList.append(qGreen(col));
                valueList.append(qBlue(col));
                if (rgbwCh.size() == 4)
                    valueList.append(qAlpha(col));  // W channel packed in alpha bits by script
            }
        }
        else if (!usePerDefinitionMapping && m_controlMode == ControlModeShutter)
        {
            QVector<quint32> shutterCh = head.shutterChannels();

            if (shutterCh.size())
            {
                // make sure only one channel is in the list
                shutterCh.resize(1);
                channelList += shutterCh;
                valueList.append(rgbToGrey(col));
            }
        }
        else if (!usePerDefinitionMapping && (m_controlMode == ControlModeDimmer || m_dimmerControl))
        {
            // Collect all dimmers that affect current head:
            // They are the master dimmer (affects whole fixture)
            // and per-head dimmer.
            //
            // If there are no RGB or CMY channels, the least important* dimmer channel
            // is used to create grayscale image.
            //
            // The rest of the dimmer channels are set to full if dimmer control is
            // enabled and target color is > 0 (see
            // http://www.qlcplus.org/forum/viewtopic.php?f=29&t=11090)
            //
            // Note: If there is only one head, and only one dimmer channel,
            // make it a master dimmer in fixture definition.
            //
            // *least important - per head dimmer if present,
            // otherwise per fixture dimmer if present

            quint32 masterDim = fxi->masterIntensityChannel();
            quint32 headDim = head.channelNumber(QLCChannel::Intensity, QLCChannel::MSB);

            if (masterDim != QLCChannel::invalid())
            {
                channelList.append(masterDim);
                valueList.append(rgbToGrey(col));
            }

            if (headDim != QLCChannel::invalid() && headDim != masterDim)
            {
                channelList.append(headDim);
                valueList.append(rgbToGrey(col) == 0 ? 0 : 255);
            }

            if (masterDim == QLCChannel::invalid() && headDim == QLCChannel::invalid() && fxi->hasVirtualDimmer())
            {
                uchar greyVal = rgbToGrey(col);
                foreach (quint32 ch, fxi->virtualDimmerChannels())
                {
                    channelList.append(ch);
                    valueList.append(greyVal);
                }
            }
        }
        else if (!usePerDefinitionMapping && m_controlMode == ControlModeDimmerFullRange)
        {
            // Full range dimmer control mode - all dimmers use full 0-255 range
            // This is similar to ControlModeDimmer but without binary per-head dimmer
            quint32 masterDim = fxi->masterIntensityChannel();
            quint32 headDim = head.channelNumber(QLCChannel::Intensity, QLCChannel::MSB);

            if (masterDim != QLCChannel::invalid())
            {
                channelList.append(masterDim);
                valueList.append(rgbToGrey(col));
            }

            if (headDim != QLCChannel::invalid() && headDim != masterDim)
            {
                channelList.append(headDim);
                valueList.append(rgbToGrey(col)); // Full range 0-255, not binary!
            }

            if (masterDim == QLCChannel::invalid() && headDim == QLCChannel::invalid() && fxi->hasVirtualDimmer())
            {
                uchar greyVal = rgbToGrey(col);
                foreach (quint32 ch, fxi->virtualDimmerChannels())
                {
                    channelList.append(ch);
                    valueList.append(greyVal);
                }
            }
        }
        else if (!usePerDefinitionMapping)
        {
            if (m_controlMode == ControlModeWhite)
                channelList.append(head.channelNumber(QLCChannel::White, QLCChannel::MSB));
            else if (m_controlMode == ControlModeAmber)
                channelList.append(head.channelNumber(QLCChannel::Amber, QLCChannel::MSB));
            else if (m_controlMode == ControlModeUV)
                channelList.append(head.channelNumber(QLCChannel::UV, QLCChannel::MSB));

            valueList.append(rgbToGrey(col));
        }

        quint32 absAddress = fxi->universeAddress();

        for (int i = 0; i < channelList.count(); i++)
        {
            if (channelList.at(i) == QLCChannel::invalid())
                continue;

            quint32 universeIndex = floor((absAddress + channelList.at(i)) / 512);
            Universe *uni = universes.at(universeIndex);

            FadeChannel *fc = getFader(uni, grpHead.fxi, channelList.at(i));
            if (updateFaderValues(fc, valueList.at(i), fadeTime))
            {
                // ZeroIsTransparent: remove the channel entirely so it is
                // released for other functions (critical for LTP channels like
                // color wheels which would otherwise latch at 0).
                QSharedPointer<GenericFader> fader = m_fadersMap.value(uni->id(), QSharedPointer<GenericFader>());
                if (!fader.isNull())
                    fader->remove(fc);
            }
        }
    }
}

uchar RGBMatrix::rgbToGrey(uint col)
{
    uchar r = qRed(col);
    uchar g = qGreen(col);
    uchar b = qBlue(col);
    
    // Special case: if R=G=B (grayscale), return the value directly
    // This avoids floating-point precision issues
    if (r == g && g == b)
        return r;
    
    // the weights are taken from
    // https://en.wikipedia.org/wiki/YUV#SDTV_with_BT.601
    return uchar(round(0.299 * r + 0.587 * g + 0.114 * b));
}

/*********************************************************************
 * Attributes
 *********************************************************************/

int RGBMatrix::adjustAttribute(qreal fraction, int attributeId)
{
    int attrIndex = Function::adjustAttribute(fraction, attributeId);

    if (attrIndex == Intensity)
    {
        foreach (QSharedPointer<GenericFader> fader, m_fadersMap)
        {
            if (!fader.isNull())
                fader->adjustIntensity(getAttributeValue(Function::Intensity));
        }
    }

    return attrIndex;
}

/*************************************************************************
 * Blend mode
 *************************************************************************/

void RGBMatrix::setBlendMode(Universe::BlendMode mode)
{
    if (mode == blendMode())
        return;

    foreach (QSharedPointer<GenericFader> fader, m_fadersMap)
    {
        if (!fader.isNull())
            fader->setBlendMode(mode);
    }

    Function::setBlendMode(mode);
    emit changed(id());
}

/*************************************************************************
 * Control Mode
 *************************************************************************/

RGBMatrix::ControlMode RGBMatrix::controlMode() const
{
    return m_controlMode;
}

void RGBMatrix::setControlMode(RGBMatrix::ControlMode mode)
{
    m_controlMode = mode;
    emit changed(id());
}

RGBMatrix::ControlMode RGBMatrix::stringToControlMode(QString mode)
{
    if (mode == KXMLQLCRGBMatrixControlModeRgb)
        return ControlModeRgb;
    else if (mode == KXMLQLCRGBMatrixControlModeAmber)
        return ControlModeAmber;
    else if (mode == KXMLQLCRGBMatrixControlModeWhite)
        return ControlModeWhite;
    else if (mode == KXMLQLCRGBMatrixControlModeUV)
        return ControlModeUV;
    else if (mode == KXMLQLCRGBMatrixControlModeDimmer)
        return ControlModeDimmer;
    else if (mode == KXMLQLCRGBMatrixControlModeShutter)
        return ControlModeShutter;
    else if (mode == KXMLQLCRGBMatrixControlModeDimmerFullRange)
        return ControlModeDimmerFullRange;
    else if (mode == KXMLQLCRGBMatrixControlModeNone)
        return ControlModeNone;
    else if (mode == KXMLQLCRGBMatrixControlModeRgbw)
        return ControlModeRgbw;

    return ControlModeRgb;
}

QString RGBMatrix::controlModeToString(RGBMatrix::ControlMode mode)
{
    switch(mode)
    {
        default:
        case ControlModeRgb:
            return QString(KXMLQLCRGBMatrixControlModeRgb);
        break;
        case ControlModeAmber:
            return QString(KXMLQLCRGBMatrixControlModeAmber);
        break;
        case ControlModeWhite:
            return QString(KXMLQLCRGBMatrixControlModeWhite);
        break;
        case ControlModeUV:
            return QString(KXMLQLCRGBMatrixControlModeUV);
        break;
        case ControlModeDimmer:
            return QString(KXMLQLCRGBMatrixControlModeDimmer);
        break;
        case ControlModeShutter:
            return QString(KXMLQLCRGBMatrixControlModeShutter);
        break;
        case ControlModeDimmerFullRange:
            return QString(KXMLQLCRGBMatrixControlModeDimmerFullRange);
        break;
        case ControlModeNone:
            return QString(KXMLQLCRGBMatrixControlModeNone);
        break;
        case ControlModeRgbw:
            return QString(KXMLQLCRGBMatrixControlModeRgbw);
        break;
    }
}

/*************************************************************************
 * Per-Definition Channel Mapping
 *************************************************************************/

QString RGBMatrix::getFixtureDefKey(const QLCFixtureDef *def)
{
    if (def == NULL)
        return QString();

    return QString("%1|%2").arg(def->manufacturer()).arg(def->model());
}

quint32 RGBMatrix::findChannelByName(const QLCFixtureMode *mode, const QString &channelName)
{
    if (mode == NULL || channelName.isEmpty())
        return QLCChannel::invalid();

    // Special case: Virtual Dimmer
    if (channelName == "__VIRTUAL_DIMMER__")
    {
        if (mode->virtualDimmer())
            return RGBMATRIX_VIRTUAL_DIMMER_CHANNEL;
        else
            return QLCChannel::invalid();
    }

    for (int i = 0; i < mode->channels().size(); i++)
    {
        QLCChannel *ch = mode->channel(i);
        if (ch != NULL && ch->name() == channelName)
            return i;  // Return relative channel index
    }

    return QLCChannel::invalid();
}

void RGBMatrix::setFixtureDefChannelMappings(const QString &fixtureDefKey, const QList<ChannelMapping> &mappings)
{
    if (fixtureDefKey.isEmpty())
        return;

    if (mappings.isEmpty())
        m_fixtureDefChannelMap.remove(fixtureDefKey);
    else
        m_fixtureDefChannelMap[fixtureDefKey] = mappings;

    emit changed(id());
}

QList<RGBMatrix::ChannelMapping> RGBMatrix::fixtureDefChannelMappings(const QString &fixtureDefKey) const
{
    return m_fixtureDefChannelMap.value(fixtureDefKey, QList<ChannelMapping>());
}

void RGBMatrix::addChannelMapping(const QString &fixtureDefKey, const QString &channelName, int valueIndex)
{
    if (fixtureDefKey.isEmpty())
        return;
    
    QList<ChannelMapping> mappings = m_fixtureDefChannelMap.value(fixtureDefKey);
    mappings.append(ChannelMapping(channelName, valueIndex));
    m_fixtureDefChannelMap[fixtureDefKey] = mappings;
    
    emit changed(id());
}

void RGBMatrix::removeChannelMapping(const QString &fixtureDefKey, int index)
{
    if (fixtureDefKey.isEmpty() || !m_fixtureDefChannelMap.contains(fixtureDefKey))
        return;
    
    QList<ChannelMapping> mappings = m_fixtureDefChannelMap[fixtureDefKey];
    if (index >= 0 && index < mappings.size())
    {
        mappings.removeAt(index);
        
        // If no mappings left, keep an empty list (will show default single row in UI)
        m_fixtureDefChannelMap[fixtureDefKey] = mappings;
        
        emit changed(id());
    }
}

void RGBMatrix::removeChannelMappingsByOffset(const QString &fixtureDefKey, int valueIndex)
{
    if (fixtureDefKey.isEmpty() || !m_fixtureDefChannelMap.contains(fixtureDefKey))
        return;

    QList<ChannelMapping> mappings = m_fixtureDefChannelMap[fixtureDefKey];
    for (int i = mappings.size() - 1; i >= 0; i--)
    {
        if (mappings[i].valueIndex == valueIndex)
            mappings.removeAt(i);
    }
    m_fixtureDefChannelMap[fixtureDefKey] = mappings;
    emit changed(id());
}

void RGBMatrix::clearChannelMappings(const QString &fixtureDefKey)
{
    if (!fixtureDefKey.isEmpty())
    {
        m_fixtureDefChannelMap.remove(fixtureDefKey);
        emit changed(id());
    }
}

/*********************************************************************
 * Multi-Value Mapping Mode
 *********************************************************************/

void RGBMatrix::setEnablePerFixtureMapping(bool enable)
{
    m_enablePerFixtureMapping = enable;
    emit changed(id());
}

bool RGBMatrix::enablePerFixtureMapping() const
{
    return m_enablePerFixtureMapping;
}

/*********************************************************************
 * Row filtering
 *********************************************************************/

void RGBMatrix::setSelectedRows(const QList<int>& rows)
{
    m_selectedRows = rows;
    emit changed(id());
}

QList<int> RGBMatrix::selectedRows() const
{
    return m_selectedRows;
}

bool RGBMatrix::isRowSelected(int row) const
{
    // If list is empty, all rows are selected (default behavior)
    if (m_selectedRows.isEmpty())
        return true;
    
    return m_selectedRows.contains(row);
}

/*************************************************************************
 *************************************************************************
 *                          RGBMatrixStep class
 *************************************************************************
 *************************************************************************/

RGBMatrixStep::RGBMatrixStep()
    : m_direction(Function::Forward)
    , m_currentStepIndex(0)
    , m_stepColor(QColor())
    , m_crDelta(0)
    , m_cgDelta(0)
    , m_cbDelta(0)
{

}

void RGBMatrixStep::setCurrentStepIndex(int index)
{
    m_currentStepIndex = index;
}

int RGBMatrixStep::currentStepIndex() const
{
    return m_currentStepIndex;
}

void RGBMatrixStep::calculateColorDelta(QColor startColor, QColor endColor, RGBAlgorithm *algorithm)
{
    m_crDelta = 0;
    m_cgDelta = 0;
    m_cbDelta = 0;

    if (endColor.isValid() && algorithm != NULL && algorithm->acceptColors() > 1)
    {
        m_crDelta = endColor.red() - startColor.red();
        m_cgDelta = endColor.green() - startColor.green();
        m_cbDelta = endColor.blue() - startColor.blue();

        //qDebug() << "Color deltas:" << m_crDelta << m_cgDelta << m_cbDelta;
    }
}

void RGBMatrixStep::setStepColor(QColor color)
{
    m_stepColor = color;
}

QColor RGBMatrixStep::stepColor()
{
    return m_stepColor;
}

void RGBMatrixStep::updateStepColor(int stepIndex, QColor startColor, int stepsCount)
{
    if (stepsCount <= 0)
        return;

    if (stepsCount == 1)
    {
        m_stepColor = startColor;
    }
    else
    {
        m_stepColor.setRed(startColor.red() + (m_crDelta * stepIndex / (stepsCount - 1)));
        m_stepColor.setGreen(startColor.green() + (m_cgDelta * stepIndex / (stepsCount - 1)));
        m_stepColor.setBlue(startColor.blue() + (m_cbDelta * stepIndex / (stepsCount - 1)));
    }

    //qDebug() << "RGBMatrix step" << stepIndex << ", color:" << QString::number(m_stepColor.rgb(), 16);
}

void RGBMatrixStep::initializeDirection(Function::Direction direction, QColor startColor, QColor endColor, int stepsCount, RGBAlgorithm *algorithm)
{
    m_direction = direction;

    if (m_direction == Function::Forward)
    {
        setCurrentStepIndex(0);
        setStepColor(startColor);
    }
    else
    {
        setCurrentStepIndex(stepsCount - 1);

        if (endColor.isValid())
            setStepColor(endColor);
        else
            setStepColor(startColor);
    }

    calculateColorDelta(startColor, endColor, algorithm);
}

bool RGBMatrixStep::checkNextStep(Function::RunOrder order,
                                  QColor startColor, QColor endColor, int stepsNumber)
{
    if (order == Function::PingPong)
    {
        if (m_direction == Function::Forward && (m_currentStepIndex + 1) == stepsNumber)
        {
            m_direction = Function::Backward;
            m_currentStepIndex = stepsNumber - 2;
            if (endColor.isValid())
                m_stepColor = endColor;

            updateStepColor(m_currentStepIndex, startColor, stepsNumber);
        }
        else if (m_direction == Function::Backward && (m_currentStepIndex - 1) < 0)
        {
            m_direction = Function::Forward;
            m_currentStepIndex = 1;
            m_stepColor = startColor;
            updateStepColor(m_currentStepIndex, startColor, stepsNumber);
        }
        else
        {
            if (m_direction == Function::Forward)
                m_currentStepIndex++;
            else
                m_currentStepIndex--;
            updateStepColor(m_currentStepIndex, startColor, stepsNumber);
        }
    }
    else if (order == Function::SingleShot)
    {
        if (m_direction == Function::Forward)
        {
            if (m_currentStepIndex >= stepsNumber - 1)
                return false;
            else
            {
                m_currentStepIndex++;
                updateStepColor(m_currentStepIndex, startColor, stepsNumber);
            }
        }
        else
        {
            if (m_currentStepIndex <= 0)
                return false;
            else
            {
                m_currentStepIndex--;
                updateStepColor(m_currentStepIndex, startColor, stepsNumber);
            }
        }
    }
    else
    {
        if (m_direction == Function::Forward)
        {
            if (m_currentStepIndex >= stepsNumber - 1)
            {
                m_currentStepIndex = 0;
                m_stepColor = startColor;
            }
            else
            {
                m_currentStepIndex++;
                updateStepColor(m_currentStepIndex, startColor, stepsNumber);
            }
        }
        else
        {
            if (m_currentStepIndex <= 0)
            {
                m_currentStepIndex = stepsNumber - 1;
                if (endColor.isValid())
                    m_stepColor = endColor;
            }
            else
            {
                m_currentStepIndex--;
                updateStepColor(m_currentStepIndex, startColor, stepsNumber);
            }
        }
    }

    return true;
}
