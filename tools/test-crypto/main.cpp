#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>
#include <QFile>
#include <QDebug>
#include <QRandomGenerator>

#include "qlccrypto.h"

static const char PREMIUM_MAGIC[] = "QLCP";
static const quint8 PREMIUM_VERSION = 1;

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    qInfo() << "=== QLC+ License Protection System - Test ===\n";

    // --- Step 1: Generate hardware fingerprint ---
    QString hwFingerprint = QLCCrypto::generateHardwareFingerprint();
    qInfo() << "1. Hardware fingerprint:" << hwFingerprint;

    // --- Step 2: Generate a master content key ---
    QByteArray masterKey(32, 0);
    QRandomGenerator *rng = QRandomGenerator::global();
    for (int i = 0; i < 32; i++)
        masterKey[i] = static_cast<char>(rng->bounded(256));
    QString masterKeyHex = QString(masterKey.toHex());
    qInfo() << "2. Generated master content key:" << masterKeyHex;

    // --- Step 3: Create a sample JS script ---
    QString sampleScript =
        "var algo = {};\n"
        "algo.name = \"Premium Test Script\";\n"
        "algo.apiVersion = 3;\n"
        "algo.acceptColors = 2;\n"
        "algo.paramCount = 1;\n"
        "algo.properties = [];\n"
        "algo.rgbMapStepCount = function(width, height) { return width; };\n"
        "algo.rgbMapSetColors = function(rawColors) {};\n"
        "algo.rgbMap = function(width, height, rgb, step) {\n"
        "  var map = new Array(height);\n"
        "  for (var y = 0; y < height; y++) {\n"
        "    map[y] = new Array(width);\n"
        "    for (var x = 0; x < width; x++) {\n"
        "      map[y][x] = (x === step) ? rgb : 0;\n"
        "    }\n"
        "  }\n"
        "  return map;\n"
        "};\n";
    qInfo() << "3. Sample script created (" << sampleScript.size() << "bytes)";

    // --- Step 4: Encrypt the script as .qlcscript ---
    QByteArray plainBytes = sampleScript.toUtf8();
    QByteArray encrypted = QLCCrypto::aesEncrypt(plainBytes, masterKey);
    QByteArray premiumFile;
    premiumFile.append(PREMIUM_MAGIC, 4);
    premiumFile.append(static_cast<char>(PREMIUM_VERSION));
    premiumFile.append(encrypted);

    QFile encFile("test_premium.qlcscript");
    encFile.open(QIODevice::WriteOnly);
    encFile.write(premiumFile);
    encFile.close();
    qInfo() << "4. Encrypted script saved as test_premium.qlcscript (" << premiumFile.size() << "bytes)";

    // --- Step 5: Simulate license activator - create .qlckey ---
    QJsonObject payload;
    payload["magic"] = QString("QLCPLUS_LICENSE");
    payload["content_key"] = masterKeyHex;
    payload["customer_name"] = QString("Test User");
    payload["customer_email"] = QString("test@example.com");
    payload["license_key"] = QString("test-key-12345");
    payload["activated_at"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

    QByteArray jsonData = QJsonDocument(payload).toJson(QJsonDocument::Compact);
    QByteArray encryptionKey = QLCCrypto::deriveKey(hwFingerprint);
    QByteArray encryptedKey = QLCCrypto::aesEncrypt(jsonData, encryptionKey);

    QFile keyFile("test.qlckey");
    keyFile.open(QIODevice::WriteOnly);
    keyFile.write(encryptedKey);
    keyFile.close();
    qInfo() << "5. License key file saved as test.qlckey (" << encryptedKey.size() << "bytes)";

    // --- Step 6: Now simulate what the fork does - load .qlckey ---
    qInfo() << "\n=== Simulating QLC+ Fork loading ===\n";

    QFile readKeyFile("test.qlckey");
    readKeyFile.open(QIODevice::ReadOnly);
    QByteArray readEncrypted = readKeyFile.readAll();
    readKeyFile.close();

    QString readHw = QLCCrypto::generateHardwareFingerprint();
    QByteArray readDecryptKey = QLCCrypto::deriveKey(readHw);
    QByteArray decryptedJson = QLCCrypto::aesDecrypt(readEncrypted, readDecryptKey);

    QJsonDocument readDoc = QJsonDocument::fromJson(decryptedJson);
    QJsonObject readObj = readDoc.object();

    if (readObj.value("magic").toString() == "QLCPLUS_LICENSE")
    {
        qInfo() << "6. License VALID!";
        qInfo() << "   Licensed to:" << readObj.value("customer_name").toString();
        qInfo() << "   Email:" << readObj.value("customer_email").toString();
        qInfo() << "   Content key recovered:" << readObj.value("content_key").toString().left(16) << "...";
    }
    else
    {
        qCritical() << "6. License INVALID (wrong hardware?)";
        return 1;
    }

    // --- Step 7: Decrypt the premium file ---
    QByteArray contentKey = QByteArray::fromHex(readObj.value("content_key").toString().toUtf8());

    QFile readPremium("test_premium.qlcscript");
    readPremium.open(QIODevice::ReadOnly);
    QByteArray premiumData = readPremium.readAll();
    readPremium.close();

    if (premiumData.left(4) != QByteArray(PREMIUM_MAGIC, 4))
    {
        qCritical() << "7. Not a premium file!";
        return 1;
    }

    QByteArray payloadData = premiumData.mid(5);
    QByteArray decryptedScript = QLCCrypto::aesDecrypt(payloadData, contentKey);

    qInfo() << "7. Premium script decrypted! Size:" << decryptedScript.size() << "bytes";
    qInfo() << "   First line:" << QString::fromUtf8(decryptedScript).split('\n').first();

    // --- Step 8: Verify content matches original ---
    if (decryptedScript == plainBytes)
        qInfo() << "\n8. SUCCESS: Decrypted content matches original!";
    else
        qCritical() << "\n8. FAILURE: Content mismatch!";

    // --- Step 9: Test with WRONG hardware fingerprint ---
    qInfo() << "\n=== Test: Wrong hardware fingerprint ===\n";
    QByteArray wrongKey = QLCCrypto::deriveKey("wrong-fingerprint-from-another-machine");
    QByteArray wrongDecrypt = QLCCrypto::aesDecrypt(readEncrypted, wrongKey);

    if (wrongDecrypt.isEmpty())
        qInfo() << "9. CORRECT: .qlckey cannot be decrypted with wrong hardware - returns empty";
    else
    {
        QJsonDocument wrongDoc = QJsonDocument::fromJson(wrongDecrypt);
        if (wrongDoc.isNull() || wrongDoc.object().value("magic").toString() != "QLCPLUS_LICENSE")
            qInfo() << "9. CORRECT: .qlckey decrypted to garbage with wrong hardware";
        else
            qCritical() << "9. PROBLEM: .qlckey decrypted with wrong hardware?!";
    }

    // --- Step 10: Test decrypting premium file with wrong key ---
    qInfo() << "\n=== Test: Wrong content key ===\n";
    QByteArray wrongContentKey(32, 'X');
    QByteArray wrongPremiumDecrypt = QLCCrypto::aesDecrypt(payloadData, wrongContentKey);
    if (wrongPremiumDecrypt.isEmpty() || wrongPremiumDecrypt != plainBytes)
        qInfo() << "10. CORRECT: Premium file cannot be decrypted with wrong key";
    else
        qCritical() << "10. PROBLEM: Premium file decrypted with wrong key?!";

    qInfo() << "\n=== All tests complete ===";

    // Cleanup
    QFile::remove("test_premium.qlcscript");
    QFile::remove("test.qlckey");

    return 0;
}
