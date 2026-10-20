#ifndef AUDIOPLAYER_H
#define AUDIOPLAYER_H

#include <QObject>
#include <QString>

class QMediaPlayer;
class QAudioOutput;

// 音频播放：优先使用 Qt Multimedia；若模块不可用则在 Windows 上回退到 Win32 PlaySound。
// 音频文件需要用户自行放入 resources/audio/ 目录（见文件末尾的查找规则）。
class AudioPlayer : public QObject
{
    Q_OBJECT

public:
    explicit AudioPlayer(QObject *parent = nullptr);
    ~AudioPlayer() override;

    // 在 resources/audio 下查找 <key>.mp3 / .wav / .ogg / .m4a，返回完整路径（找不到为空）
    static QString findAudio(const QString &key);

    // 播放背景音乐（循环）；找不到文件返回 false，界面据此给出提示
    bool playBackground(const QString &key, int volumePercent = 55);
    void stopBackground();
    bool isBackgroundPlaying() const;
    void setBackgroundVolume(int volumePercent);
    qint64 backgroundPositionMs() const; // 当前配乐播放位置（毫秒），用于验证"续播"

    // 播放一次性音效（答题对错、骰子等）
    void playEffect(const QString &key, int volumePercent = 80);

    // 题目音频场景：答题时暂停背景配乐，只保留题目声音；答完再恢复配乐
    void suspendBackground(); // 暂停配乐并记住状态
    void resumeBackground();  // 恢复之前被暂停的配乐
    void stopEffect();        // 立即停止当前正在播放的题目音频

    // 最近一次错误信息（用于界面提示）
    QString lastError() const { return m_lastError; }

    // 音频未配置时给出放置说明
    static QString audioFolderHint();

private:
    QMediaPlayer *m_player = nullptr;
    QAudioOutput *m_output = nullptr;
    QMediaPlayer *m_effect = nullptr; // 题目音频（独立于配乐，便于单独停止）
    QString m_currentKey;
    QString m_lastError;
    int m_volume = 55;
    bool m_playing = false;
    bool m_suspended = false;
};

#endif // AUDIOPLAYER_H
