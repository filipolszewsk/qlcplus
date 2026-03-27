/*
  Q Light Controller Plus
  qlccrypto.h

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

#ifndef QLCCRYPTO_H
#define QLCCRYPTO_H

#include <QByteArray>
#include <QString>

class QLCCrypto
{
public:
    static QByteArray aesEncrypt(const QByteArray &plaintext, const QByteArray &key);
    static QByteArray aesDecrypt(const QByteArray &ciphertext, const QByteArray &key);
    static QByteArray deriveKey(const QString &input);
    static QString generateHardwareFingerprint();
    static QString generateLegacyHardwareFingerprint();

    static const int KEY_LEN = 32;
    static const int BLOCK_LEN = 16;

private:
    static QByteArray pkcs7Pad(const QByteArray &data, int blockSize);
    static QByteArray pkcs7Unpad(const QByteArray &data);
};

#endif // QLCCRYPTO_H
