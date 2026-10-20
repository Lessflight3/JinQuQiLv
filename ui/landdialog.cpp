#include "landdialog.h"

#include "artwork.h"

#include <QDialogButtonBox>
#include <QFont>
#include <QFrame>
#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <QVBoxLayout>

namespace {

const int kImageWidth = 400; // 4:3（地域风貌图）
const int kImageHeight = 300;

} // namespace

LandDialog::LandDialog(const QString &title, const QString &subtitle, const QString &landKey,
                       const QStringList &infoLines, const QString &acceptText, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(subtitle.isEmpty() ? title : subtitle);
    setModal(true);
    setMinimumWidth(500);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(26, 22, 26, 22);
    layout->setSpacing(16);

    auto *titleLabel = new QLabel(title, this);
    QFont titleFont = titleLabel->font();
    titleFont.setPixelSize(24);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setStyleSheet(QStringLiteral("color:#F0D98C;"));
    titleLabel->setWordWrap(true);
    titleLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(titleLabel);

    if (!subtitle.isEmpty()) {
        auto *subLabel = new QLabel(subtitle, this);
        subLabel->setStyleSheet(QStringLiteral("color:#D8C9A6;"));
        subLabel->setAlignment(Qt::AlignCenter);
        subLabel->setWordWrap(true);
        layout->addWidget(subLabel);
    }

    // 地域风貌图（优先使用 resources/places/<key>.jpg，否则由代码绘制）
    auto *image = new QLabel(this);
    image->setFixedSize(kImageWidth, kImageHeight);
    image->setAlignment(Qt::AlignCenter);
    image->setPixmap(Artwork::placeImage(landKey, title, QSize(kImageWidth, kImageHeight)));
    image->setStyleSheet(QStringLiteral("border:2px solid #D4AF37; border-radius:6px;"));
    layout->addWidget(image, 0, Qt::AlignHCenter);

    // 价格与状态信息
    auto *line = new QFrame(this);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    layout->addWidget(line);

    auto *info = new QLabel(this);
    info->setTextFormat(Qt::RichText);
    info->setWordWrap(true);
    info->setText(infoLines.join(QStringLiteral("<br>")));
    QFont infoFont = info->font();
    infoFont.setPixelSize(16);
    info->setFont(infoFont);
    info->setStyleSheet(QStringLiteral("color:#F2EDE2;"));
    layout->addWidget(info);

    auto *buttons = new QDialogButtonBox(this);
    if (acceptText.isEmpty()) {
        auto *ok = buttons->addButton(QStringLiteral("知道啦"), QDialogButtonBox::AcceptRole);
        connect(ok, &QPushButton::clicked, this, &QDialog::accept);
    } else {
        auto *ok = buttons->addButton(acceptText, QDialogButtonBox::AcceptRole);
        auto *cancel = buttons->addButton(QStringLiteral("再看看"), QDialogButtonBox::RejectRole);
        connect(ok, &QPushButton::clicked, this, &QDialog::accept);
        connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
    }
    layout->addWidget(buttons);
}

void LandDialog::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter p(this);
    p.drawPixmap(rect(), Artwork::quizBackground(size()));
}

bool LandDialog::askBuy(QWidget *parent, const QString &landName, const QString &landKey, int price,
                        int rent, int playerMoney, const QString &playerName)
{
    const QStringList info = {
        QStringLiteral("购买价格：<b>%1</b> 元").arg(price),
        QStringLiteral("每次租金：<b>%1</b> 元").arg(rent),
        QStringLiteral("%1 当前现金：<b>%2</b> 元").arg(playerName).arg(playerMoney),
        QStringLiteral("购买后余额：<b>%1</b> 元").arg(playerMoney - price),
    };
    LandDialog dialog(landName, QStringLiteral("待购买地块 · 点击下方按钮确认"), landKey, info,
                      QStringLiteral("买下它"), parent);
    return dialog.exec() == QDialog::Accepted;
}

void LandDialog::showInfo(QWidget *parent, const QString &title, const QString &subtitle,
                          const QString &landKey, const QStringList &infoLines)
{
    LandDialog dialog(title, subtitle, landKey, infoLines, QString(), parent);
    dialog.exec();
}
