#ifndef LANDDIALOG_H
#define LANDDIALOG_H

#include <QDialog>
#include <QString>
#include <QStringList>

// 地块弹窗：普通地块落地时显示该地标的地域风貌图 + 价格信息 + 购买按钮
class LandDialog : public QDialog
{
    Q_OBJECT

public:
    LandDialog(const QString &title, const QString &subtitle, const QString &landKey,
               const QStringList &infoLines, const QString &acceptText, QWidget *parent = nullptr);

    // 询问是否购买：返回 true 表示玩家选择购买
    static bool askBuy(QWidget *parent, const QString &landName, const QString &landKey, int price,
                       int rent, int playerMoney, const QString &playerName);

    // 只做信息展示（踩到自己的地，或观看地块资料）
    static void showInfo(QWidget *parent, const QString &title, const QString &subtitle,
                         const QString &landKey, const QStringList &infoLines);

protected:
    void paintEvent(QPaintEvent *event) override;
};

#endif // LANDDIALOG_H
