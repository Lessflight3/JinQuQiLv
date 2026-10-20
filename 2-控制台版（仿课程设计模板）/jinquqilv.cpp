// ============================================================
//  《晋曲棋旅》——晋剧文化主题大富翁（控制台版）
//  课程设计：C++ 程序设计
//  用法：直接运行本程序，按菜单提示输入数字即可
//  说明：棋盘 24 格，2 位玩家，买地收租、答晋剧题、破产判负
// ============================================================

#include <iostream>     // 标准输入输出（cout / cin）
#include <iomanip>      // 控制输出格式（setw 对齐）
#include <string>       // 字符串类型 string
#include <cstdlib>      // rand() / srand()
#include <ctime>        // time() 取时间做随机种子
#include <fstream>      // 文件读写，用来保存战绩

using namespace std;

// ============================================================
//  全局常量
// ============================================================
const int CELL_COUNT  = 24;    // 棋盘格子总数
const int START_MONEY = 1000;  // 每位玩家的初始金钱
const int QUIZ_COUNT  = 44;    // 题库里的题目数量
const int START_BONUS = 100;   // 停在起点领到的钱
const int QUIZ_RIGHT  = 100;   // 答题答对加的钱
const int QUIZ_WRONG  = 100;   // 答题答错扣的钱

// 地块类型：0=起点，1=普通地块，2=晋剧答题地块
const int TYPE_START  = 0;
const int TYPE_NORMAL = 1;
const int TYPE_QUIZ   = 2;

// ============================================================
//  类：Player（玩家类）
//  描述一位玩家的姓名、金钱、位置、地产数量、是否破产
// ============================================================
class Player
{
private:
    string name;      // 玩家姓名
    int money;        // 现金
    int pos;          // 当前所在格子编号（0~15）
    int landCount;    // 拥有几块地
    bool out;         // 是否破产出局

public:
    // 构造函数：创建一个没有名字的玩家
    Player()
    {
        name = "";
        money = 0;
        pos = 0;
        landCount = 0;
        out = false;
    }

    // 初始化玩家：设置姓名和初始金钱
    void init(string n, int m)
    {
        name = n;
        money = m;
        pos = 0;
        landCount = 0;
        out = false;
    }

    // 获取姓名
    string getName()
    {
        return name;
    }

    // 获取现金
    int getMoney()
    {
        return money;
    }

    // 获取位置
    int getPos()
    {
        return pos;
    }

    // 获取地产数量
    int getLandCount()
    {
        return landCount;
    }

    // 是否已破产
    bool isOut()
    {
        return out;
    }

    // 加钱（参数为负数就是扣钱）
    void addMoney(int delta)
    {
        money = money + delta;
    }

    // 移动到某个格子
    void setPos(int p)
    {
        pos = p;
    }

    // 买地成功，地产数量加一
    void addLand()
    {
        landCount++;
    }

    // 破产出局
    void setOut()
    {
        out = true;
        money = 0;
    }
};

// ============================================================
//  类：Land（地块类）
//  描述一个格子的名称、类型、价格、租金、被谁买走
// ============================================================
class Land
{
private:
    string name;      // 地块名称
    int type;         // 类型：0 起点 / 1 普通 / 2 晋剧答题
    int price;        // 购买价格
    int rent;         // 每次收租金
    int owner;        // 拥有者：-1 表示没人买，0/1 表示玩家编号

public:
    // 构造函数
    Land()
    {
        name = "";
        type = TYPE_NORMAL;
        price = 0;
        rent = 0;
        owner = -1;
    }

    // 设置地块信息
    void init(string n, int t, int p, int r)
    {
        name = n;
        type = t;
        price = p;
        rent = r;
        owner = -1;
    }

    // 获取地块名称
    string getName()
    {
        return name;
    }

    // 获取地块类型
    int getType()
    {
        return type;
    }

    // 获取价格
    int getPrice()
    {
        return price;
    }

    // 获取租金
    int getRent()
    {
        return rent;
    }

    // 获取拥有者
    int getOwner()
    {
        return owner;
    }

    // 是否已被买走
    bool hasOwner()
    {
        return owner >= 0;
    }

    // 设置拥有者
    void setOwner(int playerIndex)
    {
        owner = playerIndex;
    }
};

// ============================================================
//  结构体：Quiz（晋剧题目）
//  一道题包含题干、四个选项和正确答案
// ============================================================
struct Quiz
{
    string question;   // 题干
    string optionA;    // A 选项
    string optionB;    // B 选项
    string optionC;    // C 选项
    string optionD;    // D 选项
    char   answer;     // 正确答案：'A' / 'B' / 'C' / 'D'
};

// 晋剧题库：共 44 道题（常识题 + 唱段题，由 model/questionbank.cpp 自动导出）
static const Quiz g_quizBank[QUIZ_COUNT] = {
    {"晋剧又被称为下列哪一个名称？",
     "中路梆子", "河南梆子", "河北梆子", "蒲剧", 'A'},
    {"晋剧主要流传于我国的哪个省份？",
     "河南省", "山西省", "山东省", "安徽省", 'B'},
    {"下列剧种中，与晋剧同属“山西四大梆子”的是？",
     "豫剧", "评剧", "蒲剧", "秦腔", 'C'},
    {"晋剧的唱腔属于哪一声腔体系？",
     "梆子腔", "高腔", "皮黄腔", "昆腔", 'A'},
    {"晋剧演出中用来击打节奏、名称与剧种同源的乐器是？",
     "梆子", "编钟", "古琴", "笙", 'A'},
    {"晋剧的行当讲究“红、黑、生、旦”和下列哪一个？",
     "丑", "末", "外", "副", 'A'},
    {"晋剧中的“须生”一般扮演哪类角色？",
     "年轻女子", "中年以上、戴胡须的男性", "滑稽小人物", "少年书生", 'B'},
    {"晋剧脸谱中，红色一般用来表现人物怎样的性格？",
     "忠勇正直", "阴险狡诈", "粗鲁莽撞", "阴森恐怖", 'A'},
    {"《打金枝》是下列哪个剧种的代表剧目？",
     "越剧", "黄梅戏", "晋剧", "昆曲", 'C'},
    {"晋剧《打金枝》讲述的故事发生在哪个朝代？",
     "汉代", "唐代", "宋代", "明代", 'B'},
    {"晋剧著名须生表演艺术家、艺名“果子红”的是谁？",
     "丁果仙", "王爱爱", "牛桂英", "程玉英", 'A'},
    {"被誉为“晋剧皇后”的著名青衣演员是？",
     "常香玉", "王爱爱", "袁雪芬", "新凤霞", 'B'},
    {"晋剧演出的念白以哪一地区的话音为主？",
     "粤语", "晋中一带方言", "四川方言", "东北方言", 'B'},
    {"晋剧表演中的“水袖”属于哪一部分的技巧？",
     "头饰", "服饰袖口", "靴子", "脸谱", 'B'},
    {"晋剧传统上也常被外地观众称为？",
     "山西梆子", "绍兴戏", "梆子戏", "淮剧", 'A'},
    {"晋剧的主奏乐器是？",
     "晋胡（呼胡）", "京胡", "马头琴", "唢呐", 'A'},
    {"晋剧大体形成于哪个朝代？",
     "唐代", "宋代", "清代", "民国", 'C'},
    {"晋剧传统演出的舞台陈设通常只有？",
     "一桌二椅", "三张方桌", "五把椅子", "满台布景", 'A'},
    {"晋剧中扮演帝王、高官的戏服主要是？",
     "蟒袍", "靠衣", "褶子", "短打", 'A'},
    {"武将背上插的四面小旗在晋剧中称为？",
     "靠旗", "翎子", "水袖", "髯口", 'A'},
    {"晋剧脸谱中，白色一般用来表现人物怎样的性格？",
     "忠勇正直", "刚直无私", "奸诈多疑", "天真活泼", 'C'},
    {"唱段唱词：“有为王打坐在长安地面……”这是唐代宗的唱段，出自哪一出晋剧？",
     "《打金枝》", "《下河东》", "《算粮登殿》", "《蝴蝶杯》", 'A'},
    {"唱段唱词：“困住了赵王太祖……”这是赵匡胤被困时的唱段，出自哪一出？",
     "《下河东》", "《空城计》", "《渭水河》", "《辕门斩子》", 'A'},
    {"这是青衣唱腔，唱的是王宝钏苦守寒窑十八年、盼丈夫薛平贵归来，出自哪一出？",
     "《算粮登殿》", "《芦花》", "《教子》", "《蝴蝶杯》", 'A'},
    {"唱段唱词：“我本是卧龙岗散淡的人……”这是诸葛亮的唱段，出自哪一出？",
     "《空城计》", "《渭水河》", "《下河东》", "《辕门斩子》", 'A'},
    {"唱段由老仆曹福（衰派老生）演唱，唱他护送曹玉莲千里寻亲、誓死相随，出自哪一出？",
     "《走山》", "《四郎探母》", "《渭水河》", "《教子》", 'A'},
    {"这段唱是杨延辉在异邦思念老母佘太君时所唱，唱词提到十五年未见娘亲，出自哪一出？",
     "《四郎探母》", "《辕门斩子》", "《走山》", "《下河东》", 'A'},
    {"这一段是小生唱腔，唱词提到酒席前见到貂蝉、心生爱慕，出自哪一出？",
     "《小宴》", "《蝴蝶杯》", "《教子》", "《打金枝》", 'A'},
    {"唱段由花脸演唱，内容是不畏权势、依法惩恶、铁面无私的自白，出自哪一出？",
     "《明公断》", "《清风亭》", "《走山》", "《小宴》", 'A'},
    {"郭子仪寿宴上，儿媳升平公主仗着皇家身份不肯拜寿，郭暧一怒之下打了她，唐代宗与皇后出面调解，夫妻和好。这是哪一出？",
     "《打金枝》", "《算粮登殿》", "《教子》", "《明公断》", 'A'},
    {"赵匡胤亲自领兵出征，战事失利被困孤城，最终收服猛将呼延赞扭转战局。这是哪一出？",
     "《下河东》", "《空城计》", "《辕门斩子》", "《渭水河》", 'A'},
    {"周文王在河边访求贤才，遇到用直钩垂钓的姜子牙，几番相请，姜子牙终于答应出山辅佐。这是哪一出？",
     "《渭水河》", "《小宴》", "《下河东》", "《空城计》", 'A'},
    {"薛平贵离家十八年归来，与苦守寒窑的王宝钏相认，随后夫妻一同入朝、受封团圆。这是哪一出？",
     "《算粮登殿》", "《蝴蝶杯》", "《教子》", "《打金枝》", 'A'},
    {"包拯顶住皇亲国戚的压力，为秦香莲伸冤，最终铡了负心的驸马陈世美。这是哪一出？",
     "《明公断》", "《打金枝》", "《空城计》", "《四郎探母》", 'A'},
    {"秦香莲带着儿女进京寻夫，反被丈夫陈世美派人追杀，她告到开封府才得以昭雪。这是哪一出？",
     "《明公断》", "《清风亭》", "《四郎探母》", "《教子》", 'A'},
    {"闵子骞的继母用不保暖的花絮给他做冬衣，父亲发现后要休妻，闵子骞却跪地替继母求情。这是哪一出？",
     "《芦花》", "《蝴蝶杯》", "《教子》", "《走山》", 'A'},
    {"数九寒天，闵子骞冻得拉不住车，父亲扯开他的衣服才发现里面絮的不是棉，一怒之下要写休书。这是哪一出？",
     "《芦花》", "《教子》", "《走山》", "《下河东》", 'A'},
    {"江夏县令之子田玉川与渔家女胡凤莲在江边相遇，两家以一件家传宝杯定下姻缘。这是哪一出？",
     "《蝴蝶杯》", "《清风亭》", "《小宴》", "《芦花》", 'A'},
    {"王春娥纺线织布供前房之子读书，孩子贪玩不肯用功，她当场断机杼示警，终于把孩子培养成才。这是哪一出？",
     "《教子》", "《芦花》", "《算粮登殿》", "《打金枝》", 'A'},
    {"王允设宴请吕布赴席，席间貂蝉相陪，借此挑起吕布与董卓父子反目。这是哪一出？",
     "《小宴》", "《蝴蝶杯》", "《打金枝》", "《明公断》", 'A'},
    {"司马懿兵临城下，诸葛亮大开城门、在城楼上抚琴不动，司马懿疑有伏兵，最终退兵。这是哪一出？",
     "《空城计》", "《渭水河》", "《下河东》", "《辕门斩子》", 'A'},
    {"张元秀夫妇拾得一个弃婴，含辛茹苦抚养十三年；孩子中了状元却不肯相认，二老悲愤而死。这是哪一出？",
     "《清风亭》", "《芦花》", "《教子》", "《明公断》", 'A'},
    {"杨延昭因儿子杨宗保私自招亲、违犯军令，将他绑在帐外要处斩，众将求情无效，最后穆桂英带着援兵赶到才救下丈夫。这是哪一出？",
     "《辕门斩子》", "《四郎探母》", "《空城计》", "《下河东》", 'A'},
    {"杨延辉流落异邦十五年，娶了铁镜公主；得知母亲佘太君押粮到阵前，他在公主帮助下连夜出关，只为见母亲一面。这是哪一出？",
     "《四郎探母》", "《辕门斩子》", "《走山》", "《下河东》", 'A'},
};

// ============================================================
//  类：Game（游戏类）
//  负责棋盘初始化、回合流程、买地收租、答题、胜负判断
//  说明：相当于模板里“图书库类 BDatabase”的角色
// ============================================================
class Game
{
private:
    Land   board[CELL_COUNT];   // 棋盘：24 个格子
    Player players[2];          // 两位玩家
    int    current;             // 当前轮到谁（0 或 1）
    int    roundCount;          // 已经进行了多少个回合
    bool   over;                // 游戏是否结束

public:
    // 构造函数：把棋盘和玩家都准备好
    Game()
    {
        initGame();
    }

    // 初始化：设置 24 个格子的信息，两位玩家各 1000 元
    void initGame()
    {
        board[0].init("起点", TYPE_START, 0, 0);
        board[1].init("晋祠", TYPE_NORMAL, 200, 120);
        board[2].init("脸谱阁", TYPE_QUIZ, 0, 0);
        board[3].init("乔家大院", TYPE_NORMAL, 180, 110);
        board[4].init("水袖台", TYPE_QUIZ, 0, 0);
        board[5].init("平遥古城", TYPE_NORMAL, 160, 95);
        board[6].init("云冈石窟", TYPE_NORMAL, 220, 130);
        board[7].init("梆子腔", TYPE_QUIZ, 0, 0);
        board[8].init("壶口瀑布", TYPE_NORMAL, 240, 145);
        board[9].init("琴师坊", TYPE_QUIZ, 0, 0);
        board[10].init("五台山", TYPE_NORMAL, 200, 120);
        board[11].init("悬空寺", TYPE_NORMAL, 180, 110);
        board[12].init("髯口坊", TYPE_QUIZ, 0, 0);
        board[13].init("应县木塔", TYPE_NORMAL, 160, 95);
        board[14].init("雁门关", TYPE_NORMAL, 260, 155);
        board[15].init("戏服阁", TYPE_QUIZ, 0, 0);
        board[16].init("娘子关", TYPE_NORMAL, 200, 120);
        board[17].init("绵山", TYPE_NORMAL, 220, 130);
        board[18].init("翎子台", TYPE_QUIZ, 0, 0);
        board[19].init("碛口古镇", TYPE_NORMAL, 140, 85);
        board[20].init("梨园堂", TYPE_QUIZ, 0, 0);
        board[21].init("老陈醋坊", TYPE_NORMAL, 140, 85);
        board[22].init("八泉峡", TYPE_NORMAL, 180, 110);
        board[23].init("洪洞大槐树", TYPE_NORMAL, 120, 70);

        players[0].init("玩家一", START_MONEY);
        players[1].init("玩家二", START_MONEY);

        current = 0;
        roundCount = 0;
        over = false;
    }

    // 生成 [low, high] 之间的随机整数
    int randomInt(int low, int high)
    {
        return low + rand() % (high - low + 1);
    }

    // 清屏（不同系统命令不同，这里简单地打一些空行）
    void clearScreen()
    {
        for (int i = 0; i < 3; i++)
            cout << endl;
    }

    // 显示当前棋盘（文字版）
    void showBoard()
    {
        cout << "---------------- 棋盘（共 24 格）----------------" << endl;
        for (int i = 0; i < CELL_COUNT; i++)
        {
            cout << setw(3) << i << "  " << setw(12) << board[i].getName();

            // 第二列：显示价格 / 地主 / 类型
            if (board[i].getType() == TYPE_START)
                cout << setw(10) << "[起点]";
            else if (board[i].getType() == TYPE_QUIZ)
                cout << setw(10) << "[晋剧答题]";
            else if (board[i].hasOwner())
                cout << setw(10) << players[board[i].getOwner()].getName();
            else
                cout << setw(10) << board[i].getPrice();

            // 第三列：谁站在这一格
            cout << "   ";
            for (int p = 0; p < 2; p++)
            {
                if (!players[p].isOut() && players[p].getPos() == i)
                    cout << " ★" << (p + 1);
            }
            cout << endl;
        }
        cout << "-----------------------------------------------" << endl;
    }

    // 显示两位玩家的状态
    void showPlayers()
    {
        cout << "------------ 玩家状态 ------------" << endl;
        for (int i = 0; i < 2; i++)
        {
            cout << setw(10) << players[i].getName()
                 << "  现金：" << setw(6) << players[i].getMoney()
                 << "  地产：" << players[i].getLandCount() << " 块";
            if (players[i].isOut())
                cout << "  （已破产）";
            if (i == current && !over)
                cout << "  ← 该你了";
            cout << endl;
        }
        cout << "----------------------------------" << endl;
    }

    // 抽一道晋剧题让当前玩家回答
    void askQuiz()
    {
        int index = randomInt(0, QUIZ_COUNT - 1);   // 从题库里随机抽一道题
        Quiz q = g_quizBank[index];

        cout << endl << "********** 晋剧文化考校 **********" << endl;
        cout << q.question << endl;
        cout << "  A. " << q.optionA << endl;
        cout << "  B. " << q.optionB << endl;
        cout << "  C. " << q.optionC << endl;
        cout << "  D. " << q.optionD << endl;
        cout << "请选择（输入 A/B/C/D）：";

        char choice;
        cin >> choice;
        // 把输入的小写字母也接受
        if (choice >= 'a' && choice <= 'z')
            choice = choice - 32;

        Player &p = players[current];
        if (choice == q.answer)
        {
            cout << "回答正确！奖励 " << QUIZ_RIGHT << " 元" << endl;
            p.addMoney(QUIZ_RIGHT);
        }
        else
        {
            cout << "回答错误，扣除 " << QUIZ_WRONG << " 元" << endl;
            cout << "正确答案是：" << q.answer << endl;
            p.addMoney(-QUIZ_WRONG);
        }
    }

    // 处理“走到某个格子”后发生的事件
    void landEvent()
    {
        Player &p = players[current];
        Land &land = board[p.getPos()];

        if (land.getType() == TYPE_START)
        {
            cout << "停在起点，领取 " << START_BONUS << " 元" << endl;
            p.addMoney(START_BONUS);
        }
        else if (land.getType() == TYPE_QUIZ)
        {
            askQuiz();      // 晋剧答题地块
        }
        else
        {
            // 普通地块：分三种情况
            if (!land.hasOwner())
            {
                if (p.getMoney() >= land.getPrice())
                {
                    cout << "这块地还没人买，价格 " << land.getPrice() << " 元，要买吗？(y/n)：";
                    char ch;
                    cin >> ch;
                    if (ch == 'y' || ch == 'Y')
                    {
                        p.addMoney(-land.getPrice());
                        p.addLand();
                        land.setOwner(current);
                        cout << "买下【" << land.getName() << "】成功！" << endl;
                    }
                    else
                    {
                        cout << "你没有买这块地。" << endl;
                    }
                }
                else
                {
                    cout << "现金不足，买不起这块地。" << endl;
                }
            }
            else if (land.getOwner() == current)
            {
                cout << "这是你自己的地产，什么也没发生。" << endl;
            }
            else
            {
                int rent = land.getRent();
                p.addMoney(-rent);
                players[land.getOwner()].addMoney(rent);
                cout << "这是【" << players[land.getOwner()].getName() << "】的地，"
                     << "付租金 " << rent << " 元。" << endl;
            }
        }

        // 钱扣成负数就算破产
        if (p.getMoney() < 0)
        {
            cout << p.getName() << " 现金不够，宣告破产出局！" << endl;
            p.setOut();
        }
    }

    // 换下一位玩家（0 和 1 之间切换）
    void nextPlayer()
    {
        current = 1 - current;
    }

    // 判断游戏是否结束；结束返回 true
    bool checkOver()
    {
        int alive = 0;
        int who = 0;
        for (int i = 0; i < 2; i++)
        {
            if (!players[i].isOut())
            {
                alive++;
                who = i;
            }
        }

        if (alive <= 1)
        {
            over = true;
            cout << endl << "========= 游戏结束 =========" << endl;
            if (alive == 1)
                cout << players[who].getName() << " 获胜！" << endl;
            else
                cout << "双方都破产了。" << endl;
            cout << "本局共进行了 " << roundCount << " 个回合。" << endl;
            cout << "============================" << endl;
            saveRecord(who, alive == 1);
            return true;
        }
        return false;
    }

    // 保存战绩到文件（用文件保存数据，方便下次查看）
    void saveRecord(int winner, bool someoneWon)
    {
        ofstream fout("jinquqilv_record.txt", ios::app);   // app 表示追加到文件末尾
        if (!fout)
            return;
        fout << "一局结束：";
        if (someoneWon)
            fout << players[winner].getName() << " 获胜";
        else
            fout << "双方均破产";
        fout << "（共 " << roundCount << " 回合）" << endl;
        fout.close();
    }

    // 一个完整回合：掷骰子 → 移动 → 落地事件 → 换人
    void playOneTurn()
    {
        roundCount++;
        Player &p = players[current];

        cout << endl << "【" << p.getName() << "】的回合" << endl;
        cout << "按回车键掷骰子……";
        cin.ignore();     // 清掉上一次输入的换行符
        cin.get();

        int dice = randomInt(1, 6);      // 掷出 1~6 点
        cout << "掷出 " << dice << " 点！" << endl;

        int newPos = (p.getPos() + dice) % CELL_COUNT;   // 取模让位置在 0~23 循环
        p.setPos(newPos);
        cout << "走到第 " << newPos << " 格【" << board[newPos].getName() << "】" << endl;

        landEvent();      // 处理落地事件

        if (!checkOver())
            nextPlayer();
    }

    // 游戏主循环（一局游戏从这里开始跑）
    void run()
    {
        initGame();
        while (!over)
        {
            clearScreen();
            showBoard();
            showPlayers();
            playOneTurn();
        }

        cout << endl << "按回车键返回主菜单……";
        cin.ignore();
        cin.get();
    }

    // 显示游戏规则
    void showRules()
    {
        clearScreen();
        cout << "============== 《晋曲棋旅》游戏规则 ==============" << endl;
        cout << "1. 两位玩家各有 " << START_MONEY << " 元，都从 0 号格（起点）出发。" << endl;
        cout << "2. 轮流掷骰子（1~6 点），顺时针向前走，走满 24 格回到起点。" << endl;
        cout << "3. 走到不同格子的效果：" << endl;
        cout << "   · 起点：领取 " << START_BONUS << " 元" << endl;
        cout << "   · 无人买的地块：可以选择买下" << endl;
        cout << "   · 自己的地块：什么也不发生" << endl;
        cout << "   · 别人的地块：支付租金给地主" << endl;
        cout << "   · 晋剧答题地块：抽一道晋剧题，答对 +" << QUIZ_RIGHT
             << " 元，答错 -" << QUIZ_WRONG << " 元" << endl;
        cout << "4. 现金变成负数就破产出局，剩下的人获胜。" << endl;
        cout << "=================================================" << endl;
        cout << endl << "按回车键返回主菜单……";
        cin.ignore();
        cin.get();
    }

    // 查看历史战绩（从文件里读出来）
    void showHistory()
    {
        clearScreen();
        cout << "============ 历史战绩（jinquqilv_record.txt）============" << endl;
        ifstream fin("jinquqilv_record.txt");
        if (!fin)
        {
            cout << "还没有战绩记录，先玩一局吧！" << endl;
        }
        else
        {
            string line;
            int n = 0;
            while (getline(fin, line))      // 一行一行读
            {
                n++;
                cout << setw(3) << n << "  " << line << endl;
            }
            fin.close();
            if (n == 0)
                cout << "记录文件是空的。" << endl;
        }
        cout << "========================================================" << endl;
        cout << endl << "按回车键返回主菜单……";
        cin.ignore();
        cin.get();
    }
};

// ============================================================
//  显示主菜单
// ============================================================
void showMenu()
{
    cout << endl;
    cout << "\t\t********************************" << endl;
    cout << "\t\t*     晋 曲 棋 旅              *" << endl;
    cout << "\t\t*     晋剧文化主题大富翁        *" << endl;
    cout << "\t\t********************************" << endl;
    cout << "\t\t*      1. 开始游戏             *" << endl;
    cout << "\t\t*      2. 游戏规则             *" << endl;
    cout << "\t\t*      3. 历史战绩             *" << endl;
    cout << "\t\t*      0. 退出程序             *" << endl;
    cout << "\t\t********************************" << endl;
    cout << "\t\t请选择（0-3）：";
}

// ============================================================
//  主函数：程序入口，菜单循环
// ============================================================
int main()
{
    srand((unsigned)time(NULL));    // 用当前时间做随机种子，保证每局骰子不同

    Game game;                      // 创建一个游戏对象
    char choice;                    // 菜单选择

    while (1)
    {
        showMenu();
        cin >> choice;

        switch (choice)
        {
        case '1':                   // 开始游戏
            game.run();
            break;

        case '2':                   // 游戏规则
            game.showRules();
            break;

        case '3':                   // 历史战绩
            game.showHistory();
            break;

        case '0':                   // 退出
            cout << endl << "感谢游玩《晋曲棋旅》，再见！" << endl;
            return 0;

        default:                    // 输入了别的字符
            cout << "输入有误，请输入 0~3 之间的数字！" << endl;
            break;
        }
    }

    return 0;
}
