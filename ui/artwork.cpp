#include "artwork.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QFont>
#include <QFontMetrics>
#include <QHash>
#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QRadialGradient>
#include <QVector>

#include <cmath>

namespace {

// ---------------------------------------------------------------- 基础工具

QHash<QString, QPixmap> g_cache;

const QColor kGold(212, 175, 55);
const QColor kGoldLight(240, 217, 140);
const QColor kInk(18, 14, 12);
const QColor kCurtainDark(58, 12, 12);
const QColor kCurtainMid(110, 27, 27);
const QColor kBrick(78, 72, 66);
const QColor kJade(46, 94, 91);
constexpr double kPi = 3.14159265358979323846;

QString appDir()
{
    return QCoreApplication::applicationDirPath();
}

QStringList buildResourceRoots()
{
    QStringList roots;
    QDir dir(appDir());
    for (int level = 0; level < 5; ++level) {
        roots << dir.absolutePath() + "/resources/";
        if (!dir.cdUp())
            break;
    }
    roots << QDir::currentPath() + "/resources/";

    // 兼容“构建目录与工程目录并列”的情况：在各级上级目录中查找含 JinQuQiLv.pro 的工程目录
    QDir probe(appDir());
    for (int level = 0; level < 4; ++level) {
        if (!probe.cdUp())
            break;
        const QFileInfoList subs =
            probe.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
        int scanned = 0;
        bool found = false;
        for (const QFileInfo &sub : subs) {
            if (++scanned > 40)
                break;
            if (QFileInfo::exists(sub.absoluteFilePath() + "/JinQuQiLv.pro")) {
                roots << sub.absoluteFilePath() + "/resources/";
                found = true;
                break;
            }
        }
        if (found)
            break;
    }

    roots.removeDuplicates();
    return roots;
}

QString findFile(const QStringList &relativeNames)
{
    const QStringList roots = buildResourceRoots();
    for (const QString &root : roots) {
        for (const QString &rel : relativeNames) {
            const QString full = root + rel;
            if (QFileInfo::exists(full))
                return full;
        }
    }
    return QString();
}

QStringList withExtensions(const QString &base)
{
    QStringList list;
    const QStringList exts = {"png", "jpg", "jpeg", "webp", "bmp"};
    for (const QString &e : exts)
        list << QString("%1.%2").arg(base, e);
    return list;
}

// 按 rect 宽度自动缩放字号绘制文字；仍放不下时做省略处理
void drawTextFit(QPainter &p, const QRect &rect, const QString &text, int maxSize,
                 const QColor &color, bool bold = false, int minSize = 9)
{
    if (text.isEmpty() || rect.width() <= 2)
        return;
    for (int size = maxSize; size >= minSize; --size) {
        QFont f = p.font();
        f.setPixelSize(size);
        f.setBold(bold);
        const QFontMetrics fm(f);
        if (fm.horizontalAdvance(text) <= rect.width() && fm.height() <= rect.height()) {
            p.setFont(f);
            p.setPen(color);
            p.drawText(rect, Qt::AlignCenter, text);
            return;
        }
    }
    QFont f = p.font();
    f.setPixelSize(minSize);
    f.setBold(bold);
    const QFontMetrics fm(f);
    p.setFont(f);
    p.setPen(color);
    p.drawText(rect, Qt::AlignCenter, fm.elidedText(text, Qt::ElideRight, rect.width()));
}

// 云纹（祥云）：一圈卷曲的弧形
void drawCloud(QPainter &p, const QPointF &center, double s, const QColor &color)
{
    QPen pen(color, qMax(1.0, s * 0.10));
    pen.setCapStyle(Qt::RoundCap);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    QPainterPath path;
    path.moveTo(center.x() - s, center.y());
    path.cubicTo(center.x() - s * 0.7, center.y() - s * 0.7,
                 center.x() - s * 0.1, center.y() - s * 0.7,
                 center.x() - s * 0.05, center.y() - s * 0.1);
    path.cubicTo(center.x(), center.y() - s * 0.75,
                 center.x() + s * 0.7, center.y() - s * 0.6,
                 center.x() + s * 0.6, center.y());
    path.cubicTo(center.x() + s * 1.0, center.y() + s * 0.05,
                 center.x() + s * 0.9, center.y() + s * 0.5,
                 center.x() + s * 0.35, center.y() + s * 0.45);
    p.drawPath(path);
    p.drawArc(QRectF(center.x() - s * 0.55, center.y() - s * 0.5, s * 0.7, s * 0.7), 30 * 16,
              300 * 16);
}

// 山峦剪影（可叠多层）
void drawMountains(QPainter &p, const QRectF &area, const QColor &color, double phase,
                   int peaks)
{
    QPainterPath path;
    path.moveTo(area.left(), area.bottom());
    const double step = area.width() / peaks;
    for (int i = 0; i <= peaks; ++i) {
        const double x = area.left() + i * step;
        const double h = (i % 2 == 0) ? area.height() * 0.75 : area.height() * 0.45;
        const double wave = std::sin(phase + i * 1.3) * area.height() * 0.12;
        path.lineTo(x - step / 2, area.bottom() - h * 0.6 - wave);
        path.lineTo(x, area.bottom() - h - wave);
        path.lineTo(x + step / 2, area.bottom() - h * 0.5 - wave);
    }
    path.lineTo(area.right(), area.bottom());
    path.closeSubpath();
    p.setPen(Qt::NoPen);
    p.setBrush(color);
    p.drawPath(path);
}

// 古建屋顶（歇山顶）
void drawRoof(QPainter &p, const QRectF &r, const QColor &color)
{
    QPainterPath roof;
    roof.moveTo(r.left(), r.bottom());
    roof.lineTo(r.left() + r.width() * 0.12, r.top());
    roof.lineTo(r.right() - r.width() * 0.12, r.top());
    roof.lineTo(r.right(), r.bottom());
    roof.closeSubpath();
    p.setPen(Qt::NoPen);
    p.setBrush(color);
    p.drawPath(roof);
    // 翘起的檐角
    QPainterPath eave;
    eave.moveTo(r.left(), r.bottom());
    eave.quadTo(r.left() - r.width() * 0.06, r.bottom() - r.height() * 0.05,
                r.left() + r.width() * 0.10, r.bottom() - r.height() * 0.12);
    eave.lineTo(r.left() + r.width() * 0.16, r.bottom() - r.height() * 0.02);
    eave.closeSubpath();
    p.drawPath(eave);
    QPainterPath eave2;
    eave2.moveTo(r.right(), r.bottom());
    eave2.quadTo(r.right() + r.width() * 0.06, r.bottom() - r.height() * 0.05,
                 r.right() - r.width() * 0.10, r.bottom() - r.height() * 0.12);
    eave2.lineTo(r.right() - r.width() * 0.16, r.bottom() - r.height() * 0.02);
    eave2.closeSubpath();
    p.drawPath(eave2);
}

// 楼阁剪影（多层屋檐）
void drawPagoda(QPainter &p, const QRectF &base, const QColor &color, int tiers)
{
    const double tierH = base.height() / tiers;
    const double bodyW = base.width() * 0.62;
    for (int i = 0; i < tiers; ++i) {
        const double y = base.bottom() - (i + 1) * tierH;
        const double shrink = 1.0 - i * 0.12;
        const QRectF roof(base.center().x() - bodyW * shrink * 0.75, y + tierH * 0.45,
                          bodyW * shrink * 1.5, tierH * 0.5);
        drawRoof(p, roof, color);
        p.setBrush(color.darker(115));
        p.drawRect(QRectF(base.center().x() - bodyW * shrink * 0.35, y + tierH * 0.95,
                          bodyW * shrink * 0.7, tierH * 0.5));
    }
}

// 戏曲脸谱（对称图案）
void drawMask(QPainter &p, const QRectF &r, const QColor &base, const QColor &accent,
              const QColor &pattern)
{
    QPainterPath face;
    face.moveTo(r.center().x(), r.top());
    face.cubicTo(r.right() - r.width() * 0.05, r.top() + r.height() * 0.10,
                 r.right(), r.top() + r.height() * 0.45,
                 r.right() - r.width() * 0.06, r.top() + r.height() * 0.72);
    face.cubicTo(r.right() - r.width() * 0.14, r.bottom() - r.height() * 0.02,
                 r.center().x() + r.width() * 0.12, r.bottom(),
                 r.center().x(), r.bottom());
    face.cubicTo(r.center().x() - r.width() * 0.12, r.bottom(),
                 r.left() + r.width() * 0.14, r.bottom() - r.height() * 0.02,
                 r.left() + r.width() * 0.06, r.top() + r.height() * 0.72);
    face.cubicTo(r.left(), r.top() + r.height() * 0.45,
                 r.left() + r.width() * 0.05, r.top() + r.height() * 0.10,
                 r.center().x(), r.top());
    face.closeSubpath();

    p.setPen(QPen(pattern, qMax(1.0, r.width() * 0.02)));
    p.setBrush(base);
    p.drawPath(face);

    // 眉与眼窝
    p.setBrush(accent);
    p.setPen(Qt::NoPen);
    const double eyeW = r.width() * 0.26;
    const double eyeH = r.height() * 0.13;
    QPainterPath eyeL;
    eyeL.moveTo(r.center().x() - r.width() * 0.07, r.top() + r.height() * 0.42);
    eyeL.cubicTo(r.center().x() - r.width() * 0.30, r.top() + r.height() * 0.30,
                 r.center().x() - r.width() * 0.42, r.top() + r.height() * 0.44,
                 r.center().x() - r.width() * 0.36, r.top() + r.height() * 0.52);
    eyeL.cubicTo(r.center().x() - r.width() * 0.28, r.top() + r.height() * 0.50,
                 r.center().x() - r.width() * 0.16, r.top() + r.height() * 0.50,
                 r.center().x() - r.width() * 0.07, r.top() + r.height() * 0.42);
    p.drawPath(eyeL);
    QPainterPath eyeR;
    eyeR.moveTo(r.center().x() + r.width() * 0.07, r.top() + r.height() * 0.42);
    eyeR.cubicTo(r.center().x() + r.width() * 0.30, r.top() + r.height() * 0.30,
                 r.center().x() + r.width() * 0.42, r.top() + r.height() * 0.44,
                 r.center().x() + r.width() * 0.36, r.top() + r.height() * 0.52);
    eyeR.cubicTo(r.center().x() + r.width() * 0.28, r.top() + r.height() * 0.50,
                 r.center().x() + r.width() * 0.16, r.top() + r.height() * 0.50,
                 r.center().x() + r.width() * 0.07, r.top() + r.height() * 0.42);
    p.drawPath(eyeR);

    // 额头图案
    p.setBrush(pattern.lighter(120));
    QPainterPath brow;
    brow.moveTo(r.center().x(), r.top() + r.height() * 0.10);
    brow.lineTo(r.center().x() + r.width() * 0.12, r.top() + r.height() * 0.28);
    brow.lineTo(r.center().x(), r.top() + r.height() * 0.22);
    brow.lineTo(r.center().x() - r.width() * 0.12, r.top() + r.height() * 0.28);
    brow.closeSubpath();
    p.drawPath(brow);

    // 嘴
    p.setBrush(accent.darker(160));
    p.drawEllipse(QRectF(r.center().x() - r.width() * 0.10, r.top() + r.height() * 0.72,
                         r.width() * 0.20, r.height() * 0.09));
    Q_UNUSED(eyeW)
    Q_UNUSED(eyeH)
}

// 画一个简单的“戏曲人物”剪影（用于靠旗、翎子、髯口等题）
void drawOperaFigure(QPainter &p, const QRectF &r, const QColor &body, const QColor &skin)
{
    p.setPen(Qt::NoPen);
    // 身体
    p.setBrush(body);
    QPainterPath torso;
    torso.moveTo(r.center().x() - r.width() * 0.30, r.bottom());
    torso.lineTo(r.center().x() - r.width() * 0.18, r.top() + r.height() * 0.42);
    torso.lineTo(r.center().x() + r.width() * 0.18, r.top() + r.height() * 0.42);
    torso.lineTo(r.center().x() + r.width() * 0.30, r.bottom());
    torso.closeSubpath();
    p.drawPath(torso);
    // 头
    p.setBrush(skin);
    p.drawEllipse(QRectF(r.center().x() - r.width() * 0.14, r.top() + r.height() * 0.14,
                         r.width() * 0.28, r.height() * 0.28));
}

// 等比缩放（铺满）后居中裁剪到目标尺寸：绝不拉伸变形
QPixmap fitCover(const QPixmap &src, const QSize &size)
{
    if (src.isNull())
        return QPixmap();
    const QPixmap scaled =
        src.scaled(size, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    if (scaled.size() == size)
        return scaled;
    return scaled.copy(QRect((scaled.width() - size.width()) / 2,
                             (scaled.height() - size.height()) / 2, size.width(),
                             size.height()));
}

} // namespace

namespace Artwork {

// 4:3 画布上绘制的原始实现（定义在文件后半部）
static QPixmap mainBackgroundRaw(const QSize &size);
static QPixmap quizBackgroundRaw(const QSize &size);

// 两张背景图统一按 4:3 画布绘制，再等比裁剪到窗口/弹窗尺寸
QPixmap mainBackground(const QSize &size)
{
    const QSize safe(qMax(1, size.width()), qMax(1, size.height()));
    if (safe.width() * 3 == safe.height() * 4)
        return mainBackgroundRaw(safe);
    const int cw = qMax(safe.width(), safe.height() * 4 / 3);
    return fitCover(mainBackgroundRaw(QSize(cw, cw * 3 / 4)), safe);
}

QPixmap quizBackground(const QSize &size)
{
    const QSize safe(qMax(1, size.width()), qMax(1, size.height()));
    if (safe.width() * 3 == safe.height() * 4)
        return quizBackgroundRaw(safe);
    const int cw = qMax(safe.width(), safe.height() * 4 / 3);
    return fitCover(quizBackgroundRaw(QSize(cw, cw * 3 / 4)), safe);
}

// 4:3 成品导出尺寸（用于报告插图或替换素材前的对照）
QSize artworkCanvasSize() { return QSize(1600, 1200); }

QStringList resourceRoots()
{
    return buildResourceRoots();
}

QString findUserImage(const QStringList &relativeNames)
{
    return findFile(relativeNames);
}

void clearCache()
{
    g_cache.clear();
}

static QPixmap mainBackgroundRaw(const QSize &size)
{
    const QString key = QString("main|%1x%2").arg(size.width()).arg(size.height());
    if (g_cache.contains(key))
        return g_cache.value(key);

    const QSize safe(qMax(1, size.width()), qMax(1, size.height()));
    QPixmap pm(safe);
    pm.fill(kInk);

    // 用户放入的照片优先
    const QString userImg = findUserImage(withExtensions("backgrounds/main_bg"));
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);

    if (!userImg.isEmpty()) {
        QPixmap raw(userImg);
        if (!raw.isNull()) {
            const QPixmap scaled =
                raw.scaled(safe, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
            p.drawPixmap(QRect(QPoint(0, 0), safe),
                         scaled, QRect((scaled.width() - safe.width()) / 2,
                                       (scaled.height() - safe.height()) / 2, safe.width(),
                                       safe.height()));
            p.fillRect(QRect(QPoint(0, 0), safe), QColor(12, 8, 10, 110));
            p.end();
            g_cache.insert(key, pm);
            return pm;
        }
    }

    const int w = safe.width();
    const int h = safe.height();

    // 1. 底色：深红幕布渐变
    QLinearGradient bg(0, 0, w * 0.3, h);
    bg.setColorAt(0.0, QColor(96, 26, 26));
    bg.setColorAt(0.45, QColor(58, 16, 18));
    bg.setColorAt(1.0, QColor(20, 12, 14));
    p.fillRect(QRect(0, 0, w, h), bg);

    // 2. 顶部舞台光
    QRadialGradient light(QPointF(w * 0.5, h * 0.06), h * 0.75);
    light.setColorAt(0.0, QColor(255, 226, 168, 90));
    light.setColorAt(0.5, QColor(255, 200, 120, 30));
    light.setColorAt(1.0, QColor(0, 0, 0, 0));
    p.fillRect(QRect(0, 0, w, h), light);

    // 3. 远山与古城剪影（山西意象）
    drawMountains(p, QRectF(0, h * 0.55, w, h * 0.35), QColor(28, 18, 20, 210), 0.6, 7);
    drawMountains(p, QRectF(0, h * 0.64, w, h * 0.30), QColor(18, 12, 14, 230), 1.9, 5);

    // 4. 右侧楼阁剪影
    const QRectF pagoda(w * 0.74, h * 0.46, w * 0.20, h * 0.34);
    drawPagoda(p, pagoda, QColor(30, 20, 22, 230), 4);
    // 5. 左侧城墙剪影
    p.setBrush(QColor(26, 18, 20, 220));
    p.setPen(Qt::NoPen);
    const double wallH = h * 0.16;
    p.drawRect(QRectF(w * 0.04, h - wallH, w * 0.26, wallH));
    for (int i = 0; i < 6; ++i)
        p.drawRect(QRectF(w * 0.05 + i * w * 0.042, h - wallH - h * 0.022, w * 0.026, h * 0.024));

    // 6. 中央大脸谱水印
    const QRectF maskRect(w * 0.30, h * 0.12, w * 0.40, h * 0.62);
    p.setOpacity(0.16);
    drawMask(p, maskRect, QColor(240, 210, 150), QColor(60, 20, 20), kGoldLight);
    p.setOpacity(1.0);

    // 7. 幕布：顶部横幕 + 左右侧幕
    QLinearGradient valance(0, 0, 0, h * 0.12);
    valance.setColorAt(0.0, QColor(120, 30, 30));
    valance.setColorAt(1.0, QColor(64, 14, 16));
    p.setBrush(valance);
    p.setPen(Qt::NoPen);
    p.drawRect(QRectF(0, 0, w, h * 0.11));
    for (int i = 0; i < 22; ++i) {
        const double x = w * i / 22.0;
        p.setBrush(QColor(40, 8, 10, 90));
        p.drawRect(QRectF(x, 0, w * 0.012, h * 0.11));
    }
    // 金色流苏
    p.setBrush(kGold);
    p.drawRect(QRectF(0, h * 0.105, w, qMax(2.0, h * 0.006)));

    QLinearGradient sideL(0, 0, w * 0.06, 0);
    sideL.setColorAt(0.0, QColor(120, 30, 30, 230));
    sideL.setColorAt(1.0, QColor(70, 16, 18, 0));
    p.setBrush(sideL);
    p.drawRect(QRectF(0, 0, w * 0.06, h));
    QLinearGradient sideR(w, 0, w * 0.94, 0);
    sideR.setColorAt(0.0, QColor(120, 30, 30, 230));
    sideR.setColorAt(1.0, QColor(70, 16, 18, 0));
    p.setBrush(sideR);
    p.drawRect(QRectF(w * 0.94, 0, w * 0.06, h));

    // 8. 底部祥云纹
    for (int i = 0; i < 6; ++i) {
        const QPointF c(w * (0.08 + i * 0.166), h * 0.955);
        drawCloud(p, c, h * 0.035, QColor(212, 175, 55, 120));
    }
    // 9. 点缀金星
    p.setPen(Qt::NoPen);
    for (int i = 0; i < 40; ++i) {
        const double x = (i * 97 % 1000) / 1000.0 * w;
        const double y = (i * 53 % 1000) / 1000.0 * h * 0.8;
        const double r = (i % 3 == 0) ? 2.2 : 1.2;
        p.setBrush(QColor(240, 217, 140, 70 + (i * 7) % 90));
        p.drawEllipse(QPointF(x, y), r, r);
    }

    p.end();
    g_cache.insert(key, pm);
    return pm;
}

static QPixmap quizBackgroundRaw(const QSize &size)
{
    const QString key = QString("quiz|%1x%2").arg(size.width()).arg(size.height());
    if (g_cache.contains(key))
        return g_cache.value(key);

    const QSize safe(qMax(1, size.width()), qMax(1, size.height()));
    QPixmap pm(safe);
    pm.fill(kCurtainDark);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);

    const QString userImg = findUserImage(withExtensions("backgrounds/quiz_bg"));
    if (!userImg.isEmpty()) {
        QPixmap raw(userImg);
        if (!raw.isNull()) {
            const QPixmap scaled =
                raw.scaled(safe, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
            p.drawPixmap(QRect(QPoint(0, 0), safe),
                         scaled, QRect((scaled.width() - safe.width()) / 2,
                                       (scaled.height() - safe.height()) / 2, safe.width(),
                                       safe.height()));
            p.fillRect(QRect(QPoint(0, 0), safe), QColor(16, 8, 10, 140));
            p.end();
            g_cache.insert(key, pm);
            return pm;
        }
    }

    const int w = safe.width();
    const int h = safe.height();

    QLinearGradient bg(0, 0, w, h);
    bg.setColorAt(0.0, QColor(104, 28, 28));
    bg.setColorAt(0.55, QColor(66, 16, 18));
    bg.setColorAt(1.0, QColor(26, 12, 14));
    p.fillRect(QRect(0, 0, w, h), bg);

    // 幕布折痕
    p.setPen(Qt::NoPen);
    for (int i = 0; i < 26; ++i) {
        const double x = w * i / 26.0;
        p.setBrush(QColor(30, 6, 8, 70));
        p.drawRect(QRectF(x, 0, w * 0.008, h));
    }

    // 中央暗色内容板：保证文字清晰
    const QRectF panel(w * 0.035, h * 0.035, w * 0.93, h * 0.93);
    QLinearGradient panelGrad(panel.topLeft(), panel.bottomRight());
    panelGrad.setColorAt(0.0, QColor(24, 16, 20, 232));
    panelGrad.setColorAt(1.0, QColor(14, 10, 14, 240));
    p.setBrush(panelGrad);
    p.setPen(QPen(kGold, qMax(2.0, w * 0.004)));
    p.drawRoundedRect(panel, 14, 14);

    // 角部云纹
    drawCloud(p, QPointF(panel.left() + w * 0.05, panel.top() + h * 0.05), h * 0.030,
              QColor(212, 175, 55, 150));
    drawCloud(p, QPointF(panel.right() - w * 0.05, panel.bottom() - h * 0.04), h * 0.030,
              QColor(212, 175, 55, 150));

    // 右下角脸谱水印
    p.setOpacity(0.13);
    drawMask(p, QRectF(panel.right() - w * 0.30, panel.bottom() - h * 0.42, w * 0.26, h * 0.36),
             QColor(240, 210, 150), QColor(60, 20, 20), kGoldLight);
    p.setOpacity(1.0);

    p.end();
    g_cache.insert(key, pm);
    return pm;
}

QPixmap placeImage(const QString &key, const QString &title, const QSize &size)
{
    const QString cacheKey =
        QString("place|%1|%2x%3").arg(key).arg(size.width()).arg(size.height());
    if (g_cache.contains(cacheKey))
        return g_cache.value(cacheKey);

    const QSize safe(qMax(1, size.width()), qMax(1, size.height()));
    QPixmap pm(safe);
    pm.fill(QColor(24, 20, 22));
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);

    // 用户放入的真实照片优先（resources/places/<key>.png|jpg...）
    const QString userImg = findUserImage(withExtensions("places/" + key));
    if (!userImg.isEmpty()) {
        QPixmap raw(userImg);
        if (!raw.isNull()) {
            const QPixmap scaled =
                raw.scaled(safe, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
            p.drawPixmap(QRect(QPoint(0, 0), safe),
                         scaled, QRect((scaled.width() - safe.width()) / 2,
                                       (scaled.height() - safe.height()) / 2, safe.width(),
                                       safe.height()));
        } else {
            pm.fill(QColor(30, 26, 28));
        }
    } else {
        const int w = safe.width();
        const int h = safe.height();

        // 天空与地面
        QColor skyTop(120, 150, 180);
        QColor skyBottom(226, 200, 158);
        QColor ground(96, 82, 62);
        if (key == "hukoupubu" || key == "qikouguzhen") {
            skyTop = QColor(140, 160, 178);
            skyBottom = QColor(238, 214, 172);
            ground = QColor(150, 120, 74);
        } else if (key == "wutaishan" || key == "mianshan" || key == "yanmenguan") {
            skyTop = QColor(96, 124, 158);
            skyBottom = QColor(206, 214, 214);
            ground = QColor(84, 92, 88);
        } else if (key == "laochencufang" || key == "hongtongdahuaishu" || key == "qiaojiadayuan") {
            skyTop = QColor(150, 132, 108);
            skyBottom = QColor(238, 208, 158);
            ground = QColor(104, 78, 56);
        }
        QLinearGradient sky(0, 0, 0, h);
        sky.setColorAt(0.0, skyTop);
        sky.setColorAt(0.72, skyBottom);
        sky.setColorAt(1.0, skyBottom.darker(115));
        p.fillRect(QRect(0, 0, w, h), sky);

        // 太阳
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(255, 240, 200, 170));
        p.drawEllipse(QPointF(w * 0.78, h * 0.22), h * 0.075, h * 0.075);

        // 远山
        drawMountains(p, QRectF(0, h * 0.34, w, h * 0.34), ground.lighter(150).darker(110), 0.4, 6);
        drawMountains(p, QRectF(0, h * 0.44, w, h * 0.30), ground.darker(105), 1.6, 4);

        const double gy = h * 0.74; // 地平线
        p.setBrush(ground);
        p.drawRect(QRectF(0, gy, w, h - gy));

        const QColor dark = ground.darker(180);
        const QColor roofC = QColor(70, 58, 52);
        const QColor wallC = QColor(196, 178, 150);

        if (key == "jinci") {
            // 晋祠：祠堂 + 泉水 + 古树
            p.setBrush(wallC);
            p.drawRect(QRectF(w * 0.28, gy - h * 0.16, w * 0.44, h * 0.16));
            drawRoof(p, QRectF(w * 0.24, gy - h * 0.24, w * 0.52, h * 0.09), roofC);
            p.setBrush(dark);
            for (int i = 0; i < 4; ++i)
                p.drawRect(QRectF(w * (0.32 + i * 0.10), gy - h * 0.13, w * 0.035, h * 0.13));
            p.setBrush(QColor(90, 120, 110));
            p.drawEllipse(QRectF(w * 0.10, gy + h * 0.04, w * 0.26, h * 0.10));
            p.setBrush(QColor(48, 66, 44));
            p.drawEllipse(QRectF(w * 0.80, gy - h * 0.20, w * 0.12, h * 0.20));
            p.drawRect(QRectF(w * 0.852, gy - h * 0.06, w * 0.02, h * 0.06));
        } else if (key == "qiaojiadayuan") {
            // 晋商大院：高墙 + 门楼 + 灯笼
            p.setBrush(wallC.darker(110));
            p.drawRect(QRectF(w * 0.16, gy - h * 0.20, w * 0.68, h * 0.20));
            drawRoof(p, QRectF(w * 0.40, gy - h * 0.30, w * 0.20, h * 0.08), roofC);
            p.setBrush(QColor(90, 52, 40));
            p.drawRect(QRectF(w * 0.44, gy - h * 0.15, w * 0.12, h * 0.15));
            p.setBrush(QColor(190, 60, 50));
            p.drawEllipse(QRectF(w * 0.40, gy - h * 0.26, w * 0.045, h * 0.055));
            p.drawEllipse(QRectF(w * 0.555, gy - h * 0.26, w * 0.045, h * 0.055));
        } else if (key == "pingyaogucheng") {
            // 平遥古城：城墙 + 城楼
            p.setBrush(QColor(120, 106, 92));
            p.drawRect(QRectF(0, gy - h * 0.18, w, h * 0.18));
            for (int i = 0; i < 12; ++i)
                p.drawRect(QRectF(i * w / 12.0, gy - h * 0.22, w * 0.045, h * 0.045));
            drawRoof(p, QRectF(w * 0.36, gy - h * 0.36, w * 0.28, h * 0.09), roofC);
            p.setBrush(wallC.darker(120));
            p.drawRect(QRectF(w * 0.40, gy - h * 0.28, w * 0.20, h * 0.10));
        } else if (key == "yungangshiku") {
            // 云冈石窟：崖壁 + 佛龛 + 大佛
            p.setBrush(QColor(150, 128, 100));
            p.drawRect(QRectF(0, gy - h * 0.42, w, h * 0.42));
            p.setBrush(QColor(96, 78, 58));
            for (int i = 0; i < 5; ++i) {
                const double bx = w * (0.10 + i * 0.17);
                p.drawRoundedRect(QRectF(bx, gy - h * 0.34, w * 0.12, h * 0.26), 8, 8);
                p.setBrush(QColor(206, 178, 138));
                p.drawEllipse(QRectF(bx + w * 0.032, gy - h * 0.31, w * 0.055, h * 0.10));
                p.setBrush(QColor(96, 78, 58));
            }
        } else if (key == "hukoupubu") {
            // 壶口瀑布：黄河瀑布 + 水雾
            p.setBrush(QColor(176, 138, 88));
            p.drawRect(QRectF(0, gy - h * 0.10, w, h * 0.10));
            p.setBrush(QColor(226, 232, 236, 220));
            p.drawRect(QRectF(w * 0.18, gy - h * 0.12, w * 0.64, h * 0.30));
            p.setBrush(QColor(250, 250, 250, 140));
            for (int i = 0; i < 7; ++i)
                p.drawEllipse(QRectF(w * (0.15 + i * 0.10), gy + h * 0.10, w * 0.12, h * 0.10));
            p.setBrush(QColor(150, 120, 80, 200));
            p.drawRect(QRectF(0, gy + h * 0.16, w, h * 0.10));
        } else if (key == "wutaishan") {
            // 五台山：白塔 + 群山
            drawPagoda(p, QRectF(w * 0.40, gy - h * 0.30, w * 0.20, h * 0.30),
                       QColor(238, 236, 230), 2);
            p.setBrush(QColor(206, 202, 194));
            p.drawRect(QRectF(w * 0.36, gy - h * 0.10, w * 0.28, h * 0.10));
        } else if (key == "xuankongsi") {
            // 悬空寺：悬崖 + 支撑柱
            p.setBrush(QColor(122, 100, 78));
            p.drawRect(QRectF(0, gy - h * 0.52, w * 0.62, h * 0.52));
            p.setBrush(QColor(160, 142, 112));
            p.drawRect(QRectF(w * 0.56, gy - h * 0.30, w * 0.30, h * 0.16));
            drawRoof(p, QRectF(w * 0.54, gy - h * 0.38, w * 0.34, h * 0.08), roofC);
            p.setBrush(QColor(70, 52, 36));
            for (int i = 0; i < 4; ++i)
                p.drawRect(QRectF(w * (0.58 + i * 0.07), gy - h * 0.14, w * 0.012, h * 0.14));
        } else if (key == "yingxianmuta") {
            // 应县木塔
            p.setBrush(QColor(120, 88, 58));
            drawPagoda(p, QRectF(w * 0.34, gy - h * 0.46, w * 0.32, h * 0.46),
                       QColor(126, 92, 60), 5);
        } else if (key == "yanmenguan") {
            // 雁门关：关城 + 长城
            p.setBrush(QColor(112, 100, 88));
            p.drawRect(QRectF(0, gy - h * 0.16, w, h * 0.16));
            for (int i = 0; i < 14; ++i)
                p.drawRect(QRectF(i * w / 14.0, gy - h * 0.20, w * 0.04, h * 0.04));
            p.setBrush(QColor(96, 84, 72));
            p.drawRect(QRectF(w * 0.40, gy - h * 0.34, w * 0.20, h * 0.18));
            drawRoof(p, QRectF(w * 0.37, gy - h * 0.42, w * 0.26, h * 0.08), roofC);
        } else if (key == "niangziguan") {
            // 娘子关：关城 + 瀑布
            p.setBrush(QColor(110, 98, 86));
            p.drawRect(QRectF(w * 0.10, gy - h * 0.26, w * 0.34, h * 0.26));
            drawRoof(p, QRectF(w * 0.07, gy - h * 0.34, w * 0.40, h * 0.08), roofC);
            p.setBrush(QColor(232, 240, 244, 210));
            p.drawRect(QRectF(w * 0.62, gy - h * 0.28, w * 0.10, h * 0.36));
            p.setBrush(QColor(250, 250, 250, 130));
            p.drawEllipse(QRectF(w * 0.58, gy + h * 0.06, w * 0.18, h * 0.10));
        } else if (key == "mianshan") {
            // 绵山：层峦 + 山寺
            drawMountains(p, QRectF(0, h * 0.34, w, h * 0.46), QColor(88, 96, 80), 2.4, 5);
            p.setBrush(QColor(200, 186, 158));
            p.drawRect(QRectF(w * 0.60, h * 0.44, w * 0.18, h * 0.10));
            drawRoof(p, QRectF(w * 0.57, h * 0.38, w * 0.24, h * 0.07), roofC);
        } else if (key == "qikouguzhen") {
            // 碛口古镇：黄河 + 窑洞 + 帆船
            p.setBrush(QColor(170, 132, 84));
            p.drawRect(QRectF(0, gy + h * 0.02, w, h * 0.24));
            p.setBrush(QColor(150, 126, 96));
            p.drawRect(QRectF(0, gy - h * 0.22, w * 0.55, h * 0.24));
            p.setBrush(QColor(80, 62, 44));
            for (int i = 0; i < 3; ++i)
                p.drawRoundedRect(QRectF(w * (0.06 + i * 0.16), gy - h * 0.16, w * 0.11, h * 0.11), 14, 14);
            // 帆船
            p.setBrush(QColor(240, 236, 226));
            QPainterPath sail;
            sail.moveTo(w * 0.72, gy + h * 0.04);
            sail.lineTo(w * 0.78, gy - h * 0.10);
            sail.lineTo(w * 0.80, gy + h * 0.04);
            sail.closeSubpath();
            p.drawPath(sail);
            p.setBrush(QColor(92, 66, 44));
            p.drawRect(QRectF(w * 0.68, gy + h * 0.04, w * 0.16, h * 0.022));
        } else if (key == "laochencufang") {
            // 老陈醋坊：作坊 + 醋缸
            p.setBrush(wallC);
            p.drawRect(QRectF(w * 0.12, gy - h * 0.26, w * 0.76, h * 0.26));
            drawRoof(p, QRectF(w * 0.09, gy - h * 0.34, w * 0.82, h * 0.08), roofC);
            p.setBrush(QColor(70, 46, 32));
            for (int i = 0; i < 3; ++i)
                p.drawRoundedRect(QRectF(w * (0.20 + i * 0.22), gy - h * 0.10, w * 0.13, h * 0.10),
                                  8, 8);
            p.setBrush(QColor(150, 40, 40));
            p.drawRect(QRectF(w * 0.56, gy - h * 0.36, w * 0.05, h * 0.12));
        } else if (key == "baquanxia") {
            // 八泉峡：两侧峭壁夹着一道峡谷，崖间多股泉水跌落成溪
            p.setBrush(QColor(118, 112, 96));
            QPainterPath cliffL;
            cliffL.moveTo(0, h);
            cliffL.lineTo(0, h * 0.26);
            cliffL.lineTo(w * 0.16, h * 0.20);
            cliffL.lineTo(w * 0.30, h * 0.46);
            cliffL.lineTo(w * 0.26, h);
            cliffL.closeSubpath();
            p.drawPath(cliffL);
            QPainterPath cliffR;
            cliffR.moveTo(w, h);
            cliffR.lineTo(w, h * 0.22);
            cliffR.lineTo(w * 0.84, h * 0.17);
            cliffR.lineTo(w * 0.70, h * 0.48);
            cliffR.lineTo(w * 0.76, h);
            cliffR.closeSubpath();
            p.drawPath(cliffR);
            // 崖间泉水（多股细瀑布）
            p.setBrush(QColor(244, 250, 252, 230));
            for (int i = 0; i < 4; ++i) {
                const double bx = w * (0.34 + i * 0.09);
                p.drawRect(QRectF(bx, h * (0.30 + (i % 2) * 0.06), w * 0.035, h * 0.42));
                p.setBrush(QColor(250, 252, 255, 150));
                p.drawEllipse(QRectF(bx - w * 0.02, h * (0.66 + (i % 2) * 0.06), w * 0.075, h * 0.06));
                p.setBrush(QColor(244, 250, 252, 230));
            }
            // 谷底溪流与石块
            p.setBrush(QColor(150, 186, 190));
            p.drawRect(QRectF(w * 0.26, h * 0.78, w * 0.48, h * 0.22));
            p.setBrush(QColor(96, 92, 84));
            p.drawEllipse(QRectF(w * 0.30, h * 0.74, w * 0.07, h * 0.06));
            p.drawEllipse(QRectF(w * 0.62, h * 0.76, w * 0.09, h * 0.06));
        } else if (key == "hongtongdahuaishu") {
            // 洪洞大槐树：古槐 + 碑亭 + 寻根祭祖的红绸
            p.setBrush(QColor(92, 66, 44));
            QPainterPath trunk;
            trunk.moveTo(w * 0.44, gy);
            trunk.lineTo(w * 0.47, gy - h * 0.24);
            trunk.lineTo(w * 0.56, gy - h * 0.24);
            trunk.lineTo(w * 0.59, gy);
            trunk.closeSubpath();
            p.drawPath(trunk);
            // 树冠：几团叠加，显得苍老茂密
            p.setBrush(QColor(64, 96, 56));
            p.drawEllipse(QRectF(w * 0.24, gy - h * 0.52, w * 0.50, h * 0.30));
            p.drawEllipse(QRectF(w * 0.32, gy - h * 0.66, w * 0.34, h * 0.26));
            p.setBrush(QColor(80, 114, 66));
            p.drawEllipse(QRectF(w * 0.30, gy - h * 0.58, w * 0.30, h * 0.22));
            // 碑亭与石碑
            p.setBrush(QColor(196, 178, 150));
            p.drawRect(QRectF(w * 0.68, gy - h * 0.18, w * 0.20, h * 0.18));
            drawRoof(p, QRectF(w * 0.65, gy - h * 0.26, w * 0.26, h * 0.08), roofC);
            p.setBrush(QColor(162, 152, 134));
            p.drawRect(QRectF(w * 0.73, gy - h * 0.14, w * 0.10, h * 0.14));
            // 祭祖红绸
            p.setBrush(QColor(190, 60, 50));
            p.drawRect(QRectF(w * 0.52, gy - h * 0.32, w * 0.028, h * 0.15));
            p.drawRect(QRectF(w * 0.40, gy - h * 0.29, w * 0.024, h * 0.13));
        } else {
            // 兜底：远山 + 小亭
            drawRoof(p, QRectF(w * 0.38, gy - h * 0.20, w * 0.24, h * 0.08), roofC);
            p.setBrush(wallC);
            p.drawRect(QRectF(w * 0.44, gy - h * 0.12, w * 0.12, h * 0.12));
        }
    }

    // 底部标题条 + 金边
    const int w = safe.width();
    const int h = safe.height();
    p.setBrush(QColor(12, 10, 12, 190));
    p.setPen(Qt::NoPen);
    p.drawRect(QRectF(0, h * 0.82, w, h * 0.18));
    drawTextFit(p, QRect(6, int(h * 0.83), w - 12, int(h * 0.16)), title,
                qMax(12, int(h * 0.10)), kGoldLight, true, 9);
    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(kGold, 2));
    p.drawRect(QRectF(0.5, 0.5, w - 1.0, h - 1.0));

    p.end();
    g_cache.insert(cacheKey, pm);
    return pm;
}

QPixmap quizImage(const QString &key, const QSize &size)
{
    const QString cacheKey = QString("qi|%1|%2x%3").arg(key).arg(size.width()).arg(size.height());
    if (g_cache.contains(cacheKey))
        return g_cache.value(cacheKey);

    const QSize safe(qMax(1, size.width()), qMax(1, size.height()));
    QPixmap pm(safe);
    pm.fill(QColor(28, 22, 24));
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);

    const QString userImg = findUserImage(withExtensions("quiz/" + key));
    if (!userImg.isEmpty()) {
        QPixmap raw(userImg);
        if (!raw.isNull()) {
            const QPixmap scaled =
                raw.scaled(safe, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
            p.drawPixmap(QRect(QPoint(0, 0), safe),
                         scaled, QRect((scaled.width() - safe.width()) / 2,
                                       (scaled.height() - safe.height()) / 2, safe.width(),
                                       safe.height()));
            p.end();
            g_cache.insert(cacheKey, pm);
            return pm;
        }
    }

    const int w = safe.width();
    const int h = safe.height();
    QLinearGradient bg(0, 0, 0, h);
    bg.setColorAt(0.0, QColor(72, 52, 46));
    bg.setColorAt(1.0, QColor(32, 22, 22));
    p.fillRect(QRect(0, 0, w, h), bg);

    const QRectF stage(w * 0.06, h * 0.06, w * 0.88, h * 0.88);
    const QColor skin(238, 214, 186);
    const QColor wood(126, 88, 56);
    const QColor cloth(150, 40, 42);

    if (key.startsWith("mask_")) {
        QColor base(200, 40, 40), accent(30, 20, 20), pattern(240, 220, 160);
        if (key == "mask_black") {
            base = QColor(48, 46, 48);
            accent = QColor(240, 240, 240);
            pattern = QColor(220, 200, 150);
        } else if (key == "mask_white") {
            base = QColor(240, 238, 232);
            accent = QColor(40, 36, 40);
            pattern = QColor(60, 60, 70);
        } else if (key == "mask_gold") {
            base = QColor(214, 178, 84);
            accent = QColor(60, 40, 20);
            pattern = QColor(250, 240, 200);
        } else if (key == "mask_ugly") {
            base = QColor(245, 240, 230);
            accent = QColor(60, 50, 46);
            pattern = QColor(50, 46, 44);
        }
        drawMask(p, stage, base, accent, pattern);
        if (key == "mask_ugly") {
            p.setBrush(QColor(255, 255, 255));
            p.setPen(Qt::NoPen);
            p.drawEllipse(QRectF(stage.center().x() - stage.width() * 0.10,
                                 stage.center().y() - stage.height() * 0.02,
                                 stage.width() * 0.20, stage.height() * 0.14));
        }
    } else if (key.startsWith("beard_")) {
        const bool white = (key == "beard_white");
        p.setBrush(skin);
        p.setPen(Qt::NoPen);
        p.drawEllipse(QRectF(stage.center().x() - stage.width() * 0.24, stage.top(),
                             stage.width() * 0.48, stage.height() * 0.56));
        p.setBrush(QColor(40, 34, 32));
        p.drawEllipse(QRectF(stage.center().x() - stage.width() * 0.25, stage.top() + stage.height() * 0.02,
                             stage.width() * 0.50, stage.height() * 0.20));
        p.setBrush(white ? QColor(245, 245, 242) : QColor(34, 30, 30));
        QPainterPath beard;
        beard.moveTo(stage.center().x() - stage.width() * 0.20, stage.top() + stage.height() * 0.40);
        beard.cubicTo(stage.center().x() - stage.width() * 0.26, stage.bottom() - stage.height() * 0.06,
                      stage.center().x() + stage.width() * 0.26, stage.bottom() - stage.height() * 0.06,
                      stage.center().x() + stage.width() * 0.20, stage.top() + stage.height() * 0.40);
        beard.cubicTo(stage.center().x(), stage.top() + stage.height() * 0.52,
                      stage.center().x(), stage.top() + stage.height() * 0.52,
                      stage.center().x() - stage.width() * 0.20, stage.top() + stage.height() * 0.40);
        p.drawPath(beard);
    } else if (key == "sleeve_water") {
        p.setBrush(cloth);
        p.setPen(Qt::NoPen);
        p.drawRect(QRectF(stage.left() + stage.width() * 0.24, stage.top(),
                          stage.width() * 0.52, stage.height() * 0.32));
        QPainterPath sleeve;
        sleeve.moveTo(stage.left() + stage.width() * 0.28, stage.top() + stage.height() * 0.28);
        sleeve.cubicTo(stage.left() + stage.width() * 0.10, stage.top() + stage.height() * 0.55,
                       stage.left() + stage.width() * 0.22, stage.bottom() - stage.height() * 0.02,
                       stage.left() + stage.width() * 0.46, stage.bottom() - stage.height() * 0.06);
        sleeve.cubicTo(stage.left() + stage.width() * 0.70, stage.bottom() - stage.height() * 0.10,
                       stage.right() - stage.width() * 0.08, stage.bottom() - stage.height() * 0.22,
                       stage.right() - stage.width() * 0.14, stage.top() + stage.height() * 0.30);
        sleeve.closeSubpath();
        p.setBrush(QColor(250, 250, 248));
        p.drawPath(sleeve);
    } else if (key == "feather_lingzi") {
        drawOperaFigure(p, stage.adjusted(stage.width() * 0.18, stage.height() * 0.24, -stage.width() * 0.18, 0),
                        cloth, skin);
        p.setBrush(QColor(30, 30, 36));
        p.drawRect(QRectF(stage.center().x() - stage.width() * 0.22, stage.top() + stage.height() * 0.16,
                          stage.width() * 0.44, stage.height() * 0.10));
        QPen pen(QColor(120, 90, 60), qMax(2.0, w * 0.012));
        pen.setCapStyle(Qt::RoundCap);
        p.setPen(pen);
        p.drawLine(QPointF(stage.center().x() - stage.width() * 0.12, stage.top() + stage.height() * 0.18),
                   QPointF(stage.center().x() - stage.width() * 0.34, stage.top() - stage.height() * 0.02));
        p.drawLine(QPointF(stage.center().x() + stage.width() * 0.12, stage.top() + stage.height() * 0.18),
                   QPointF(stage.center().x() + stage.width() * 0.34, stage.top() - stage.height() * 0.02));
    } else if (key == "prop_whip") {
        p.setPen(QPen(wood, qMax(3.0, w * 0.018)));
        p.drawLine(QPointF(stage.left() + stage.width() * 0.22, stage.bottom() - stage.height() * 0.06),
                   QPointF(stage.right() - stage.width() * 0.30, stage.top() + stage.height() * 0.20));
        p.setPen(QPen(QColor(210, 180, 120), qMax(2.0, w * 0.010)));
        for (int i = 0; i < 7; ++i) {
            const double t = i / 7.0;
            const QPointF base(stage.left() + stage.width() * (0.30 + t * 0.42),
                               stage.bottom() - stage.height() * (0.10 + t * 0.62));
            p.drawLine(base, QPointF(base.x() - stage.width() * 0.05, base.y() + stage.height() * 0.08));
        }
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(200, 60, 50));
        p.drawEllipse(QRectF(stage.right() - stage.width() * 0.38, stage.top() + stage.height() * 0.14,
                             stage.width() * 0.10, stage.height() * 0.08));
    } else if (key == "prop_oar") {
        p.setPen(QPen(wood, qMax(4.0, w * 0.022)));
        p.drawLine(QPointF(stage.left() + stage.width() * 0.26, stage.top() + stage.height() * 0.18),
                   QPointF(stage.right() - stage.width() * 0.26, stage.bottom() - stage.height() * 0.14));
        p.setBrush(wood.lighter(115));
        p.setPen(Qt::NoPen);
        p.drawEllipse(QRectF(stage.right() - stage.width() * 0.36, stage.bottom() - stage.height() * 0.26,
                             stage.width() * 0.20, stage.height() * 0.22));
    } else if (key == "stage_table") {
        p.setBrush(wood);
        p.setPen(Qt::NoPen);
        p.drawRect(QRectF(stage.left() + stage.width() * 0.14, stage.top() + stage.height() * 0.34,
                          stage.width() * 0.72, stage.height() * 0.10));
        p.drawRect(QRectF(stage.left() + stage.width() * 0.20, stage.top() + stage.height() * 0.44,
                          stage.width() * 0.05, stage.height() * 0.36));
        p.drawRect(QRectF(stage.right() - stage.width() * 0.25, stage.top() + stage.height() * 0.44,
                          stage.width() * 0.05, stage.height() * 0.36));
        for (int s = 0; s < 2; ++s) {
            const double x = (s == 0) ? stage.left() + stage.width() * 0.06 : stage.right() - stage.width() * 0.26;
            p.drawRect(QRectF(x, stage.top() + stage.height() * 0.46, stage.width() * 0.20, stage.height() * 0.08));
            p.drawRect(QRectF(x + stage.width() * 0.02, stage.top() + stage.height() * 0.54,
                              stage.width() * 0.035, stage.height() * 0.28));
            p.drawRect(QRectF(x + stage.width() * 0.145, stage.top() + stage.height() * 0.54,
                              stage.width() * 0.035, stage.height() * 0.28));
            p.drawRect(QRectF(x, stage.top() + stage.height() * 0.40, stage.width() * 0.20, stage.height() * 0.06));
        }
    } else if (key == "inst_huqin") {
        p.setBrush(QColor(90, 40, 30));
        p.setPen(Qt::NoPen);
        p.drawEllipse(QRectF(stage.center().x() - stage.width() * 0.12, stage.bottom() - stage.height() * 0.30,
                             stage.width() * 0.24, stage.height() * 0.26));
        p.setBrush(wood);
        p.drawRect(QRectF(stage.center().x() - stage.width() * 0.025, stage.top(),
                          stage.width() * 0.05, stage.height() * 0.72));
        p.setBrush(QColor(60, 40, 30));
        p.drawRect(QRectF(stage.center().x() - stage.width() * 0.16, stage.top() + stage.height() * 0.18,
                          stage.width() * 0.32, stage.height() * 0.07));
        p.setPen(QPen(QColor(230, 230, 220), 1.5));
        p.drawLine(QPointF(stage.center().x() - stage.width() * 0.02, stage.top() + stage.height() * 0.02),
                   QPointF(stage.center().x() - stage.width() * 0.02, stage.bottom() - stage.height() * 0.22));
        p.drawLine(QPointF(stage.center().x() + stage.width() * 0.02, stage.top() + stage.height() * 0.02),
                   QPointF(stage.center().x() + stage.width() * 0.02, stage.bottom() - stage.height() * 0.22));
    } else if (key == "inst_bangzi") {
        p.setPen(Qt::NoPen);
        p.setBrush(wood);
        p.drawRoundedRect(QRectF(stage.left() + stage.width() * 0.18, stage.top() + stage.height() * 0.30,
                                 stage.width() * 0.16, stage.height() * 0.52), 8, 8);
        p.drawRoundedRect(QRectF(stage.left() + stage.width() * 0.44, stage.top() + stage.height() * 0.20,
                                 stage.width() * 0.16, stage.height() * 0.52), 8, 8);
        p.setPen(QPen(wood.lighter(120), qMax(3.0, w * 0.016)));
        p.drawLine(QPointF(stage.left() + stage.width() * 0.66, stage.bottom() - stage.height() * 0.18),
                   QPointF(stage.right() - stage.width() * 0.06, stage.top() + stage.height() * 0.20));
    } else if (key == "inst_gong") {
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(198, 160, 60));
        p.drawEllipse(QRectF(stage.left() + stage.width() * 0.06, stage.top() + stage.height() * 0.12,
                             stage.width() * 0.46, stage.height() * 0.46));
        p.setBrush(QColor(160, 124, 46));
        p.drawEllipse(QRectF(stage.left() + stage.width() * 0.13, stage.top() + stage.height() * 0.19,
                             stage.width() * 0.32, stage.height() * 0.32));
        p.setBrush(QColor(180, 60, 48));
        p.drawEllipse(QRectF(stage.left() + stage.width() * 0.52, stage.top() + stage.height() * 0.48,
                             stage.width() * 0.34, stage.height() * 0.30));
        p.setBrush(QColor(240, 236, 222));
        p.drawEllipse(QRectF(stage.left() + stage.width() * 0.55, stage.top() + stage.height() * 0.51,
                             stage.width() * 0.28, stage.height() * 0.24));
        p.setPen(QPen(wood, qMax(3.0, w * 0.014)));
        p.drawLine(QPointF(stage.right() - stage.width() * 0.24, stage.bottom() - stage.height() * 0.06),
                   QPointF(stage.right() - stage.width() * 0.08, stage.top() + stage.height() * 0.34));
    } else if (key == "costume_mang" || key == "costume_kao") {
        const QColor robe = (key == "costume_mang") ? QColor(160, 46, 48) : QColor(46, 70, 110);
        QPainterPath body;
        body.moveTo(stage.center().x() - stage.width() * 0.30, stage.bottom());
        body.lineTo(stage.center().x() - stage.width() * 0.18, stage.top() + stage.height() * 0.18);
        body.lineTo(stage.center().x(), stage.top() + stage.height() * 0.10);
        body.lineTo(stage.center().x() + stage.width() * 0.18, stage.top() + stage.height() * 0.18);
        body.lineTo(stage.center().x() + stage.width() * 0.30, stage.bottom());
        body.closeSubpath();
        p.setBrush(robe);
        p.setPen(Qt::NoPen);
        p.drawPath(body);
        // 玉带
        p.setBrush(kGold);
        p.drawRect(QRectF(stage.center().x() - stage.width() * 0.24, stage.top() + stage.height() * 0.44,
                          stage.width() * 0.48, stage.height() * 0.07));
        if (key == "costume_mang") {
            p.setPen(QPen(kGoldLight, qMax(2.0, w * 0.008)));
            p.setBrush(Qt::NoBrush);
            QPainterPath dragon;
            dragon.moveTo(stage.center().x() - stage.width() * 0.14, stage.top() + stage.height() * 0.60);
            dragon.cubicTo(stage.center().x() - stage.width() * 0.02, stage.top() + stage.height() * 0.52,
                           stage.center().x() + stage.width() * 0.10, stage.top() + stage.height() * 0.66,
                           stage.center().x() + stage.width() * 0.14, stage.top() + stage.height() * 0.58);
            p.drawPath(dragon);
        } else {
            // 靠旗
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(240, 236, 220));
            for (int i = 0; i < 4; ++i) {
                const double dx = (i < 2 ? -1 : 1) * stage.width() * (i % 2 == 0 ? 0.34 : 0.20);
                QPainterPath flag;
                flag.moveTo(stage.center().x(), stage.top() + stage.height() * 0.22);
                flag.lineTo(stage.center().x() + dx, stage.top() - stage.height() * 0.02);
                flag.lineTo(stage.center().x() + dx * 1.1, stage.top() + stage.height() * 0.16);
                flag.closeSubpath();
                p.drawPath(flag);
            }
        }
    } else if (key == "prop_fan") {
        p.setBrush(QColor(246, 240, 224));
        p.setPen(QPen(QColor(120, 90, 60), 2));
        QPainterPath fan;
        fan.moveTo(stage.center().x(), stage.bottom() - stage.height() * 0.18);
        fan.arcTo(QRectF(stage.center().x() - stage.width() * 0.34,
                         stage.bottom() - stage.height() * 0.86,
                         stage.width() * 0.68, stage.height() * 0.68),
                  30, 120);
        fan.closeSubpath();
        p.drawPath(fan);
        p.setPen(QPen(QColor(150, 110, 70), 1.2));
        for (int i = 0; i <= 6; ++i) {
            const double ang = (30 + i * 20) * kPi / 180.0;
            p.drawLine(QPointF(stage.center().x(), stage.bottom() - stage.height() * 0.18),
                       QPointF(stage.center().x() + std::cos(ang) * stage.width() * 0.34,
                               stage.bottom() - stage.height() * 0.18 - std::sin(ang) * stage.height() * 0.34));
        }
    } else if (key == "prop_whisk") {
        p.setPen(QPen(wood, qMax(3.0, w * 0.016)));
        p.drawLine(QPointF(stage.left() + stage.width() * 0.34, stage.bottom() - stage.height() * 0.10),
                   QPointF(stage.center().x() + stage.width() * 0.02, stage.top() + stage.height() * 0.42));
        p.setPen(QPen(QColor(246, 246, 242), qMax(2.0, w * 0.008)));
        for (int i = -4; i <= 4; ++i) {
            p.drawLine(QPointF(stage.center().x() + stage.width() * 0.02, stage.top() + stage.height() * 0.42),
                       QPointF(stage.center().x() + stage.width() * 0.02 + i * stage.width() * 0.035,
                               stage.top() + stage.height() * 0.02));
        }
    } else if (key == "prop_handkerchief") {
        p.setBrush(QColor(230, 90, 120));
        p.setPen(QPen(kGoldLight, 2));
        p.drawRoundedRect(QRectF(stage.center().x() - stage.width() * 0.26,
                                 stage.center().y() - stage.height() * 0.26,
                                 stage.width() * 0.52, stage.height() * 0.52), 10, 10);
        p.setBrush(kGoldLight);
        p.setPen(Qt::NoPen);
        p.drawEllipse(QPointF(stage.center().x(), stage.center().y()), w * 0.035, w * 0.035);
    } else if (key == "costume_boot") {
        p.setBrush(QColor(24, 22, 24));
        p.setPen(Qt::NoPen);
        QPainterPath boot;
        boot.moveTo(stage.center().x() - stage.width() * 0.12, stage.top() + stage.height() * 0.06);
        boot.lineTo(stage.center().x() + stage.width() * 0.10, stage.top() + stage.height() * 0.10);
        boot.lineTo(stage.center().x() + stage.width() * 0.16, stage.bottom() - stage.height() * 0.22);
        boot.lineTo(stage.center().x() - stage.width() * 0.34, stage.bottom() - stage.height() * 0.14);
        boot.lineTo(stage.center().x() - stage.width() * 0.34, stage.bottom() - stage.height() * 0.02);
        boot.lineTo(stage.center().x() + stage.width() * 0.22, stage.bottom() - stage.height() * 0.02);
        boot.lineTo(stage.center().x() + stage.width() * 0.22, stage.bottom() - stage.height() * 0.10);
        boot.closeSubpath();
        p.setBrush(QColor(30, 28, 30));
        p.drawPath(boot);
        p.setBrush(QColor(220, 216, 208));
        p.drawRect(QRectF(stage.center().x() - stage.width() * 0.34, stage.bottom() - stage.height() * 0.04,
                          stage.width() * 0.56, stage.height() * 0.04));
    } else {
        // 兜底插画：脸谱
        drawMask(p, stage, QColor(190, 60, 50), QColor(40, 30, 30), kGoldLight);
    }

    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(kGold, 2));
    p.drawRect(QRectF(0.5, 0.5, w - 1.0, h - 1.0));
    p.end();
    g_cache.insert(cacheKey, pm);
    return pm;
}

void paintTokenFace(QPainter *painter, const QRectF &rect, const QColor &color)
{
    if (!painter)
        return;
    painter->save();
    painter->setPen(Qt::NoPen);
    painter->setBrush(color);
    painter->drawEllipse(rect);
    // 脸谱式的白色眼纹，让棋子更像戏曲脸谱
    const double w = rect.width();
    const double h = rect.height();
    painter->setBrush(QColor(255, 255, 255, 210));
    QPainterPath left;
    left.moveTo(rect.center().x() - w * 0.05, rect.center().y() - h * 0.06);
    left.cubicTo(rect.center().x() - w * 0.30, rect.center().y() - h * 0.26,
                 rect.center().x() - w * 0.36, rect.center().y() + h * 0.06,
                 rect.center().x() - w * 0.10, rect.center().y() - h * 0.02);
    left.closeSubpath();
    painter->drawPath(left);
    QPainterPath right;
    right.moveTo(rect.center().x() + w * 0.05, rect.center().y() - h * 0.06);
    right.cubicTo(rect.center().x() + w * 0.30, rect.center().y() - h * 0.26,
                  rect.center().x() + w * 0.36, rect.center().y() + h * 0.06,
                  rect.center().x() + w * 0.10, rect.center().y() - h * 0.02);
    right.closeSubpath();
    painter->drawPath(right);
    painter->restore();
}

} // namespace Artwork
