#include "boardwidget.h"

#include "../model/question.h"
#include "artwork.h"

#include <QFont>
#include <QFontMetrics>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QToolTip>

namespace {

const int kGridSide = 7; // 7x7 网格，外环共 24 格
const int kGridLast = kGridSide - 1;

// 格号 -> 网格坐标：0 号在左下角，沿顺时针环绕
QPoint gridPosOf(int index)
{
    if (index < 0)
        return QPoint(0, kGridLast);
    if (index <= 6)
        return QPoint(index, kGridLast);
    if (index <= 12)
        return QPoint(kGridLast, kGridLast - (index - 6));
    if (index <= 18)
        return QPoint(kGridLast - (index - 12), 0);
    return QPoint(0, index - 18);
}

const QColor kNormalFill(247, 241, 228, 232);
const QColor kNormalBorder(186, 168, 132);
const QColor kNormalText(56, 46, 34);
const QColor kStartFill(255, 224, 138, 240);
const QColor kStartBorder(198, 148, 32);
const QColor kJinjuFill(236, 226, 250, 238);
const QColor kJinjuBorder(126, 87, 194);
const QColor kJinjuText(88, 48, 168);
const QColor kHighlight(217, 79, 61);

// 按可用空间自动缩放字号；实在放不下就做省略，保证不出现“字被遮住”
void drawFittedText(QPainter &p, const QRect &rect, const QString &text, int maxPixel,
                    int minPixel, const QColor &color, bool bold)
{
    if (text.isEmpty() || rect.width() <= 4 || rect.height() <= 4)
        return;
    for (int size = maxPixel; size >= minPixel; --size) {
        QFont f = p.font();
        f.setPixelSize(size);
        f.setBold(bold);
        const QFontMetrics fm(f);
        if (fm.horizontalAdvance(text) <= rect.width() - 2 && fm.height() <= rect.height()) {
            p.setFont(f);
            p.setPen(color);
            p.drawText(rect, Qt::AlignCenter, text);
            return;
        }
    }
    QFont f = p.font();
    f.setPixelSize(minPixel);
    f.setBold(bold);
    const QFontMetrics fm(f);
    p.setFont(f);
    p.setPen(color);
    p.drawText(rect, Qt::AlignCenter, fm.elidedText(text, Qt::ElideRight, rect.width() - 2));
}

} // namespace

BoardWidget::BoardWidget(QWidget *parent)
    : QWidget(parent)
{
    setMouseTracking(true);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMinimumSize(minimumSizeHint());
}

QSize BoardWidget::minimumSizeHint() const
{
    const int side = preferredCellSize() * kGridSide;
    return QSize(side, side);
}

void BoardWidget::setLands(const std::vector<Land> &lands)
{
    m_lands = lands;
    update();
}

void BoardWidget::setQuizCategories(const QList<int> &categories)
{
    m_categories = categories;
    update();
}

void BoardWidget::setEffectiveRents(const QList<int> &rents)
{
    m_rents = rents;
    update();
}

int BoardWidget::rentOf(int index) const
{
    if (index >= 0 && index < static_cast<int>(m_rents.size()))
        return m_rents.at(index);
    if (index >= 0 && index < static_cast<int>(m_lands.size()))
        return m_lands[index].rent();
    return 0;
}

void BoardWidget::setTokens(const QList<Token> &tokens)
{
    m_tokens = tokens;
    update();
}

void BoardWidget::setPlayerColors(const QList<QColor> &colors)
{
    m_playerColors = colors;
    update();
}

void BoardWidget::setPlayerNames(const QStringList &names)
{
    m_playerNames = names;
    update();
}

void BoardWidget::setCurrentCell(int cellIndex)
{
    if (m_currentCell == cellIndex)
        return;
    m_currentCell = cellIndex;
    update();
}

int BoardWidget::cellSize() const
{
    return qMax(40, qMin(width(), height()) / kGridSide);
}

QPoint BoardWidget::boardOrigin() const
{
    const int side = cellSize() * kGridSide;
    return QPoint((width() - side) / 2, (height() - side) / 2);
}

QRect BoardWidget::cellRect(int index) const
{
    const int cell = cellSize();
    const QPoint origin = boardOrigin();
    const QPoint grid = gridPosOf(index);
    return QRect(origin.x() + grid.x() * cell, origin.y() + grid.y() * cell, cell, cell)
        .adjusted(5, 5, -5, -5); // 格子之间留 10px 空隙，不再挤在一起
}

QRect BoardWidget::centerRect() const
{
    const int cell = cellSize();
    const QPoint origin = boardOrigin();
    return QRect(origin.x() + cell, origin.y() + cell, cell * 5, cell * 5);
}

BoardWidget::CenterLayout BoardWidget::centerLayout() const
{
    // 中央区域的排版：标题牌在上部，图例在其下方逐行排开，
    // 全部用绝对坐标与固定间距计算，避免出现“字叠在一起 / 串行”。
    CenterLayout out;
    const int cell = cellSize();
    const QRect center = centerRect();
    const int cw = center.width();
    const int ch = center.height();

    // 标题牌
    const int plateW = cw * 84 / 100;
    const int plateH = ch * 32 / 100;
    out.plate = QRect(center.x() + (cw - plateW) / 2, center.y() + ch * 5 / 100, plateW, plateH);

    // 标题与副标题：上下两段，中间留出固定间距
    const int pad = qMax(10, cell / 8);
    const int innerW = out.plate.width() - 2 * pad;
    const int gap = qMax(6, cell / 16);
    const int titleH = out.plate.height() * 52 / 100;
    out.title = QRect(out.plate.x() + pad, out.plate.y() + pad / 2, innerW, titleH);
    const int subtitleTop = out.title.bottom() + gap;
    out.subtitle = QRect(out.plate.x() + pad, subtitleTop, innerW,
                         qMax(14, out.plate.bottom() - gap / 2 - subtitleTop));

    // 图例：位于标题牌下方，行高与行距都由可用空间推导，保证每行不重叠
    const int rowH = qMax(20, ch * 9 / 100);
    const int rowGap = qMax(10, ch * 4 / 100);
    const int legendW = cw * 72 / 100;
    const int legendX = center.x() + (cw - legendW) / 2;
    int y = out.plate.bottom() + qMax(18, ch * 8 / 100);
    for (int i = 0; i < 3; ++i) {
        out.legendRows.append(QRect(legendX, y, legendW, rowH));
        y += rowH + rowGap;
    }
    return out;
}

void BoardWidget::drawCenterBlock(QPainter &painter) const
{
    const CenterLayout lay = centerLayout();
    const int cell = cellSize();

    // 标题牌
    painter.setPen(QPen(QColor(212, 175, 55, 200), 2));
    painter.setBrush(QColor(16, 12, 14, 185));
    painter.drawRoundedRect(lay.plate, 14, 14);

    drawFittedText(painter, lay.title, QStringLiteral("晋曲棋旅"), qMax(22, cell * 4 / 7), 14,
                   QColor(247, 214, 150), true);
    drawFittedText(painter, lay.subtitle,
                   QStringLiteral("晋剧文化主题大富翁 · 晋地十五景 + 晋剧八题"),
                   qMax(11, cell / 8), 9, QColor(226, 214, 190), false);

    // 三行图例：色块与文字在同一行内左右排布
    struct Legend
    {
        QColor fill;
        QColor border;
        QString text;
    };
    const QList<Legend> legend = {
        {kStartFill, kStartBorder, QStringLiteral("起点：经过可领 100 元补贴")},
        {kNormalFill, kNormalBorder, QStringLiteral("普通地块：买地收租，连片越多租越高")},
        {kJinjuFill, kJinjuBorder, QStringLiteral("晋剧地块：四类题目，答对 +100 / 答错 −150")},
    };

    for (int i = 0; i < lay.legendRows.size() && i < legend.size(); ++i) {
        const QRect row = lay.legendRows.at(i);
        const Legend &item = legend.at(i);
        const int box = qMin(row.height() - 6, qMax(14, cell / 5));
        const QRect boxRect(row.x(), row.y() + (row.height() - box) / 2, box, box);
        painter.setBrush(item.fill);
        painter.setPen(QPen(item.border, 2));
        painter.drawRoundedRect(boxRect, 4, 4);

        const int textLeft = boxRect.right() + qMax(8, cell / 12);
        const QRect textRect(textLeft, row.y(), qMax(10, row.right() - textLeft), row.height());
        drawFittedText(painter, textRect, item.text, qMax(11, cell / 8), 9,
                       QColor(232, 224, 206), false);
    }
}

QColor BoardWidget::ownerColor(int owner) const
{
    if (owner < 0 || owner >= static_cast<int>(m_playerColors.size()))
        return QColor(150, 150, 150);
    return m_playerColors.at(owner);
}

QString BoardWidget::categoryBadge(int index) const
{
    if (index < 0 || index >= static_cast<int>(m_categories.size()))
        return QString();
    const int c = m_categories.at(index);
    if (c < 0 || c >= kQuizCategoryCount)
        return QString();
    return QString::fromUtf8(quizCategoryShort(static_cast<QuizCategory>(c)));
}

int BoardWidget::hitTest(const QPoint &pos) const
{
    for (int i = 0; i < static_cast<int>(m_lands.size()); ++i) {
        if (cellRect(i).contains(pos))
            return i;
    }
    return -1;
}

QStringList BoardWidget::layoutReport() const
{
    QStringList out;
    const int cell = cellSize();
    out << QStringLiteral("BOARD size=%1x%2 cell=%3px 格子数=%4")
               .arg(width())
               .arg(height())
               .arg(cell)
               .arg(static_cast<int>(m_lands.size()));

    QFont probe = font();
    int smallest = 99;
    QString worst;
    for (int i = 0; i < static_cast<int>(m_lands.size()); ++i) {
        const QRect r = cellRect(i);
        const QRect nameRect(r.x() + 5, r.y() + r.height() * 7 / 100, r.width() - 10,
                             r.height() * 46 / 100);
        const QRect infoRect(r.x() + 5, r.y() + r.height() * 55 / 100, r.width() - 10,
                             r.height() * 34 / 100);
        const QString name = QString::fromStdString(m_lands[i].name());
        QString info;
        if (m_lands[i].isStart())
            info = QStringLiteral("START");
        else if (m_lands[i].isJinju())
            info = QStringLiteral("晋剧答题");
        else if (m_lands[i].isOwned())
            info = QStringLiteral("租 %1").arg(rentOf(i));
        else
            info = QStringLiteral("%1 / %2").arg(m_lands[i].price()).arg(m_lands[i].rent());

        for (const QPair<QString, QRect> &pair :
             QList<QPair<QString, QRect>>{{name, nameRect}, {info, infoRect}}) {
            int fit = 0;
            for (int size = qMax(16, cell / 5); size >= 6; --size) {
                probe.setPixelSize(size);
                probe.setBold(true);
                const QFontMetrics fm(probe);
                if (fm.horizontalAdvance(pair.first) <= pair.second.width() - 2
                    && fm.height() <= pair.second.height()) {
                    fit = size;
                    break;
                }
            }
            if (fit < smallest) {
                smallest = fit;
                worst = QStringLiteral("格%1「%2」").arg(i).arg(pair.first);
            }
        }
    }
    out << QStringLiteral("BOARD 最小可用字号=%1px（%2）%3")
               .arg(smallest)
               .arg(worst)
               .arg(smallest >= 10 ? QStringLiteral(" -> 文字不会被裁切")
                                   : QStringLiteral(" -> 警告：文字过小或需省略"));

    // 中央文字区排版检查：标题、副标题、三行图例之间不得重叠，图例必须在标题牌下方
    const CenterLayout lay = centerLayout();
    QList<QRect> blocks;
    blocks << lay.title << lay.subtitle;
    for (const QRect &row : lay.legendRows)
        blocks << row;
    QStringList overlaps;
    for (int i = 0; i < blocks.size(); ++i) {
        for (int j = i + 1; j < blocks.size(); ++j) {
            if (blocks.at(i).intersects(blocks.at(j)))
                overlaps << QStringLiteral("块%1×块%2").arg(i).arg(j);
        }
    }
    for (int i = 0; i < lay.legendRows.size(); ++i) {
        if (lay.legendRows.at(i).top() < lay.plate.bottom())
            overlaps << QStringLiteral("图例%1越界到标题牌").arg(i + 1);
    }
    out << QStringLiteral("CENTER 标题牌=%1x%2 标题=%3x%4 副标题=%5x%6 图例行高=%7 重叠=%8%9")
               .arg(lay.plate.width())
               .arg(lay.plate.height())
               .arg(lay.title.width())
               .arg(lay.title.height())
               .arg(lay.subtitle.width())
               .arg(lay.subtitle.height())
               .arg(lay.legendRows.isEmpty() ? 0 : lay.legendRows.first().height())
               .arg(overlaps.size())
               .arg(overlaps.isEmpty() ? QStringLiteral(" -> 文字互不重叠")
                                       : QStringLiteral(" -> ") + overlaps.join(QLatin1Char(',')));
    return out;
}

void BoardWidget::mouseMoveEvent(QMouseEvent *event)
{
    const int cell = hitTest(event->pos());
    if (cell != m_hoverCell) {
        m_hoverCell = cell;
        update();
    }
    if (cell < 0 || cell >= static_cast<int>(m_lands.size())) {
        QToolTip::hideText();
        return;
    }

    const Land &land = m_lands[cell];
    const QString name = QString::fromStdString(land.name());
    QString info = QStringLiteral("%1 号地块 · %2\n").arg(cell).arg(name);
    if (land.isStart()) {
        info += QStringLiteral("起点：经过可领取 200 元补贴");
    } else if (land.isJinju()) {
        const QString badge = categoryBadge(cell);
        info += QStringLiteral("晋剧文化地块（%1题）\n答对 +150 元，答错 −100 元").arg(badge);
    } else if (land.isOwned()) {
        const QString ownerName = (land.owner() < static_cast<int>(m_playerNames.size()))
                                      ? m_playerNames.at(land.owner())
                                      : QStringLiteral("玩家 %1").arg(land.owner() + 1);
        info += QStringLiteral("拥有者：%1\n当前租金：%2 元（含连片加成，基础 %3）").arg(ownerName).arg(rentOf(cell)).arg(land.rent());
    } else {
        info += QStringLiteral("售价：%1 元\n基础租金：%2 元").arg(land.price()).arg(land.rent());
    }
    QToolTip::showText(event->globalPosition().toPoint(), info, this);
}

void BoardWidget::leaveEvent(QEvent *event)
{
    Q_UNUSED(event)
    m_hoverCell = -1;
    QToolTip::hideText();
    update();
}

void BoardWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    // 晋剧 + 山西文化融合背景（代码绘制；放入 resources/backgrounds/main_bg.* 则用照片）
    if (m_background.size() != size() || m_background.isNull())
        m_background = Artwork::mainBackground(size());
    painter.drawPixmap(0, 0, m_background);

    const int cell = cellSize();

    // 中央文字区（标题 + 副标题 + 三行图例），由 centerLayout() 统一排布，保证互不重叠
    drawCenterBlock(painter);

    // 各格子
    for (int i = 0; i < static_cast<int>(m_lands.size()); ++i) {
        const Land &land = m_lands[i];
        const QRect r = cellRect(i);

        QColor fill = kNormalFill;
        QColor border = kNormalBorder;
        QColor textColor = kNormalText;
        if (land.isStart()) {
            fill = kStartFill;
            border = kStartBorder;
        } else if (land.isJinju()) {
            fill = kJinjuFill;
            border = kJinjuBorder;
            textColor = kJinjuText;
        } else if (land.isOwned()) {
            const QColor oc = ownerColor(land.owner());
            // 地主色染得更明显一些，远看也能分辨归属
            fill = QColor((oc.red() + 255 * 2) / 3, (oc.green() + 255 * 2) / 3,
                          (oc.blue() + 255 * 2) / 3, 242);
            border = oc;
        }

        if (i == m_currentCell)
            border = kHighlight;
        const int borderWidth = (i == m_currentCell) ? 4 : (land.isOwned() ? 3 : 2);
        if (i == m_hoverCell)
            fill = fill.lighter(106);

        painter.setBrush(fill);
        painter.setPen(QPen(border, borderWidth));
        painter.drawRoundedRect(r, 8, 8);

        // 地块名（自适应字号，绝不超出格子）
        const QRect nameRect(r.x() + 5, r.y() + r.height() * 7 / 100, r.width() - 10,
                             r.height() * 46 / 100);
        drawFittedText(painter, nameRect, QString::fromStdString(land.name()),
                       qMax(12, cell * 3 / 16), 9, textColor, true);

        // 价格 / 租金
        QString info;
        if (land.isStart())
            info = QStringLiteral("START");
        else if (land.isJinju())
            info = QStringLiteral("晋剧答题");
        else if (land.isOwned())
            info = QStringLiteral("租 %1").arg(rentOf(i));
        else
            info = QStringLiteral("%1 / %2").arg(land.price()).arg(land.rent());
        const QRect infoRect(r.x() + 5, r.y() + r.height() * 55 / 100, r.width() - 10,
                             r.height() * 34 / 100);
        drawFittedText(painter, infoRect, info, qMax(10, cell / 8), 8,
                       QColor(textColor.red(), textColor.green(), textColor.blue(), 210), false);

        // 晋剧地块的类别角标
        if (land.isJinju()) {
            const QString badge = categoryBadge(i);
            if (!badge.isEmpty()) {
                const QRect badgeRect(r.right() - r.width() * 46 / 100, r.y() - cell / 12,
                                      r.width() * 48 / 100 + 4, qMax(14, cell / 4));
                painter.setBrush(kJinjuBorder);
                painter.setPen(Qt::NoPen);
                painter.drawRoundedRect(badgeRect, 6, 6);
                drawFittedText(painter, badgeRect, badge, qMax(10, cell / 6), 8, Qt::white, true);
            }
        }

        // 已售地块：左上角加一个地主编号角标（颜色=该玩家棋子色），一眼看出是谁的地
        if (land.isOwned()) {
            const qreal b = qMax(18.0, cell * 0.30);
            const QRectF badge(r.x() - b * 0.22, r.y() - b * 0.22, b, b);
            painter.setBrush(ownerColor(land.owner()));
            painter.setPen(QPen(Qt::white, qMax(2.0, b * 0.12)));
            painter.drawEllipse(badge);
            drawFittedText(painter, badge.toRect(), QString::number(land.owner() + 1),
                           qMax(11, static_cast<int>(b * 0.62)), 9, Qt::white, true);
        }
    }

    // 棋子：加大、加白边与投影，尽量显眼；同一格 3~4 人时自动排成 2×2
    for (int i = 0; i < static_cast<int>(m_lands.size()); ++i) {
        QList<int> here;
        const int tokenCount = static_cast<int>(m_tokens.size());
        for (int t = 0; t < tokenCount; ++t) {
            if (m_tokens.at(t).cell == i)
                here.append(t);
        }
        if (here.isEmpty())
            continue;

        const QRect r = cellRect(i);
        const int n = static_cast<int>(here.size());
        const int radius = (n <= 2) ? qMax(15, cell / 5) : qMax(12, cell / 6);
        const int spacing = radius * 2 + qMax(4, cell / 25);

        QList<QPoint> centers;
        if (n <= 2) {
            const int y = r.bottom() - radius - 3;
            const int totalW = spacing * (n - 1);
            for (int k = 0; k < n; ++k)
                centers.append(QPoint(r.center().x() - totalW / 2 + k * spacing, y));
        } else {
            const int yBottom = r.bottom() - radius - 3;
            const int yTop = yBottom - spacing;
            centers.append(QPoint(r.center().x() - spacing / 2, yTop));
            centers.append(QPoint(r.center().x() + spacing / 2, yTop));
            centers.append(QPoint(r.center().x() - spacing / 2, yBottom));
            centers.append(QPoint(r.center().x() + spacing / 2, yBottom));
        }

        for (int k = 0; k < n; ++k) {
            const Token &token = m_tokens.at(here.at(k));
            const QPoint c = centers.at(k);

            // 投影：让棋子在浅色格子上也能跳出来
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(0, 0, 0, 95));
            painter.drawEllipse(QPointF(c.x() + 1.5, c.y() + 2.5), radius + 1.5, radius + 1.5);

            const QRectF tokenRect(c.x() - radius, c.y() - radius, radius * 2, radius * 2);
            Artwork::paintTokenFace(&painter, tokenRect, token.color);
            painter.setBrush(Qt::NoBrush);
            painter.setPen(QPen(QColor(255, 255, 255, 240), qMax(2.0, radius * 0.22)));
            painter.drawEllipse(tokenRect);
            drawFittedText(painter, tokenRect.toRect(), token.label,
                           qMax(12, static_cast<int>(radius * 1.1)), 9, Qt::white, true);
        }
    }
}
