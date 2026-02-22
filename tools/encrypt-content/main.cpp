/*
  QLC+ Content Encryption Tool
  main.cpp

  Usage:
    qlc-encrypt-content --key <hex-key> --input <file> --output <file>
    qlc-encrypt-content --key <hex-key> --input-dir <dir> --output-dir <dir>
    qlc-encrypt-content --generate-key

  Copyright (c) Filip Olszewski
  PROPRIETARY - internal tool, not distributed
*/

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QDebug>
#include <QRandomGenerator>

#include "qlccrypto.h"

static const char PREMIUM_MAGIC[] = "QLCP";
static const quint8 PREMIUM_VERSION = 1;

QByteArray encryptFile(const QByteArray &plaintext, const QByteArray &key)
{
    QByteArray header;
    header.append(PREMIUM_MAGIC, 4);
    header.append(static_cast<char>(PREMIUM_VERSION));

    QByteArray encrypted = QLCCrypto::aesEncrypt(plaintext, key);
    if (encrypted.isEmpty())
        return QByteArray();

    return header + encrypted;
}

QByteArray decryptFile(const QByteArray &fileData, const QByteArray &key)
{
    if (fileData.size() < 5 + 16)
        return QByteArray();

    if (fileData.left(4) != QByteArray(PREMIUM_MAGIC, 4))
        return QByteArray();

    return QLCCrypto::aesDecrypt(fileData.mid(5), key);
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    app.setApplicationName("qlc-encrypt-content");

    QCommandLineParser parser;
    parser.setApplicationDescription("Encrypt/decrypt QLC+ premium content files");
    parser.addHelpOption();

    QCommandLineOption keyOpt(QStringList() << "k" << "key",
        "AES-256 key as 64-char hex string", "hex-key");
    parser.addOption(keyOpt);

    QCommandLineOption inputOpt(QStringList() << "i" << "input",
        "Input file path", "file");
    parser.addOption(inputOpt);

    QCommandLineOption outputOpt(QStringList() << "o" << "output",
        "Output file path", "file");
    parser.addOption(outputOpt);

    QCommandLineOption inputDirOpt("input-dir",
        "Input directory (encrypts all .js and .qxw files)", "dir");
    parser.addOption(inputDirOpt);

    QCommandLineOption outputDirOpt("output-dir",
        "Output directory for encrypted files", "dir");
    parser.addOption(outputDirOpt);

    QCommandLineOption decryptOpt(QStringList() << "d" << "decrypt",
        "Decrypt mode (instead of encrypt)");
    parser.addOption(decryptOpt);

    QCommandLineOption genKeyOpt("generate-key",
        "Generate a new random AES-256 key");
    parser.addOption(genKeyOpt);

    parser.process(app);

    if (parser.isSet(genKeyOpt))
    {
        QByteArray randomKey(32, 0);
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
        QRandomGenerator *rng = QRandomGenerator::global();
        for (int i = 0; i < 32; i++)
            randomKey[i] = static_cast<char>(rng->bounded(256));
#else
        for (int i = 0; i < 32; i++)
            randomKey[i] = static_cast<char>(qrand() % 256);
#endif
        qInfo() << "Generated AES-256 key:" << randomKey.toHex().constData();
        qInfo() << "Use this as your MASTER_CONTENT_KEY in the license activator.";
        return 0;
    }

    if (!parser.isSet(keyOpt))
    {
        qCritical() << "Error: --key is required (64-char hex string)";
        parser.showHelp(1);
    }

    QString keyHex = parser.value(keyOpt);
    QByteArray key = QByteArray::fromHex(keyHex.toUtf8());
    if (key.size() != 32)
    {
        qCritical() << "Error: key must be exactly 64 hex characters (32 bytes)";
        return 1;
    }

    bool decryptMode = parser.isSet(decryptOpt);

    if (parser.isSet(inputDirOpt))
    {
        if (!parser.isSet(outputDirOpt))
        {
            qCritical() << "Error: --output-dir required with --input-dir";
            return 1;
        }

        QString inputDir = parser.value(inputDirOpt);
        QString outputDir = parser.value(outputDirOpt);
        QDir outDir(outputDir);
        if (!outDir.exists())
            outDir.mkpath(".");

        QStringList filters;
        if (decryptMode)
            filters << "*.qlcscript" << "*.qlcproj";
        else
            filters << "*.js" << "*.qxw";

        QDirIterator it(inputDir, filters, QDir::Files);
        int count = 0;
        while (it.hasNext())
        {
            it.next();
            QFile inFile(it.filePath());
            if (!inFile.open(QIODevice::ReadOnly))
            {
                qWarning() << "Cannot open:" << it.filePath();
                continue;
            }
            QByteArray data = inFile.readAll();
            inFile.close();

            QByteArray result;
            QString outName;

            if (decryptMode)
            {
                result = decryptFile(data, key);
                outName = it.fileName();
                if (outName.endsWith(".qlcscript"))
                    outName = outName.left(outName.length() - 10) + ".js";
                else if (outName.endsWith(".qlcproj"))
                    outName = outName.left(outName.length() - 8) + ".qxw";
            }
            else
            {
                result = encryptFile(data, key);
                outName = it.fileName();
                if (outName.endsWith(".js"))
                    outName = outName.left(outName.length() - 3) + ".qlcscript";
                else if (outName.endsWith(".qxw"))
                    outName = outName.left(outName.length() - 4) + ".qlcproj";
            }

            if (result.isEmpty())
            {
                qWarning() << "Failed to process:" << it.filePath();
                continue;
            }

            QString outPath = outputDir + QDir::separator() + outName;
            QFile outFile(outPath);
            if (outFile.open(QIODevice::WriteOnly))
            {
                outFile.write(result);
                outFile.close();
                qInfo() << (decryptMode ? "Decrypted:" : "Encrypted:") << it.fileName() << "->" << outName;
                count++;
            }
        }
        qInfo() << "Processed" << count << "files.";
        return 0;
    }

    if (!parser.isSet(inputOpt) || !parser.isSet(outputOpt))
    {
        qCritical() << "Error: --input and --output required";
        parser.showHelp(1);
    }

    QFile inFile(parser.value(inputOpt));
    if (!inFile.open(QIODevice::ReadOnly))
    {
        qCritical() << "Cannot open input file:" << parser.value(inputOpt);
        return 1;
    }
    QByteArray data = inFile.readAll();
    inFile.close();

    QByteArray result;
    if (decryptMode)
        result = decryptFile(data, key);
    else
        result = encryptFile(data, key);

    if (result.isEmpty())
    {
        qCritical() << "Processing failed";
        return 1;
    }

    QFile outFile(parser.value(outputOpt));
    if (!outFile.open(QIODevice::WriteOnly))
    {
        qCritical() << "Cannot open output file:" << parser.value(outputOpt);
        return 1;
    }
    outFile.write(result);
    outFile.close();

    qInfo() << "Done:" << parser.value(inputOpt) << "->" << parser.value(outputOpt);
    return 0;
}
