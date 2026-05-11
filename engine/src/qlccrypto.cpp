/*
  Q Light Controller Plus
  qlccrypto.cpp

  Copyright (c) Filip Olszewski

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

#include <QCryptographicHash>
#include <QNetworkInterface>
#include <QStorageInfo>
#include <QHostInfo>
#include <QDebug>

#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
#include <QRandomGenerator>
#endif

#if defined(__APPLE__) || defined(Q_OS_MAC)
#include <IOKit/IOKitLib.h>
#include <CoreFoundation/CoreFoundation.h>
#endif

#include "qlccrypto.h"

extern "C" {
#include "crypto/aes.h"
}

QByteArray QLCCrypto::deriveKey(const QString &input)
{
    return QCryptographicHash::hash(input.toUtf8(), QCryptographicHash::Sha256);
}

QString QLCCrypto::generateHardwareFingerprint()
{
    QString data;

#if defined(__APPLE__) || defined(Q_OS_MAC)
    // Use IOPlatformUUID - a stable, unique per-board identifier that never changes
    // even across OS reinstalls, VPN connections, or Wi-Fi MAC randomization.
    // kIOMainPortDefault introduced in macOS 12; use kIOMasterPortDefault on older targets.
#if MAC_OS_X_VERSION_MIN_REQUIRED >= 120000
    io_service_t service = IOServiceGetMatchingService(kIOMainPortDefault,
                                                       IOServiceMatching("IOPlatformExpertDevice"));
#else
    io_service_t service = IOServiceGetMatchingService(kIOMasterPortDefault,
                                                       IOServiceMatching("IOPlatformExpertDevice"));
#endif
    if (service)
    {
        CFStringRef uuidRef = (CFStringRef)IORegistryEntryCreateCFProperty(
            service, CFSTR("IOPlatformUUID"), kCFAllocatorDefault, 0);
        if (uuidRef)
        {
            char buf[64];
            if (CFStringGetCString(uuidRef, buf, sizeof(buf), kCFStringEncodingUTF8))
                data += QString::fromUtf8(buf);
            CFRelease(uuidRef);
        }
        IOObjectRelease(service);
    }
#endif

    // Stable cross-platform component (root disk device path)
    // hostname intentionally excluded -- it changes with network/VPN and would
    // invalidate the license file every time the user switches connections.
    data += QString::fromUtf8(QStorageInfo::root().device());

    // Fallback: if somehow data is still empty, use sorted physical interface MACs
    // (skip virtual interfaces: utun/VPN, bridge, vmnet, docker, tun, tap)
    if (data.isEmpty())
    {
        QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
        QStringList macs;
        for (const QNetworkInterface &iface : interfaces)
        {
            if (iface.flags().testFlag(QNetworkInterface::IsLoopBack))
                continue;
            if (iface.hardwareAddress().isEmpty())
                continue;
            const QString name = iface.name();
            if (name.startsWith("utun") || name.startsWith("bridge") ||
                name.startsWith("vmnet") || name.startsWith("docker") ||
                name.startsWith("tun") || name.startsWith("tap") ||
                name.startsWith("veth") || name.startsWith("lo"))
                continue;
            macs.append(iface.hardwareAddress());
        }
        macs.sort();
        data += macs.join(QString());
    }

    return QString(QCryptographicHash::hash(data.toUtf8(), QCryptographicHash::Sha256).toHex());
}

QString QLCCrypto::generateLegacyHardwareFingerprint()
{
    // Preserved for migration only -- matches the old fingerprint that included
    // localHostName(). Used to attempt decryption of license files created before
    // the hostname was removed from the fingerprint.
    QString data;

#if defined(__APPLE__) || defined(Q_OS_MAC)
#if MAC_OS_X_VERSION_MIN_REQUIRED >= 120000
    io_service_t service = IOServiceGetMatchingService(kIOMainPortDefault,
                                                       IOServiceMatching("IOPlatformExpertDevice"));
#else
    io_service_t service = IOServiceGetMatchingService(kIOMasterPortDefault,
                                                       IOServiceMatching("IOPlatformExpertDevice"));
#endif
    if (service)
    {
        CFStringRef uuidRef = (CFStringRef)IORegistryEntryCreateCFProperty(
            service, CFSTR("IOPlatformUUID"), kCFAllocatorDefault, 0);
        if (uuidRef)
        {
            char buf[64];
            if (CFStringGetCString(uuidRef, buf, sizeof(buf), kCFStringEncodingUTF8))
                data += QString::fromUtf8(buf);
            CFRelease(uuidRef);
        }
        IOObjectRelease(service);
    }
#endif

    data += QHostInfo::localHostName();
    data += QString::fromUtf8(QStorageInfo::root().device());

    if (data.isEmpty())
    {
        QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
        QStringList macs;
        for (const QNetworkInterface &iface : interfaces)
        {
            if (iface.flags().testFlag(QNetworkInterface::IsLoopBack))
                continue;
            if (iface.hardwareAddress().isEmpty())
                continue;
            const QString name = iface.name();
            if (name.startsWith("utun") || name.startsWith("bridge") ||
                name.startsWith("vmnet") || name.startsWith("docker") ||
                name.startsWith("tun") || name.startsWith("tap") ||
                name.startsWith("veth") || name.startsWith("lo"))
                continue;
            macs.append(iface.hardwareAddress());
        }
        macs.sort();
        data += macs.join(QString());
    }

    return QString(QCryptographicHash::hash(data.toUtf8(), QCryptographicHash::Sha256).toHex());
}

QByteArray QLCCrypto::pkcs7Pad(const QByteArray &data, int blockSize)
{
    int padding = blockSize - (data.size() % blockSize);
    QByteArray padded = data;
    padded.append(QByteArray(padding, static_cast<char>(padding)));
    return padded;
}

QByteArray QLCCrypto::pkcs7Unpad(const QByteArray &data)
{
    if (data.isEmpty())
        return data;

    int padding = static_cast<unsigned char>(data.at(data.size() - 1));
    if (padding < 1 || padding > BLOCK_LEN)
        return QByteArray();

    for (int i = data.size() - padding; i < data.size(); i++)
    {
        if (static_cast<unsigned char>(data.at(i)) != padding)
            return QByteArray();
    }

    return data.left(data.size() - padding);
}

QByteArray QLCCrypto::aesEncrypt(const QByteArray &plaintext, const QByteArray &key)
{
    if (key.size() != KEY_LEN)
    {
        qWarning() << "QLCCrypto::aesEncrypt: invalid key size" << key.size();
        return QByteArray();
    }

    QByteArray iv(BLOCK_LEN, 0);
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
    QRandomGenerator *rng = QRandomGenerator::global();
    for (int i = 0; i < BLOCK_LEN; i++)
        iv[i] = static_cast<char>(rng->bounded(256));
#else
    for (int i = 0; i < BLOCK_LEN; i++)
        iv[i] = static_cast<char>(qrand() % 256);
#endif

    QByteArray padded = pkcs7Pad(plaintext, BLOCK_LEN);

    struct AES_ctx ctx;
    AES_init_ctx_iv(&ctx, reinterpret_cast<const uint8_t*>(key.constData()),
                    reinterpret_cast<const uint8_t*>(iv.constData()));
    QByteArray encrypted = padded;
    AES_CBC_encrypt_buffer(&ctx, reinterpret_cast<uint8_t*>(encrypted.data()),
                           static_cast<uint32_t>(encrypted.size()));

    return iv + encrypted;
}

QByteArray QLCCrypto::aesDecrypt(const QByteArray &ciphertext, const QByteArray &key)
{
    if (key.size() != KEY_LEN)
    {
        qWarning() << "QLCCrypto::aesDecrypt: invalid key size" << key.size();
        return QByteArray();
    }

    if (ciphertext.size() < BLOCK_LEN * 2)
    {
        qWarning() << "QLCCrypto::aesDecrypt: data too short";
        return QByteArray();
    }

    QByteArray iv = ciphertext.left(BLOCK_LEN);
    QByteArray encrypted = ciphertext.mid(BLOCK_LEN);

    if (encrypted.size() % BLOCK_LEN != 0)
    {
        qWarning() << "QLCCrypto::aesDecrypt: invalid ciphertext alignment";
        return QByteArray();
    }

    struct AES_ctx ctx;
    AES_init_ctx_iv(&ctx, reinterpret_cast<const uint8_t*>(key.constData()),
                    reinterpret_cast<const uint8_t*>(iv.constData()));
    QByteArray decrypted = encrypted;
    AES_CBC_decrypt_buffer(&ctx, reinterpret_cast<uint8_t*>(decrypted.data()),
                           static_cast<uint32_t>(decrypted.size()));

    return pkcs7Unpad(decrypted);
}
