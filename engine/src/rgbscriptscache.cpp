/*
  Q Light Controller Plus
  rgbscriptscache.cpp

  Copyright (c) David Garyga

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

#include <QDebug>
#include <QDir>

#include "rgbscriptscache.h"
#ifdef QT_QML_LIB
  #include "rgbscriptv4.h"
#else
  #include "rgbscript.h"
#endif
#include "licensemanager.h"
#include "qlcconfig.h"
#include "qlcfile.h"
#include "doc.h"

RGBScriptsCache::RGBScriptsCache(Doc* doc)
    : m_doc(doc)
{
}

QStringList RGBScriptsCache::names() const
{
    return m_scriptsMap.keys();
}

RGBScript* RGBScriptsCache::script(QString name) const
{
    RGBScript *mScript = new RGBScript(m_doc);
    QString filename = m_scriptsMap.value(name);
    if (filename.isEmpty())
    {
        return mScript;
    }
    else
    {
        mScript->load(filename);
        return mScript;
    }
}

bool RGBScriptsCache::load(const QDir& dir)
{
    qDebug() << "Loading RGB scripts in " << dir.path() << "...";

    if (dir.exists() == false || dir.isReadable() == false)
        return false;

    foreach (QString file, dir.entryList())
    {
        if (!file.endsWith(".js", Qt::CaseInsensitive) &&
            !file.endsWith(PREMIUM_SCRIPT_EXT, Qt::CaseInsensitive))
        {
            qDebug() << "    " << file << " skipped (not a .js or .qlcscript file)";
            continue;
        }
        QFile absFile(dir.absoluteFilePath(file));
        QString absFilename = absFile.fileName();

        if (m_scriptsMap.value(absFilename).isEmpty())
        {
            if (!absFile.open(QIODevice::ReadOnly | QIODevice::Text))
                return false;

            QTextStream in(&absFile);
            QString line = in.readLine();
            while (!line.isNull())
            {
                QStringList tokens = line.split("=");
                if (tokens.length() == 2 && tokens[0].simplified() == "algo.name")
                {
                    QString algoName = tokens[1].simplified().remove('"');
                    algoName.remove(';');
                    m_scriptsMap.insert(algoName, absFilename);
                    qDebug() << "    " << algoName << "script loaded";
                    break;
                }
                line = in.readLine();
            }
            absFile.close();
        }
        else
        {
            qDebug() << "    " << file << " already known";
        }
    }
    return true;
}

bool RGBScriptsCache::loadPremium(const QDir& dir)
{
    qDebug() << "Loading premium RGB scripts in " << dir.path() << "...";

    if (!dir.exists() || !dir.isReadable())
        return false;

    LicenseManager *lm = m_doc ? m_doc->licenseManager() : nullptr;
    if (!lm || !lm->isLicensed())
    {
        qDebug() << "    Skipping premium scripts (no license)";
        return true;
    }

    foreach (QString file, dir.entryList())
    {
        if (!file.endsWith(PREMIUM_SCRIPT_EXT, Qt::CaseInsensitive))
            continue;

        QFile absFile(dir.absoluteFilePath(file));
        QString absFilename = absFile.fileName();

        if (!m_scriptsMap.value(absFilename).isEmpty())
        {
            qDebug() << "    " << file << " already known";
            continue;
        }

        if (!absFile.open(QIODevice::ReadOnly))
            continue;

        QByteArray encData = absFile.readAll();
        absFile.close();

        QByteArray decrypted = lm->decryptPremiumFile(encData);
        if (decrypted.isEmpty())
        {
            qWarning() << "    " << file << " failed to decrypt";
            continue;
        }

        QString contents = QString::fromUtf8(decrypted);
        QStringList lines = contents.split('\n');
        for (const QString &line : lines)
        {
            QStringList tokens = line.split("=");
            if (tokens.length() == 2 && tokens[0].simplified() == "algo.name")
            {
                QString algoName = tokens[1].simplified().remove('"');
                algoName.remove(';');
                m_scriptsMap.insert(algoName, absFilename);
                qDebug() << "    " << algoName << "premium script loaded";
                break;
            }
        }
    }
    return true;
}

QDir RGBScriptsCache::systemScriptsDirectory()
{
    return QLCFile::systemDirectory(QString(RGBSCRIPTDIR), QString(".js"));
}

QDir RGBScriptsCache::userScriptsDirectory()
{
    return QLCFile::userDirectory(QString(USERRGBSCRIPTDIR), QString(RGBSCRIPTDIR),
            QStringList() << QString("*%1").arg(".js") << QString("*%1").arg(PREMIUM_SCRIPT_EXT));
}
