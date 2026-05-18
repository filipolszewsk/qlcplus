/*
  Q Light Controller Plus
  vcwidgetplugininstaller.h

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

#ifndef VCWIDGETPLUGININSTALLER_H
#define VCWIDGETPLUGININSTALLER_H

#include <QString>

/**
 * VCWidgetPluginInstaller handles installing a VC widget plugin from a
 * file path into the user's plugin directory.
 *
 * Accepts two formats:
 *  - Bare binary (.dll / .so / .dylib): directly validated and copied.
 *  - Package (.qlcvcw): future — ZIP with manifest + per-platform binaries.
 *
 * The install() method performs:
 *   1. Identify format (package vs. bare binary).
 *   2. For bare binary: test-load via QPluginLoader to check Qt ABI
 *      and verify it implements VCWidgetPluginInterface.
 *   3. If already loaded (same plugin ID): return AlreadyInstalled.
 *   4. Copy to userPluginDirectory.
 */
class VCWidgetPluginInstaller
{
public:
    enum Result
    {
        Ok,
        AlreadyInstalled,
        IncompatibleQt,
        InvalidPackage,
        CopyFailed
    };

    VCWidgetPluginInstaller();

    /**
     * Install a plugin from @p filePath.
     * Returns Ok on success, or an error code.
     * Call lastError() for a human-readable description of the error.
     */
    Result install(const QString& filePath);

    /** Human-readable description of the last error. */
    QString lastError() const;

private:
    Result installBinary(const QString& filePath);

    QString m_lastError;
};

#endif // VCWIDGETPLUGININSTALLER_H
