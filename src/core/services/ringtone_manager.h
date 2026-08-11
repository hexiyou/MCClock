#pragma once

#include <QObject>
#include <QString>

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

    // Resolve ringtone id + custom path to an actual file path
    static QString resolveRingtonePath(int ringtoneId, const QString& customPath);

    // Human-readable name of a builtin ringtone
    static QString builtinName(int ringtoneId);
    static int builtinCount();

    void play(int ringtoneId, const QString& customPath, int ringMode,
              int customMinutes, int volumePercent);
    void stop();
    bool isPlaying() const;

signals:
    void playbackStopped();

private slots:
    void onAutoStop();

private:
    QMediaPlayer* player_ = nullptr;
    QAudioOutput* audioOutput_ = nullptr;
    QTimer* autoStopTimer_ = nullptr;
    bool playing_ = false;
};

} // namespace mcclock::services
