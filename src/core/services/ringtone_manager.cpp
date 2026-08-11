#include "ringtone_manager.h"

#include <QMediaPlayer>
#include <QAudioOutput>
#include <QTimer>
#include <QFile>
#include <QRandomGenerator>

namespace mcclock::services {

RingtoneManager::RingtoneManager(QObject* parent)
    : QObject(parent)
{
    player_ = new QMediaPlayer(this);
    audioOutput_ = new QAudioOutput(this);
    player_->setAudioOutput(audioOutput_);

    autoStopTimer_ = new QTimer(this);
    autoStopTimer_->setSingleShot(true);
    connect(autoStopTimer_, &QTimer::timeout, this, &RingtoneManager::onAutoStop);

    connect(player_, &QMediaPlayer::mediaStatusChanged, this,
            [this](QMediaPlayer::MediaStatus status) {
        if (status == QMediaPlayer::EndOfMedia && !autoStopTimer_->isActive()) {
            // Non-looping playback finished
            if (player_->loops() == 1) {
                playing_ = false;
                emit playbackStopped();
            }
        }
    });
}

RingtoneManager::~RingtoneManager() {
    stop();
}

QString RingtoneManager::resolveRingtonePath(int ringtoneId, const QString& customPath) {
    if (ringtoneId == 8) {
        return customPath;
    }
    int id = ringtoneId;
    if (id == 7) { // random builtin
        id = QRandomGenerator::global()->bounded(1, 7);
    }
    if (id < 1 || id > 6) id = 1;
    return QString(":/sounds/ringtone%1.mp3").arg(id);
}

QString RingtoneManager::builtinName(int ringtoneId) {
    switch (ringtoneId) {
    case 1: return QStringLiteral("\u7ecf\u5178\u94c3\u58f0");    // 经典铃声
    case 2: return QStringLiteral("\u6e05\u8106\u94c3\u58f0");    // 清脆铃声
    case 3: return QStringLiteral("\u6e29\u67d4\u97f3\u4e50");    // 温柔音乐
    case 4: return QStringLiteral("\u6d3b\u6cfc\u97f3\u4e50");    // 活泼音乐
    case 5: return QStringLiteral("\u7535\u5b50\u97f3\u6548");    // 电子音效
    case 6: return QStringLiteral("\u5e03\u8c37\u9e1f\u53eb");    // 布谷鸟叫
    case 7: return QStringLiteral("\u968f\u673a\u94c3\u58f0");    // 随机铃声
    case 8: return QStringLiteral("\u81ea\u5b9a\u4e49\u94c3\u58f0"); // 自定义铃声
    default: return QStringLiteral("\u672a\u77e5");
    }
}

int RingtoneManager::builtinCount() {
    return 8;
}

void RingtoneManager::play(int ringtoneId, const QString& customPath, int ringMode,
                           int customMinutes, int volumePercent) {
    stop();

    if (ringMode == 3) { // Silent
        return;
    }

    QString path = resolveRingtonePath(ringtoneId, customPath);
    if (path.startsWith(":/")) {
        if (!QFile::exists(path)) {
            // Resource not bundled yet - nothing to play
            return;
        }
    } else if (!QFile::exists(path)) {
        return;
    }

    audioOutput_->setVolume(qBound(0, volumePercent, 100) / 100.0f);
    player_->setSource(QUrl(path));

    switch (ringMode) {
    case 0: // AnnounceTime: brief ring (play 3 loops)
        player_->setLoops(3);
        break;
    case 1: // Continuous: loop until stop()
        player_->setLoops(QMediaPlayer::Infinite);
        break;
    case 2: // Once
        player_->setLoops(1);
        break;
    case 4: // Custom duration (minutes)
        player_->setLoops(QMediaPlayer::Infinite);
        if (customMinutes > 0) {
            autoStopTimer_->start(customMinutes * 60 * 1000);
        }
        break;
    default:
        player_->setLoops(1);
        break;
    }

    player_->play();
    playing_ = true;
}

void RingtoneManager::stop() {
    autoStopTimer_->stop();
    if (player_) {
        player_->stop();
    }
    if (playing_) {
        playing_ = false;
        emit playbackStopped();
    }
}

bool RingtoneManager::isPlaying() const {
    return playing_;
}

void RingtoneManager::onAutoStop() {
    stop();
}

} // namespace mcclock::services
