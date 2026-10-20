#include "questiondialog.h"

#include "artwork.h"
#include "audioplayer.h"

#include <QDialogButtonBox>
#include <QFileInfo>
#include <QFont>
#include <QFontMetrics>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <QRadioButton>
#include <QTimer>
#include <QVBoxLayout>

namespace {

const int kImageWidth = 400; // 4:3（看图题插画）
const int kImageHeight = 300;
const int kMinWidth = 640;

} // namespace

QuestionDialog::QuestionDialog(const Question &question, const QString &landName, AudioPlayer *audio,
                               QWidget *parent)
    : QDialog(parent)
    , m_question(question)
    , m_landName(landName)
    , m_audio(audio)
{
    setWindowTitle(QStringLiteral("晋剧文化考校"));
    setModal(true);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(30, 26, 30, 26);
    layout->setSpacing(14);

    // 标题行：类别徽章 + 地块名
    auto *header = new QLabel(this);
    header->setTextFormat(Qt::RichText);
    header->setText(QStringLiteral("<span style='background:#7E57C2;color:#FFFFFF;"
                                   "padding:3px 10px;border-radius:10px;'>%1</span>"
                                   "&nbsp;&nbsp;<span style='color:#E7D9B8;'>%2</span>")
                        .arg(QString::fromUtf8(quizCategoryName(question.category)),
                             QStringLiteral("晋剧文化地块 · %1").arg(landName)));
    QFont headerFont = header->font();
    headerFont.setPixelSize(15);
    header->setFont(headerFont);
    header->setWordWrap(true);
    layout->addWidget(header);

    auto *cue = new QLabel(QString::fromUtf8(quizCategoryCue(question.category)), this);
    cue->setWordWrap(true);
    cue->setStyleSheet(QStringLiteral("color:#B9A98C;"));
    layout->addWidget(cue);

    buildMediaArea(layout);

    auto *line = new QFrame(this);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    layout->addWidget(line);

    // 题干
    m_questionLabel = new QLabel(QString::fromStdString(question.text), this);
    m_questionLabel->setWordWrap(true);
    m_questionLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_questionLabel->setStyleSheet(QStringLiteral("color:#FFF7E6;"));
    QFont questionFont = m_questionLabel->font();
    questionFont.setPixelSize(20);
    questionFont.setBold(true);
    m_questionLabel->setFont(questionFont);
    layout->addWidget(m_questionLabel);

    // 四个选项
    static const char *kLetters[4] = {"A", "B", "C", "D"};
    QFont optionFont = font();
    optionFont.setPixelSize(17);
    for (int i = 0; i < 4; ++i) {
        const QString text =
            QStringLiteral("%1. %2")
                .arg(QString::fromLatin1(kLetters[i]),
                     QString::fromStdString(question.options[static_cast<size_t>(i)]));
        auto *button = new QRadioButton(text, this);
        button->setFont(optionFont);
        button->setStyleSheet(QStringLiteral(
            "QRadioButton{color:#F5EFE2;padding:11px 8px;}"
            "QRadioButton::indicator{width:16px;height:16px;}"));
        connect(button, &QRadioButton::toggled, this, &QuestionDialog::updateOkButton);
        m_options.append(button);
        layout->addWidget(button);
    }

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttons->button(QDialogButtonBox::Ok)->setText(QStringLiteral("提交答案"));
    buttons->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("放弃作答"));
    connect(buttons, &QDialogButtonBox::accepted, this, &QuestionDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QuestionDialog::reject);
    layout->addWidget(buttons);

    updateOkButton();
}

void QuestionDialog::buildMediaArea(QVBoxLayout *layout)
{
    const QuizCategory category = m_question.category;
    if (category == QuizCategory::Picture) {
        auto *image = new QLabel(this);
        image->setFixedSize(kImageWidth, kImageHeight);
        image->setAlignment(Qt::AlignCenter);
        image->setPixmap(Artwork::quizImage(QString::fromStdString(m_question.mediaKey),
                                            QSize(kImageWidth, kImageHeight)));
        image->setStyleSheet(QStringLiteral("border:2px solid #D4AF37; border-radius:6px;"));
        layout->addWidget(image, 0, Qt::AlignHCenter);
        return;
    }

    if (category == QuizCategory::Tune || category == QuizCategory::Aria) {
        const QString key = QString::fromStdString(m_question.mediaKey);
        const QString path = AudioPlayer::findAudio(key);
        m_hasAudio = !path.isEmpty();

        auto *row = new QWidget(this);
        auto *rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(0, 0, 0, 0);
        m_playButton = new QPushButton(m_hasAudio ? QStringLiteral("▶  重播音频")
                                                  : QStringLiteral("▶  播放音频"),
                                       row);
        m_playButton->setMinimumHeight(38);
        m_playButton->setEnabled(m_hasAudio);
        connect(m_playButton, &QPushButton::clicked, this, &QuestionDialog::onPlayAudio);
        rowLayout->addWidget(m_playButton);

        m_mediaState = new QLabel(row);
        m_mediaState->setWordWrap(true);
        if (path.isEmpty()) {
            // 注意：这里不能出现曲牌名/文件名，否则等于把答案写在题目上
            m_mediaState->setText(QStringLiteral("本题音频暂未配置，请依据题干线索作答。"));
            m_mediaState->setStyleSheet(QStringLiteral("color:#FFB74D;"));
        } else {
            // 只提示"可以听"，不显示文件名（文件名含答案拼音）
            m_mediaState->setText(QStringLiteral("音频已就绪，请仔细聆听后作答。"));
            m_mediaState->setStyleSheet(QStringLiteral("color:#8BC34A;"));
        }
        rowLayout->addWidget(m_mediaState, 1);
        layout->addWidget(row);
    }
}

int QuestionDialog::selectedAnswer() const
{
    const int count = static_cast<int>(m_options.size());
    for (int i = 0; i < count; ++i) {
        if (m_options.at(i)->isChecked())
            return i;
    }
    return -1;
}

void QuestionDialog::updateOkButton()
{
    if (auto *box = findChild<QDialogButtonBox *>()) {
        if (auto *ok = box->button(QDialogButtonBox::Ok))
            ok->setEnabled(selectedAnswer() >= 0);
    }
}

void QuestionDialog::onPlayAudio()
{
    if (m_audio)
        m_audio->playEffect(QString::fromStdString(m_question.mediaKey));
}

void QuestionDialog::fitTexts()
{
    // 让题干与选项按实际字号计算所需高度，避免文字被裁掉
    const int contentWidth = qMax(320, width() - 60);
    if (m_questionLabel) {
        const QFontMetrics fm(m_questionLabel->font());
        const QRect need = fm.boundingRect(QRect(0, 0, contentWidth, 10000),
                                           Qt::TextWordWrap | Qt::AlignLeft | Qt::AlignTop,
                                           m_questionLabel->text());
        m_questionLabel->setMinimumHeight(need.height() + 6);
    }
    for (QRadioButton *option : m_options) {
        const QFontMetrics fm(option->font());
        const QRect need = fm.boundingRect(QRect(0, 0, contentWidth - 40, 10000),
                                           Qt::TextWordWrap | Qt::AlignLeft | Qt::AlignVCenter,
                                           option->text());
        option->setMinimumHeight(qMax(40, need.height() + 20));
    }
    if (m_mediaState) {
        const QFontMetrics fm(m_mediaState->font());
        const QRect need = fm.boundingRect(QRect(0, 0, qMax(240, contentWidth - kImageWidth / 2),
                                                 10000),
                                           Qt::TextWordWrap | Qt::AlignLeft | Qt::AlignVCenter,
                                           m_mediaState->text());
        m_mediaState->setMinimumHeight(need.height() + 6);
    }

    // 让窗口足够宽，保证最长的选项一行放得下
    int needed = kMinWidth;
    for (QRadioButton *option : m_options) {
        const QFontMetrics fm(option->font());
        needed = qMax(needed, fm.horizontalAdvance(option->text()) + 120);
    }
    setMinimumWidth(needed);
}

void QuestionDialog::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
    fitTexts();
    adjustSize();

    // 唱段/曲牌题：弹窗显示后自动播放题干音频（背景配乐此时已被暂停）
    if (m_hasAudio && !m_autoPlayed) {
        m_autoPlayed = true;
        QTimer::singleShot(250, this, [this]() { onPlayAudio(); });
    }
}

void QuestionDialog::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter p(this);
    p.drawPixmap(rect(), Artwork::quizBackground(size()));
}

int QuestionDialog::ask(QWidget *parent, const Question &question, const QString &landName,
                        AudioPlayer *audio)
{
    QuestionDialog dialog(question, landName, audio, parent);
    if (dialog.exec() == QDialog::Accepted)
        return dialog.selectedAnswer();
    return -1; // 放弃作答，按答错处理
}
