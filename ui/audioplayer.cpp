#include "audioplayer.h"

#include "artwork.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QStringList>

#ifdef HAVE_QT_MULTIMEDIA
#include <QAudioOutput>
#include <QMediaPlayer>
#include <QUrl>
#endif

#if defined(Q_OS_WIN)
#include <windows.h>
#include <mmsystem.h>
#endif

namespace {

QStringList candidatePaths(const QString &key)
{
    const QStringList names = {
        key,                     // 允许直接给完整文件名
        key + ".mp3", key + ".wav", key + ".ogg", key + ".m4a", key + ".flac",
    };
    QStringList paths;
    QStringList roots;
    const QStringList resourceRoots = Artwork::resourceRoots();
    for (const QString &root : resourceRoots)
        roots << root + "audio/";
    for (const QString &root : roots) {
        for (const QString &name : names) {
            const QString full = root + name;
            if (QFileInfo::exists(full))
                paths << full;
        }
    }
    return paths;
}

} // namespace

AudioPlayer::AudioPlayer(QObject *parent)
    : QObject(parent)
{
#ifdef HAVE_QT_MULTIMEDIA
    m_player = new QMediaPlayer(this);
    m_output = new QAudioOutput(this);
    m_player->setAudioOutput(m_output);
    m_output->setVolume(m_volume / 100.0);
#endif
}

AudioPlayer::~AudioPlayer()
{
#if defined(Q_OS_WIN) && !defined(HAVE_QT_MULTIMEDIA)
    PlaySoundW(nullptr, nullptr, 0);
#endif
}

QString AudioPlayer::findAudio(const QString &key)
{
    const QStringList paths = candidatePaths(key);
    return paths.isEmpty() ? QString() : paths.first();
}

QString AudioPlayer::audioFolderHint()
{
    const QStringList roots = Artwork::resourceRoots();
    for (const QString &root : roots) {
        if (QFileInfo::exists(root + "audio"))
            return root + "audio/";
    }
    return QCoreApplication::applicationDirPath() + "/resources/audio/";
}

bool AudioPlayer::playBackground(const QString &key, int volumePercent)
{
    m_volume = qBound(0, volumePercent, 100);
    const QString path = findAudio(key);
    if (path.isEmpty()) {
        m_lastError = QString("未找到音频文件：%1（可放置 %2）")
                          .arg(key, audioFolderHint() + key + ".wav");
        m_playing = false;
        return false;
    }
    m_currentKey = key;

#ifdef HAVE_QT_MULTIMEDIA
    m_output->setVolume(m_volume / 100.0);
    m_player->setLoops(QMediaPlayer::Infinite);
    m_player->setSource(QUrl::fromLocalFile(path));
    m_player->play();
    m_playing = true;
    m_lastError.clear();
    return true;
#elif defined(Q_OS_WIN)
    // Win32 PlaySound 只支持 WAV，但无需额外 Qt 模块
    if (!path.endsWith(".wav", Qt::CaseInsensitive)) {
        m_lastError = QString("回退播放器只支持 WAV 格式，请把 %1 转成 WAV（或安装 Qt Multimedia 模块）")
                          .arg(QFileInfo(path).fileName());
        m_playing = false;
        return false;
    }
    const std::wstring wpath = QDir::toNativeSeparators(path).toStdWString();
    const BOOL ok = PlaySoundW(wpath.c_str(), nullptr, SND_FILENAME | SND_ASYNC | SND_LOOP);
    m_playing = (ok != FALSE);
    m_lastError = m_playing ? QString() : QString("系统播放器无法播放：%1").arg(path);
    return m_playing;
#else
    m_lastError = "当前平台没有可用的音频后端";
    return false;
#endif
}

void AudioPlayer::stopBackground()
{
#ifdef HAVE_QT_MULTIMEDIA
    if (m_player){
        m_player->stop();
    }
#elif defined(Q_OS_WIN)
    PlaySoundW(nullptr, nullptr, 0);
#endif
    m_playing = false;
    m_suspended = false;
}

bool AudioPlayer::isBackgroundPlaying() const
{
    return m_playing;
}

qint64 AudioPlayer::backgroundPositionMs() const
{
#ifdef HAVE_QT_MULTIMEDIA
    return m_player ? m_player->position() : 0;
#else
    return 0;
#endif
}

void AudioPlayer::setBackgroundVolume(int volumePercent)
{
    m_volume = qBound(0, volumePercent, 100);
#ifdef HAVE_QT_MULTIMEDIA
    if (m_output)
        m_output->setVolume(m_volume / 100.0);
#endif
}

void AudioPlayer::playEffect(const QString &key, int volumePercent)
{
    stopEffect(); // 同一时间只播放一段题目音频
    const QString path = findAudio(key);
    if (path.isEmpty())
        return;
#ifdef HAVE_QT_MULTIMEDIA
    // 独立播放器：与背景配乐互不干扰，且可单独停止
    m_effect = new QMediaPlayer(this);
    auto *output = new QAudioOutput(this);
    m_effect->setAudioOutput(output);
    output->setVolume(qBound(0, volumePercent, 100) / 100.0);
    m_effect->setSource(QUrl::fromLocalFile(path));
    connect(m_effect, &QMediaPlayer::mediaStatusChanged, this, [this](QMediaPlayer::MediaStatus st) {
        if ((st == QMediaPlayer::EndOfMedia || st == QMediaPlayer::InvalidMedia) && m_effect) {
            m_effect->deleteLater();
            m_effect = nullptr;
        }
    });
    m_effect->play();
#elif defined(Q_OS_WIN)
    Q_UNUSED(volumePercent)
    // Win32 PlaySound 是单通道：题目音频播放时配乐已被暂停（见 suspendBackground）
    if (path.endsWith(".wav", Qt::CaseInsensitive)) {
        const std::wstring wpath = QDir::toNativeSeparators(path).toStdWString();
        PlaySoundW(wpath.c_str(), nullptr, SND_FILENAME | SND_ASYNC);
    }
#else
    Q_UNUSED(volumePercent)
#endif
}

void AudioPlayer::stopEffect()
{
#ifdef HAVE_QT_MULTIMEDIA
    if (m_effect) {
        m_effect->stop();
        m_effect->deleteLater();
        m_effect = nullptr;
    }
#elif defined(Q_OS_WIN)
    // 单通道播放器：这里停掉后由 resumeBackground() 重新播出配乐
    PlaySoundW(nullptr, nullptr, 0);
#endif
}

void AudioPlayer::suspendBackground()
{
    if (!m_playing) {
        m_suspended = false;
        return;
    }
    m_suspended = true;
#ifdef HAVE_QT_MULTIMEDIA
    // pause() 会记住当前位置，之后 play() 即可从该处续播
    if (m_player)
        m_player->pause();
    m_playing = false;
#elif defined(Q_OS_WIN)
    // 系统播放器（PlaySound）不支持暂停/续播，只能停掉，恢复时会从头播放
    PlaySoundW(nullptr, nullptr, 0);
    m_playing = false;
#endif
}

void AudioPlayer::resumeBackground()
{
    if (!m_suspended)
        return;
    m_suspended = false;
#ifdef HAVE_QT_MULTIMEDIA
    // 关键：这里不能重新 setSource（那会让配乐从头开始），
    // 直接 play() 就从暂停的位置接着播。
    if (m_player) {
        m_player->play();
        m_playing = true;
        m_lastError.clear();
    }
#else
    // PlaySound 无法续播，只能重新播放（仅在没有 Qt Multimedia 时才会走到这里）
    playBackground(m_currentKey, m_volume);
#endif
}
