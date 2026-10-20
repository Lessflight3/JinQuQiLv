#ifndef BOARDWIDGET_H
#define BOARDWIDGET_H

#include "../model/land.h"

#include <QColor>
#include <QList>
#include <QPoint>
#include <QRect>
#include <QString>
#include <QStringList>
#include <QWidget>

#include <vector>

// 棋盘视图：绘制晋剧风格背景 + 7x7 环形棋盘 + 地块信息 + 玩家棋子
class BoardWidget : public QWidget
{
    Q_OBJECT

public:
    // 棋子（一名玩家一个）
    struct Token
    {
        int cell = 0;  // 所在格
        QColor color;  // 棋子颜色
        QString label; // 棋子上的文字（玩家序号）
    };

    explicit BoardWidget(QWidget *parent = nullptr);

    void setLands(const std::vector<Land> &lands);
    void setQuizCategories(const QList<int> &categories); // 每格题目类别（-1 表示非晋剧地块）
    void setEffectiveRents(const QList<int> &rents);      // 计入连片加成后的实际租金
    void setTokens(const QList<Token> &tokens);
    void setPlayerColors(const QList<QColor> &colors);
    void setPlayerNames(const QStringList &names);
    void setCurrentCell(int cellIndex);

    // 棋盘格子的推荐最小边长（供窗口计算最小尺寸）
    static int preferredCellSize() { return 96; }

    // 版面自检：报告每格文字实际可用的字号，便于自动发现“字放不下”的情况
    QStringList layoutReport() const;

    QSize sizeHint() const override { return QSize(720, 720); }
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    // 棋盘中央文字区的显式布局：标题牌、标题、副标题、三行图例，各自独立计算，互不重叠
    struct CenterLayout
    {
        QRect plate;
        QRect title;
        QRect subtitle;
        QList<QRect> legendRows;
    };
    CenterLayout centerLayout() const;
    void drawCenterBlock(QPainter &painter) const;

    int cellSize() const;
    QPoint boardOrigin() const;
    QRect cellRect(int index) const;
    QRect centerRect() const;
    int hitTest(const QPoint &pos) const;
    QColor ownerColor(int owner) const;
    QString categoryBadge(int index) const;
    int rentOf(int index) const;

    std::vector<Land> m_lands;
    QList<int> m_categories;
    QList<int> m_rents;
    QList<Token> m_tokens;
    QList<QColor> m_playerColors;
    QStringList m_playerNames;
    QPixmap m_background;
    int m_currentCell = -1;
    int m_hoverCell = -1;
};

#endif // BOARDWIDGET_H
