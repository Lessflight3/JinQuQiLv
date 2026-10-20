#include "ui/mainwindow.h"
#include "ui/artwork.h"

#include <QApplication>
#include <QDir>
#include <QFont>
#include <QStringList>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("晋曲棋旅"));
    app.setApplicationDisplayName(QStringLiteral("晋曲棋旅 — 晋剧文化主题大富翁"));
    app.setOrganizationName(QStringLiteral("课程设计"));

    // 中文界面字体（缺失时 Qt 自动回退）
    QFont font = app.font();
    font.setFamily(QStringLiteral("Microsoft YaHei"));
    font.setPointSize(10);
    app.setFont(font);

    // 命令行参数：
    //   --players N   跳过开局对话框，直接以 N 名玩家开局（演示/截图用）
    //   --selfcheck   启动后自动检查界面文字是否放得下，结果打印到控制台
    //   --no-music    不自动播放背景音乐
    int autoPlayers = 0;
    bool selfCheck = false;
    bool noMusic = false;
    QString exportArtDir;
    const QStringList args = QCoreApplication::arguments();
    for (int i = 1; i < args.size(); ++i) {
        const QString &a = args.at(i);
        if (a == QStringLiteral("--players") && i + 1 < args.size())
            autoPlayers = args.at(++i).toInt();
        else if (a.startsWith(QStringLiteral("--players=")))
            autoPlayers = a.section(QLatin1Char('='), 1).toInt();
        else if (a == QStringLiteral("--selfcheck"))
            selfCheck = true;
        else if (a == QStringLiteral("--no-music"))
            noMusic = true;
        else if (a == QStringLiteral("--export-art") && i + 1 < args.size())
            exportArtDir = args.at(++i);
    }

    // --export-art <目录>：把两张 4:3 背景图导出为 PNG（不打开窗口）
    if (!exportArtDir.isEmpty()) {
        QDir().mkpath(exportArtDir);
        const QSize canvas = Artwork::artworkCanvasSize();
        const bool a1 = Artwork::mainBackground(canvas).save(exportArtDir + "/main_bg_4x3.png");
        const bool a2 = Artwork::quizBackground(canvas).save(exportArtDir + "/quiz_bg_4x3.png");
        return (a1 && a2) ? 0 : 1;
    }

    MainWindow window(nullptr, autoPlayers, selfCheck, noMusic);
    window.show();
    return app.exec();
}
