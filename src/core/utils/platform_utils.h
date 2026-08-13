#pragma once

#include <QString>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QSettings>
#include <QProcess>
#include <QCoreApplication>
#include <QDateTime>

#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

namespace mcclock::utils {

class PlatformUtils {
public:
    // Application start time file path
    static QString startTimeFilePath() {
        return appDataPath() + "/.start_time";
    }

    // Save start time to file (called by GUI at startup)
    static void saveStartTime() {
        QFile f(startTimeFilePath());
        if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
            f.write(QDateTime::currentDateTime().toString(Qt::ISODate).toUtf8());
            f.close();
        }
    }

    // Application start time - reads from shared file for cross-process consistency
    static QDateTime applicationStartTime() {
        QFile f(startTimeFilePath());
        if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QDateTime dt = QDateTime::fromString(f.readAll().trimmed(), Qt::ISODate);
            f.close();
            if (dt.isValid()) return dt;
        }
        // Fallback: no file found, use current time
        return QDateTime::currentDateTime();
    }

    // Get uptime string in English format like "2h 15m 30s"
    static QString uptimeString() {
        qint64 secs = applicationStartTime().secsTo(QDateTime::currentDateTime());
        if (secs < 0) secs = 0;
        int h = secs / 3600;
        int m = (secs % 3600) / 60;
        int s = secs % 60;
        QStringList parts;
        if (h > 0) parts << QStringLiteral("%1h").arg(h);
        if (m > 0) parts << QStringLiteral("%1m").arg(m);
        parts << QStringLiteral("%1s").arg(s);
        return parts.join(" ");
    }

    // Get application data directory: %APPDATA%/MCClock/
    static QString appDataPath() {
        QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        // AppDataLocation returns something like C:/Users/xxx/AppData/Roaming/MCClock
        // But we want to ensure it ends with MCClock
        QDir dir(path);
        if (!dir.exists()) {
            dir.mkpath(".");
        }
        return dir.absolutePath();
    }

    // Get database file path
    static QString databasePath() {
        return appDataPath() + "/mcclock.db";
    }

    // Get settings file path
    static QString settingsPath() {
        return appDataPath() + "/settings.json";
    }

    // Set auto-start on Windows boot (via registry)
    static bool setAutoStart(bool enable) {
#ifdef _WIN32
        HKEY hKey;
        LONG result = RegOpenKeyExW(HKEY_CURRENT_USER,
                                    L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                                    0, KEY_SET_VALUE, &hKey);
        if (result != ERROR_SUCCESS) {
            return false;
        }

        if (enable) {
            QString exePath = QCoreApplication::applicationFilePath();
            // When called from the CLI executable, auto-start should still
            // launch the GUI application
            if (exePath.contains("-CLI")) {
                QString guiPath = exePath;
                guiPath.replace("MCClock-CLI.exe", "MCClock.exe");
                if (QFile::exists(guiPath)) {
                    exePath = guiPath;
                }
            }
            QString value = "\"" + exePath + "\" --minimized";
            std::wstring wValue = value.toStdWString();
            result = RegSetValueExW(hKey, L"MCClock", 0, REG_SZ,
                                   reinterpret_cast<const BYTE*>(wValue.c_str()),
                                   static_cast<DWORD>((wValue.size() + 1) * sizeof(wchar_t)));
        } else {
            result = RegDeleteValueW(hKey, L"MCClock");
        }

        RegCloseKey(hKey);
        return (result == ERROR_SUCCESS);
#else
        Q_UNUSED(enable);
        return false;
#endif
    }

    // Check if auto-start is enabled
    static bool isAutoStartEnabled() {
#ifdef _WIN32
        HKEY hKey;
        LONG result = RegOpenKeyExW(HKEY_CURRENT_USER,
                                    L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                                    0, KEY_QUERY_VALUE, &hKey);
        if (result != ERROR_SUCCESS) {
            return false;
        }

        result = RegQueryValueExW(hKey, L"MCClock", nullptr, nullptr, nullptr, nullptr);
        RegCloseKey(hKey);
        return (result == ERROR_SUCCESS);
#else
        return false;
#endif
    }

    // Execute system shutdown/restart/logoff
    static bool executeShutdown(int option) {
#ifdef _WIN32
        // option: 0=force shutdown, 1=normal shutdown, 2=restart, 3=logoff
        QString cmd;
        switch (option) {
        case 0: // Force shutdown
            cmd = "shutdown /s /f /t 0";
            break;
        case 1: // Normal shutdown
            cmd = "shutdown /s /t 0";
            break;
        case 2: // Restart
            cmd = "shutdown /r /t 0";
            break;
        case 3: // Logoff
            cmd = "shutdown /l";
            break;
        default:
            return false;
        }

        // Enable shutdown privilege
        HANDLE hToken;
        TOKEN_PRIVILEGES tkp;
        if (OpenProcessToken(GetCurrentProcess(),
                             TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
            LookupPrivilegeValueW(nullptr, L"SeShutdownName",
                                  &tkp.Privileges[0].Luid);
            tkp.PrivilegeCount = 1;
            tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
            AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, nullptr, 0);
            CloseHandle(hToken);
        }

        return QProcess::startDetached("cmd.exe", {"/c", cmd});
#else
        Q_UNUSED(option);
        return false;
#endif
    }

    // Expand environment variables like %COMSPEC% or %USERPROFILE% in a string
    static QString expandEnvVars(const QString& input) {
#ifdef _WIN32
        if (!input.contains('%')) return input;
        const std::wstring wsrc = input.toStdWString();
        DWORD len = ExpandEnvironmentStringsW(wsrc.c_str(), nullptr, 0);
        if (len == 0) return input;
        std::wstring wbuf(len, L'\0');
        DWORD written = ExpandEnvironmentStringsW(wsrc.c_str(), wbuf.data(), len);
        if (written == 0 || written > len) return input;
        wbuf.resize(written - 1); // exclude trailing null
        return QString::fromStdWString(wbuf);
#else
        return input;
#endif
    }

    // Run a program or open a URL
    static bool runProgramOrUrl(const QString& path, const QString& args = "") {
        if (path.startsWith("http://") || path.startsWith("https://")) {
            return QProcess::startDetached("cmd.exe", {"/c", "start", path});
        }
        const QString realPath = expandEnvVars(path.trimmed());
        const QString realArgs = expandEnvVars(args);
        if (!realArgs.isEmpty()) {
            return QProcess::startDetached(realPath, realArgs.split(' ', Qt::SkipEmptyParts));
        }
        return QProcess::startDetached(realPath, {});
    }

    // Check if a program exists (environment variables are expanded first)
    static bool programExists(const QString& path) {
        return QFile::exists(expandEnvVars(path));
    }
};

} // namespace mcclock::utils
