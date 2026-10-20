#ifndef QUESTION_H
#define QUESTION_H

#include <array>
#include <string>

// 晋剧题目四大类
enum class QuizCategory
{
    CommonSense = 0, // 第一类：晋剧常识题
    Tune,            // 第二类：听曲牌识别曲牌名
    Aria,            // 第三类：听唱段识别属于哪一出剧目
    Picture          // 第四类：看图片识别戏曲内容
};

const int kQuizCategoryCount = 4;

inline const char *quizCategoryName(QuizCategory c)
{
    switch (c) {
    case QuizCategory::CommonSense:
        return "晋剧常识";
    case QuizCategory::Tune:
        return "听曲牌·识曲牌";
    case QuizCategory::Aria:
        return "听唱段·识剧目";
    case QuizCategory::Picture:
        return "看图片·识戏曲";
    }
    return "晋剧常识";
}

inline const char *quizCategoryShort(QuizCategory c)
{
    switch (c) {
    case QuizCategory::CommonSense:
        return "常识";
    case QuizCategory::Tune:
        return "曲牌";
    case QuizCategory::Aria:
        return "唱段";
    case QuizCategory::Picture:
        return "图片";
    }
    return "常识";
}

// 题目前的操作提示（音频/图片类需要先看/先听）
inline const char *quizCategoryCue(QuizCategory c)
{
    switch (c) {
    case QuizCategory::Tune:
        return "先听音频（若未放置音频文件，可依据题干中的音乐特征作答）";
    case QuizCategory::Aria:
        return "先听唱段（若未放置音频文件，可依据唱词与剧情线索作答）";
    case QuizCategory::Picture:
        return "先看图片，再选择答案";
    default:
        return "阅读题目后选择答案";
    }
}

// 晋剧题目结构体：题干 + A/B/C/D 选项 + 正确答案 + 类别 + 素材键 + 解析
struct Question
{
    std::string text;                   // 题干
    std::array<std::string, 4> options; // A、B、C、D 四个选项
    int answer = 0;                     // 正确答案下标：0=A，1=B，2=C，3=D
    QuizCategory category = QuizCategory::CommonSense;
    std::string mediaKey;               // 曲牌/唱段=音频键；图片题=插画键
    std::string tip;                    // 答案解析（答题后展示，用于传播晋剧知识）
};

#endif // QUESTION_H
