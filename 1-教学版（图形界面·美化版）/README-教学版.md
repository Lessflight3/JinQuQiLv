# 《晋曲棋旅》教学简化版

这是给**刚学 C++ 的同学**看的版本：只用了最基础的语法和最简单的 Qt 控件，
没有自绘棋盘、没有音频、没有图片、没有网络。功能比完整版少，但代码很好懂、好改。

## 一、和完整版的区别

| 项目 | 完整版 | **教学简化版** |
| --- | --- | --- |
| 代码量 | 约 4900 行（26 个文件） | **约 520 行（7 个文件）** |
| 棋盘 | 24 格，QPainter 自绘 | **16 格，用 16 个按钮摆成 5×5 外圈** |
| 音频 / 图片 | 35 个音频 + 38 张图片 | 没有（不影响学逻辑） |
| 玩家数 | 2~4 人 | 2 人（够看懂原理） |
| 题库 | 86 题 | 5 题 |
| 用到的知识 | 分层架构、容器、序列化、多媒体… | **类、数组、循环、if、随机数、信号槽** |

## 二、文件说明（一共 7 个文件）

| 文件 | 行数 | 作用 |
| --- | --- | --- |
| `player.h` | 48 | **玩家类**：名字、金钱、位置、几块地、是否破产 |
| `land.h` | 49 | **地块类**：名字、类型、价格、租金、被谁买走 |
| `quiz.h` | 17 | **题目结构体**：题干 + 四个选项 + 正确答案 |
| `mainwindow.h` | 44 | 主窗口类的声明（有哪些变量、哪些函数） |
| `mainwindow.cpp` | 345 | **主窗口类的实现（核心，注释最多）** |
| `main.cpp` | 12 | 程序入口 |
| `JinQuQiLvSimple.pro` | 14 | 工程文件（Qt Creator 打开它） |

## 三、知识点对照表（在哪个文件里学到什么）

| 知识点 | 位置 |
| --- | --- |
| 类的定义、成员变量、成员函数 | `player.h`、`land.h` |
| 构造函数、默认构造函数 | `player.h`（两个 Player 构造函数）、`land.h` |
| 枚举 `enum` | `land.h` 的 `LandType` |
| 结构体 `struct` | `quiz.h` |
| 数组与下标 | `mainwindow.h`（`Land m_land[16]`）、`mainwindow.cpp` 的 `kRow[]`/`kCol[]` |
| 循环 `for` | `mainwindow.cpp` 的 `initUI()`、`refresh()` |
| 判断 `if / else if / else` | `doLandEvent()`（落地事件的三种情况） |
| 取模运算 `%`（让位置循环） | `rollDice()` 里的 `(pos + dice) % 16` |
| 随机数 | `randomInt()` 用了 `QRandomGenerator` |
| 引用 `&`（改别名=改本体） | `rollDice()`、`doLandEvent()` 里的 `Player &cur = ...` |
| 继承与 `Q_OBJECT` | `mainwindow.h` |
| **信号与槽** | `initUI()` 最后的 `connect(...)` |
| 常用控件 | `initUI()`：`QLabel`、`QPushButton`、`QGridLayout`、`QTextEdit` |
| 弹窗 | `QMessageBox::question`（买地）、`QInputDialog::getItem`（答题） |
| 字符串拼接 | `refresh()` 里的 `QString::number(...)` 和 `+` |
| 简单美化 | `refresh()` 里的 `setStyleSheet("background:#ffe1e1;")` |

## 四、怎么运行

**方法一（推荐，能看代码）**：Qt Creator → 打开 `JinQuQiLvSimple.pro` → 选 MinGW 套件 → 按 `Ctrl+R`

**方法二（只想看看效果）**：双击 `直接运行\JinQuQiLvSimple.exe`

## 五、游戏规则（简化版）

1. 两位玩家各 **1000 元**，都从 0 号格（起点）开始
2. 点"掷骰子"：随机 1~6 点，顺时针走
3. 走到不同格子的效果：
   - **起点**：领 100 元
   - **无人买的普通地块**：弹窗问你要不要买（钱够才能买）
   - **自己的地块**：什么也不发生
   - **别人的地块**：付租金给地主
   - **晋剧答题格**：抽一道晋剧题，答对 +100 元，答错 −100 元
4. 钱变成负数就**破产出局**；对方获胜

## 六、想改点什么？（都很好改）

| 想改什么 | 改哪里 |
| --- | --- |
| 棋盘上的地块名字/价格/租金 | `initBoard()` 里那一串 `m_land[..] = Land("名字", TYPE_NORMAL, 价格, 租金);` |
| 初始金钱 | `initBoard()` 最后的 `Player("玩家一", 1000)`，改 1000 |
| 晋剧题目 | 文件开头的 `g_quiz[5]` 数组（照着格式加题、改答案编号） |
| 骰子最大点数 | `rollDice()` 里的 `randomInt(1, 6)` |
| 答题奖惩金额 | `askQuiz()` 里的 `addMoney(100)` / `addMoney(-100)` |
| 格子颜色 | `refresh()` 里那几行 `setStyleSheet("background:#...")` |
| 格子数量 | 要把 `CELL_COUNT`、`kRow[]`、`kCol[]`、`initBoard()` 一起改（建议先别动） |

## 七、几个常见问题

| 问题 | 说明 |
| --- | --- |
| 为什么不用 `std::vector`？ | 用固定数组 `Land m_land[16]` 更好理解；等你熟悉了再换成 `vector` |
| 为什么把逻辑都写在界面类里？ | 教学版优先"看得懂"，所以不做分层；完整版才拆成 model / ui |
| 文件名为什么是英文？ | qmake 遇到中文文件名会报错（这是踩过的坑），所以文件名用英文、内容用中文 |

---

完整版（24 格、86 题、音频图片、自绘棋盘）在 `C:\Users\lenovo\Desktop\The song and journey of Jin\`，
教学版就是你现在看的这个目录。两份互不影响，可以对比着看。
