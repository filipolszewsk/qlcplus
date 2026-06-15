/*
  vcplugindiagnostics.cpp
*/

#include "vcplugindiagnostics.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QHash>
#include <QMutex>
#include <QMutexLocker>
#include <QStandardPaths>
#include <QTextStream>

#include <atomic>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>

#if defined(Q_OS_UNIX)
#include <execinfo.h>
#include <fcntl.h>
#include <unistd.h>
#endif

namespace
{
constexpr int kRingLimit = 768;
constexpr int kEmergencyRingSlots = 128;
constexpr int kEmergencyLineBytes = 512;

struct DiagnosticsState
{
    QMutex mutex;
    bool installed = false;
    bool normalQuit = false;
    QString sessionId;
    QString logPath;
    QString crashPath;
    QString sessionPath;
    QStringList ring;
    QHash<QString, qint64> rateLimitLastMs;
    QtMessageHandler previousQtHandler = nullptr;
};

DiagnosticsState& state()
{
    static DiagnosticsState s;
    return s;
}

std::atomic<bool> g_emergencyInstalled { false };
std::atomic<unsigned int> g_emergencySeq { 0 };
char g_emergencyCrashPath[1024] = { 0 };
char g_emergencyRing[kEmergencyRingSlots][kEmergencyLineBytes] = {};

QString diagnosticsBaseDir()
{
    QString baseDir;
#if defined(Q_OS_MACOS) || defined(Q_OS_DARWIN) || defined(__APPLE__)
    const QString home = QDir::homePath();
    if (!home.isEmpty())
        baseDir = home + QStringLiteral("/Library/Logs/QLC+");
#endif
    if (baseDir.isEmpty())
    {
        baseDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
        if (baseDir.isEmpty())
            baseDir = QDir::homePath();
    }

    if (!baseDir.isEmpty())
    {
        QDir dir(baseDir);
        if (dir.exists() || dir.mkpath(QStringLiteral(".")))
            return dir.absolutePath();
    }

    return QDir::homePath();
}

QString fallbackLogPath(const QString& fileName)
{
    const QString baseDir = diagnosticsBaseDir();
    if (!baseDir.isEmpty())
        return QDir(baseDir).filePath(fileName);
    return QDir::home().filePath(QStringLiteral("QLC+_") + fileName);
}

QString timestamp()
{
    return QDateTime::currentDateTime().toString(Qt::ISODateWithMs);
}

QString processIdText()
{
    return QString::number(QCoreApplication::applicationPid());
}

void copyEmergencyPath(const QString& path)
{
    const QByteArray bytes = QFile::encodeName(path);
    const int len = qMin(bytes.size(), int(sizeof(g_emergencyCrashPath) - 1));
    if (len > 0)
        std::memcpy(g_emergencyCrashPath, bytes.constData(), size_t(len));
    g_emergencyCrashPath[len] = '\0';
}

void writeEmergencyBytes(const char* bytes, size_t len)
{
#if defined(Q_OS_UNIX)
    if (!g_emergencyCrashPath[0] || !bytes || len == 0)
        return;
    const int fd = ::open(g_emergencyCrashPath, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd < 0)
        return;
    while (len > 0)
    {
        const ssize_t written = ::write(fd, bytes, len);
        if (written <= 0)
            break;
        bytes += written;
        len -= size_t(written);
    }
    ::close(fd);
#else
    Q_UNUSED(bytes);
    Q_UNUSED(len);
#endif
}

void writeEmergencyText(const char* text)
{
    if (!text)
        return;
    writeEmergencyBytes(text, std::strlen(text));
}

void appendEmergencyRingLine(const QByteArray& line)
{
    const unsigned int seq = g_emergencySeq.fetch_add(1, std::memory_order_relaxed);
    char* slot = g_emergencyRing[seq % kEmergencyRingSlots];
    const int len = qMin(line.size(), kEmergencyLineBytes - 2);
    if (len > 0)
        std::memcpy(slot, line.constData(), size_t(len));
    slot[len] = '\n';
    slot[len + 1] = '\0';
}

void dumpEmergencyRing(const char* reason)
{
    writeEmergencyText("\n--- vcwidgets emergency dump ---\n");
    if (reason)
    {
        writeEmergencyText("reason=");
        writeEmergencyText(reason);
        writeEmergencyText("\n");
    }

    const unsigned int seq = g_emergencySeq.load(std::memory_order_relaxed);
    const unsigned int count = qMin(seq, unsigned(kEmergencyRingSlots));
    const unsigned int start = seq - count;
    for (unsigned int i = 0; i < count; ++i)
    {
        const char* line = g_emergencyRing[(start + i) % kEmergencyRingSlots];
        if (line[0])
            writeEmergencyText(line);
    }
#if defined(Q_OS_UNIX)
    writeEmergencyText("--- native backtrace ---\n");
    if (g_emergencyCrashPath[0])
    {
        const int fd = ::open(g_emergencyCrashPath, O_WRONLY | O_CREAT | O_APPEND, 0644);
        if (fd >= 0)
        {
            void* frames[64];
            const int frameCount = ::backtrace(frames, 64);
            ::backtrace_symbols_fd(frames, frameCount, fd);
            ::close(fd);
        }
    }
#endif
    writeEmergencyText("--- end vcwidgets emergency dump ---\n");
}

void signalHandler(int sig)
{
    char reason[64];
    std::snprintf(reason, sizeof(reason), "signal %d", sig);
    dumpEmergencyRing(reason);

    std::signal(sig, SIG_DFL);
#if defined(Q_OS_UNIX)
    ::raise(sig);
#else
    std::raise(sig);
#endif
}

void terminateHandler()
{
    dumpEmergencyRing("std::terminate");
    std::abort();
}

void appendLineUnlocked(const QString& path, const QString& line, bool immediateFlush)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
        return;

    QTextStream out(&file);
    out << line << Qt::endl;
    if (immediateFlush)
    {
        out.flush();
        file.flush();
    }
}

void rememberLineUnlocked(const QString& line)
{
    DiagnosticsState& s = state();
    s.ring.append(line);
    while (s.ring.size() > kRingLimit)
        s.ring.removeFirst();
    appendEmergencyRingLine(line.toUtf8());
}

void appendDiagnosticLine(const QString& line, bool immediateFlush)
{
    DiagnosticsState& s = state();
    QString path;
    {
        QMutexLocker locker(&s.mutex);
        path = s.logPath.isEmpty() ? VCPluginDiagnostics::logFilePath() : s.logPath;
        rememberLineUnlocked(line);
        appendLineUnlocked(path, line, immediateFlush);
    }
}

void dumpRingToCrashLog(const QString& reason)
{
    DiagnosticsState& s = state();
    QStringList ring;
    QString crashPath;
    {
        QMutexLocker locker(&s.mutex);
        crashPath = s.crashPath.isEmpty()
                ? VCPluginDiagnostics::crashLogFilePath() : s.crashPath;
        ring = s.ring;
    }

    QFile file(crashPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
        return;

    QTextStream out(&file);
    out << Qt::endl
        << "--- vcwidgets diagnostic dump " << timestamp()
        << " pid=" << processIdText()
        << " reason=\"" << reason << "\" ---" << Qt::endl;
    for (const QString& line : ring)
        out << line << Qt::endl;
    out << "--- end vcwidgets diagnostic dump ---" << Qt::endl;
    out.flush();
    file.flush();
}

void markSessionFile(const QString& text)
{
    DiagnosticsState& s = state();
    QString path;
    {
        QMutexLocker locker(&s.mutex);
        path = s.sessionPath;
    }
    if (path.isEmpty())
        return;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return;
    QTextStream out(&file);
    out << text << Qt::endl;
    out.flush();
    file.flush();
}

void normalQuit()
{
    DiagnosticsState& s = state();
    QString line;
    {
        QMutexLocker locker(&s.mutex);
        if (s.normalQuit)
            return;
        s.normalQuit = true;
        line = timestamp()
                + QStringLiteral(" plugin=vcdiagnostics widget=0 caption=\"VC Diagnostics\" session normal quit pid=")
                + processIdText()
                + QStringLiteral(" session=")
                + s.sessionId;
        rememberLineUnlocked(line);
        appendLineUnlocked(s.logPath, line, true);
    }

    markSessionFile(QStringLiteral("normal quit pid=%1 session=%2 time=%3")
                            .arg(processIdText(), state().sessionId, timestamp()));
}

void qtMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& message)
{
    if (type == QtDebugMsg || type == QtInfoMsg)
    {
        QtMessageHandler previous = nullptr;
        {
            QMutexLocker locker(&state().mutex);
            previous = state().previousQtHandler;
        }
        if (previous)
            previous(type, context, message);
        return;
    }

    const char* typeName = "debug";
    switch (type)
    {
        case QtDebugMsg: typeName = "debug"; break;
        case QtInfoMsg: typeName = "info"; break;
        case QtWarningMsg: typeName = "warning"; break;
        case QtCriticalMsg: typeName = "critical"; break;
        case QtFatalMsg: typeName = "fatal"; break;
    }

    const QString line = QStringLiteral("%1 plugin=vcdiagnostics widget=0 caption=\"VC Diagnostics\" qt %2 file=\"%3\" line=%4 msg=\"%5\"")
            .arg(timestamp(), QString::fromLatin1(typeName),
                 QString::fromLatin1(context.file ? context.file : ""),
                 QString::number(context.line),
                 message);
    appendDiagnosticLine(line, type == QtFatalMsg || type == QtCriticalMsg);

    QtMessageHandler previous = nullptr;
    {
        QMutexLocker locker(&state().mutex);
        previous = state().previousQtHandler;
    }
    if (previous)
        previous(type, context, message);
    else
        std::fprintf(stderr, "%s\n", line.toLocal8Bit().constData());

    if (type == QtFatalMsg)
    {
        dumpRingToCrashLog(QStringLiteral("Qt fatal message"));
        std::abort();
    }
}

void installEmergencyHandlers()
{
    if (g_emergencyInstalled.exchange(true, std::memory_order_acq_rel))
        return;

    std::set_terminate(terminateHandler);
    std::signal(SIGSEGV, signalHandler);
    std::signal(SIGABRT, signalHandler);
    std::signal(SIGBUS, signalHandler);
    std::signal(SIGILL, signalHandler);
    std::signal(SIGFPE, signalHandler);
    std::signal(SIGTERM, signalHandler);
}
}

QString VCPluginDiagnostics::logFilePath()
{
    return fallbackLogPath(QStringLiteral("vcwidgets.log"));
}

QString VCPluginDiagnostics::crashLogFilePath()
{
    return fallbackLogPath(QStringLiteral("vcwidgets-crash.log"));
}

void VCPluginDiagnostics::install(const QString& pluginId, quint32 widgetId,
                                  const QString& caption)
{
    DiagnosticsState& s = state();
    bool firstInstall = false;
    QString previousSessionText;
    QString sessionPath;
    {
        QMutexLocker locker(&s.mutex);
        if (!s.installed)
        {
            s.installed = true;
            firstInstall = true;
            s.logPath = logFilePath();
            s.crashPath = crashLogFilePath();
            s.sessionPath = fallbackLogPath(QStringLiteral("vcwidgets.session"));
            s.sessionId = QDateTime::currentDateTimeUtc().toString(QStringLiteral("yyyyMMddTHHmmsszzz"))
                    + QStringLiteral("-")
                    + processIdText();
            s.previousQtHandler = qInstallMessageHandler(qtMessageHandler);
            sessionPath = s.sessionPath;
            copyEmergencyPath(s.crashPath);

            QFile previous(sessionPath);
            if (previous.open(QIODevice::ReadOnly | QIODevice::Text))
                previousSessionText = QString::fromUtf8(previous.readAll()).trimmed();
        }
    }

    if (!firstInstall)
        return;

    installEmergencyHandlers();

    if (QCoreApplication::instance())
    {
        QObject::connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit,
                         QCoreApplication::instance(), []() { normalQuit(); });
    }

    const bool previousBelongsToThisProcess = previousSessionText.contains(
            QStringLiteral("running pid=%1").arg(processIdText()));
    if (!previousSessionText.isEmpty()
            && !previousSessionText.contains(QStringLiteral("normal quit"))
            && !previousBelongsToThisProcess)
    {
        const QString line = QStringLiteral("%1 plugin=vcdiagnostics widget=0 caption=\"VC Diagnostics\" previous session ended unexpectedly previous=\"%2\"")
                .arg(timestamp(), previousSessionText);
        appendDiagnosticLine(line, true);
        dumpRingToCrashLog(QStringLiteral("previous session ended unexpectedly"));
    }

    const QString startLine = QStringLiteral("%1 plugin=%2 widget=%3 caption=\"%4\" session start pid=%5 session=%6 log=\"%7\" crashLog=\"%8\"")
            .arg(timestamp(), pluginId, QString::number(widgetId), caption,
                 processIdText(), state().sessionId, logFilePath(), crashLogFilePath());
    appendDiagnosticLine(startLine, true);
    markSessionFile(QStringLiteral("running pid=%1 session=%2 time=%3 plugin=%4 widget=%5 caption=\"%6\"")
                            .arg(processIdText(), state().sessionId, timestamp(),
                                 pluginId, QString::number(widgetId), caption));
}

void VCPluginDiagnostics::breadcrumb(const QString& pluginId, quint32 widgetId,
                                     const QString& caption, const QString& message,
                                     bool immediateFlush)
{
    install(pluginId, widgetId, caption);

    const QString line = QStringLiteral("%1 plugin=%2 widget=%3 caption=\"%4\" %5")
            .arg(timestamp(), pluginId, QString::number(widgetId), caption, message);
    appendDiagnosticLine(line, immediateFlush);
}

void VCPluginDiagnostics::breadcrumbRateLimited(const QString& pluginId, quint32 widgetId,
                                                const QString& caption, const QString& key,
                                                int intervalMs, const QString& message,
                                                bool immediateFlush)
{
    install(pluginId, widgetId, caption);

    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    {
        QMutexLocker locker(&state().mutex);
        const qint64 last = state().rateLimitLastMs.value(key, 0);
        if (last > 0 && now - last < intervalMs)
        {
            const QString skipped = QStringLiteral("%1 plugin=%2 widget=%3 caption=\"%4\" %5")
                    .arg(timestamp(), pluginId, QString::number(widgetId), caption, message);
            rememberLineUnlocked(skipped);
            return;
        }
        state().rateLimitLastMs.insert(key, now);
    }

    breadcrumb(pluginId, widgetId, caption, message, immediateFlush);
}
