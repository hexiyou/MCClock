#pragma once

#include <QString>
#include <QList>
#include <QJsonObject>
#include <QJsonArray>

namespace mcclock::utils {

// Data format design for the "check update" feature.
// The network layer is intentionally not implemented; only the data
// structures for request/response are defined so a future backend can
// plug in without protocol changes.
//
// Request:
//   GET {server}/api/update/v1/check?channel=stable&version=1.0.0&platform=win64
//
// Response: UpdateCheckResponse JSON (see below)

struct UpdateChannelInfo {
    QString channel;            // "stable" / "beta"
    QString latestVersion;      // e.g. "1.2.0"
    QString minSupportedVersion;
    bool forceUpdate = false;   // Mandatory upgrade below minSupportedVersion

    QJsonObject toJson() const {
        QJsonObject o;
        o["channel"] = channel;
        o["latest_version"] = latestVersion;
        o["min_supported_version"] = minSupportedVersion;
        o["force_update"] = forceUpdate;
        return o;
    }

    static UpdateChannelInfo fromJson(const QJsonObject& o) {
        UpdateChannelInfo c;
        c.channel = o["channel"].toString();
        c.latestVersion = o["latest_version"].toString();
        c.minSupportedVersion = o["min_supported_version"].toString();
        c.forceUpdate = o["force_update"].toBool();
        return c;
    }
};

struct UpdatePackage {
    QString version;
    QString url;                // Download URL of the installer / zip
    QString sha256;             // Checksum for verification
    qint64 sizeBytes = 0;
    QString changelog;          // Release notes (markdown)

    QJsonObject toJson() const {
        QJsonObject o;
        o["version"] = version;
        o["url"] = url;
        o["sha256"] = sha256;
        o["size_bytes"] = sizeBytes;
        o["changelog"] = changelog;
        return o;
    }

    static UpdatePackage fromJson(const QJsonObject& o) {
        UpdatePackage p;
        p.version = o["version"].toString();
        p.url = o["url"].toString();
        p.sha256 = o["sha256"].toString();
        p.sizeBytes = o["size_bytes"].toVariant().toLongLong();
        p.changelog = o["changelog"].toString();
        return p;
    }
};

struct UpdateCheckResponse {
    bool updateAvailable = false;
    UpdateChannelInfo channelInfo;
    UpdatePackage package;      // Valid when updateAvailable == true
    QString serverTime;

    QJsonObject toJson() const {
        QJsonObject o;
        o["update_available"] = updateAvailable;
        o["channel"] = channelInfo.toJson();
        o["package"] = package.toJson();
        o["server_time"] = serverTime;
        return o;
    }

    static UpdateCheckResponse fromJson(const QJsonObject& o) {
        UpdateCheckResponse r;
        r.updateAvailable = o["update_available"].toBool();
        r.channelInfo = UpdateChannelInfo::fromJson(o["channel"].toObject());
        r.package = UpdatePackage::fromJson(o["package"].toObject());
        r.serverTime = o["server_time"].toString();
        return r;
    }
};

// Compare dotted version strings: returns -1 / 0 / 1
inline int compareVersions(const QString& a, const QString& b) {
    const QStringList pa = a.split('.');
    const QStringList pb = b.split('.');
    const int n = qMax(pa.size(), pb.size());
    for (int i = 0; i < n; ++i) {
        int va = i < pa.size() ? pa.value(i).toInt() : 0;
        int vb = i < pb.size() ? pb.value(i).toInt() : 0;
        if (va != vb) return va < vb ? -1 : 1;
    }
    return 0;
}

} // namespace mcclock::utils
