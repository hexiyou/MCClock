#include "ringtone_manager.h"

#include <QMediaPlayer>
#include <QAudioOutput>
#include <QTimer>
#include <QFile>
#include <QDir>
#include <QCoreApplication>
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
    stopVoice();
}

QString RingtoneManager::soundsDir() {
    // Deployment layout: sounds/ folder next to the executable
    const QDir deployed(QCoreApplication::applicationDirPath() + "/sounds");
    if (deployed.exists()) {
        return deployed.absolutePath();
    }
#ifdef MCCLOCK_SOUNDS_DIR
    // Development fallback: source tree resources/sounds
    const QDir devDir(QStringLiteral(MCCLOCK_SOUNDS_DIR));
    if (devDir.exists()) {
        return devDir.absolutePath();
    }
#endif
    return QString();
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
    const QString dir = soundsDir();
    if (dir.isEmpty()) return QString();
    return dir + QStringLiteral("/\u94c3\u58f0%1.mp3").arg(id); // 铃声N.mp3
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
    if (path.isEmpty() || !QFile::exists(path)) {
        // Sound file missing - nothing to play
        return;
    }

    audioOutput_->setVolume(qBound(0, volumePercent, 100) / 100.0f);
    player_->setSource(path.startsWith(":/") ? QUrl(path) : QUrl::fromLocalFile(path));

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

void RingtoneManager::speakTime(int hour, int minute, int volumePercent) {
    const QString dir = soundsDir();
    if (dir.isEmpty()) return;

    auto exists = [&dir](const QString& name) {
        return QFile::exists(dir + "/" + name) ? dir + "/" + name : QString();
    };

    // Build the announcement sequence:
    //   timenow -> am/pm/em -> hour(12h) -> point -> [minute digits -> MIN]
    QStringList queue;
    auto enqueue = [&](const QString& name) {
        const QString p = exists(name);
        if (!p.isEmpty()) queue << p;
    };

    enqueue("timenow.wav"); // 现在时间是
    if (hour >= 5 && hour <= 11) {
        enqueue("am.wav"); // 早上
    } else if (hour >= 12 && hour <= 17) {
        enqueue("pm.wav"); // 下午
    } else {
        enqueue("em.wav"); // 晚上
    }

    int h12 = hour % 12;
    if (h12 == 0) h12 = 12;
    // Use T2.wav for "两" when hour is 2
    enqueue(h12 == 2 ? QStringLiteral("T2.wav") : QString("%1.wav").arg(h12));
    enqueue("point.wav"); // 点

    if (minute > 0) {
        if (minute < 10) {
            // Add "零" prefix for single-digit minutes
            enqueue("0.wav");
            enqueue(QString("%1.wav").arg(minute));
        } else if (minute < 20) {
            enqueue("10.wav");
            if (minute > 10) enqueue(QString("%1.wav").arg(minute - 10));
        } else {
            enqueue(QString("%1.wav").arg(minute / 10 * 10)); // 20/30/40/50
            if (minute % 10 > 0) enqueue(QString("%1.wav").arg(minute % 10));
        }
        enqueue("MIN.wav"); // 分
    }

    if (queue.isEmpty()) return;
    stopVoice();
    voiceQueue_ = queue;
    voiceIndex_ = 0;
    voiceVolume_ = qBound(0, volumePercent, 100);
    playNextVoice();
}

void RingtoneManager::playNextVoice() {
    if (voiceIndex_ >= voiceQueue_.size()) {
        return; // sequence finished
    }
    if (!voicePlayer_) {
        voicePlayer_ = new QMediaPlayer(this);
        voiceAudio_ = new QAudioOutput(this);
        voicePlayer_->setAudioOutput(voiceAudio_);
        connect(voicePlayer_, &QMediaPlayer::mediaStatusChanged, this,
                [this](QMediaPlayer::MediaStatus status) {
            if (status == QMediaPlayer::EndOfMedia) {
                ++voiceIndex_;
                playNextVoice();
            }
        });
    }
    voiceAudio_->setVolume(voiceVolume_ / 100.0f);
    voicePlayer_->setSource(QUrl::fromLocalFile(voiceQueue_.at(voiceIndex_)));
    voicePlayer_->play();
}

void RingtoneManager::stopVoice() {
    voiceQueue_.clear();
    voiceIndex_ = 0;
    if (voicePlayer_) {
        voicePlayer_->stop();
    }
}

bool RingtoneManager::isPlaying() const {
    return playing_;
}

void RingtoneManager::onAutoStop() {
    stop();
}

} // namespace mcclock::services
