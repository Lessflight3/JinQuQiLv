#ifndef QUESTIONDIALOG_H
#define QUESTIONDIALOG_H

#include "../model/question.h"

#include <QDialog>
#include <QList>
#include <QString>

class AudioPlayer;
class QLabel;
class QPushButton;
class QRadioButton;

// 晋剧答题弹窗：晋剧风格背景 + 类别徽章 + 音频/图片 + 题干与四个选项
class QuestionDialog : public QDialog
{
    Q_OBJECT

public:
    QuestionDialog(const Question &question, const QString &landName, AudioPlayer *audio,
                   QWidget *parent = nullptr);

    int selectedAnswer() const; // 未选择返回 -1

    // 弹出答题窗口：返回所选下标；点“放弃作答”或关闭窗口返回 -1
    static int ask(QWidget *parent, const Question &question, const QString &landName,
                   AudioPlayer *audio);

protected:
    void paintEvent(QPaintEvent *event) override;
    void showEvent(QShowEvent *event) override;

private slots:
    void updateOkButton();
    void onPlayAudio();

private:
    void buildMediaArea(class QVBoxLayout *layout);
    void fitTexts(); // 根据实际字号计算文字所需高度，避免文字被裁掉

    Question m_question;
    QString m_landName;
    AudioPlayer *m_audio = nullptr;
    QList<QRadioButton *> m_options;
    QLabel *m_questionLabel = nullptr;
    QLabel *m_mediaState = nullptr;
    QPushButton *m_playButton = nullptr;
    bool m_hasAudio = false;   // 该题是否有可用音频
    bool m_autoPlayed = false; // 是否已自动播放过
};

#endif // QUESTIONDIALOG_H
