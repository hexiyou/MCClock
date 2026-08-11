#pragma once

#include <QObject>
#include <QString>
#include <QStringList>

class QMediaPlayer;
class QAudioOutput;
class QTimer;

namespace mcclock::services {

// Manages alarm/ringtone playback.
// Ringtone ids: 1-6 builtin, 7 = random builtin, 8 = custom file.
// Ring modes (models::RingMode):
//   0 AnnounceTime - ring once briefly
//   1 Continuous   - loop until stop()
//   2 Once         - play once, no loop
//   3 Silent       - no sound
//   4 Custom       - loop for customMinutes then stop
class RingtoneManager : public QObject {
    Q_OBJECT
public:
    explicit RingtoneManager(QObject* parent = nullptr);
    ~RingtoneManager() override;

    // Directory holding the sound resource files (wav/mp3).
    // Resolved as <exe dir>/sounds (deployment) with a compile-time
    // fallback to the source resources/sounds directory for development.
    static QString soundsDir();

    // Resolve ringtone id + custom path to an actual file path
    static QString resolveRingtonePath(int ringtoneId, const QString& customPath);

    // Human-readable name of a builtin ringtone
    static QString builtinName(int ringtoneId);
    static int builtinCount();

    void play(int ringtoneId, const QString& customPath, int ringMode,
              int customMinutes, int volumePercent);
    void stop();
    bool isPlaying() const;

    // Voice announcement: "现在时间是 [早/下午/晚上] H 点 [M 分]"
    // minute < 0 announces the hour only. Plays sequentially from the
    // digit/point/MIN/am/pm/em/timenow wav files in soundsDir().
    void speakTime(int hour, int minute, int volumePercent);
    void stopVoice();

signals:
    void playbackStopped();

private slots:
    void onAutoStop();

private:
    void playNextVoice();

    QMediaPlayer* player_ = nullptr;
    QAudioOutput* audioOutput_ = nullptr;
    QTimer* autoStopTimer_ = nullptr;
    bool playing_ = false;

    // Sequential voice playback queue (separate player so it does not
    // interrupt the ringtone)
    QMediaPlayer* voicePlayer_ = nullptr;
    QAudioOutput* voiceAudio_ = nullptr;
    QStringList voiceQueue_;
    int voiceIndex_ = 0;
    int voiceVolume_ = 60;
};

} // namespace mcclock::services
