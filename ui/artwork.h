#ifndef ARTWORK_H
#define ARTWORK_H

#include <QColor>
#include <QPixmap>
#include <QRect>
#include <QSize>
#include <QString>
#include <QStringList>

class QPainter;
class QRectF;

// 美术模块：所有背景与插画都由代码绘制（QPainter），
// 若用户把真实照片放进 resources 目录，则优先使用照片。
namespace Artwork {

// 在若干候选路径（相对可执行文件目录）中查找第一个存在的图片文件，找不到返回空串
QString findUserImage(const QStringList &relativeNames);

// 资源根目录候选列表：从可执行文件所在目录逐级向上查找 resources/，兼容影子构建目录
QStringList resourceRoots();

// 主界面背景：晋剧舞台 + 山西山水/古建融合
QPixmap mainBackground(const QSize &size);

// 答题弹窗背景：晋剧幕布风格
QPixmap quizBackground(const QSize &size);

// 普通地块的地域风貌图（key 见 createBoard()，如 "jinci"）
QPixmap placeImage(const QString &key, const QString &title, const QSize &size);

// 图片识别题的戏曲元素插画（key 如 "mask_red"）
QPixmap quizImage(const QString &key, const QSize &size);

// 棋子/装饰用的小图标（圆形棋子上的脸谱纹样）
void paintTokenFace(QPainter *painter, const QRectF &rect, const QColor &color);

// 4:3 成品尺寸（1600x1200），用于导出或制作报告插图
QSize artworkCanvasSize();

// 清空缓存（更换素材后调用）
void clearCache();

} // namespace Artwork

#endif // ARTWORK_H
