#ifndef QUIZ_H
#define QUIZ_H

#include <QString>

// ============================================================
//  晋剧题目：用一个简单的结构体数组保存
//  知识点：结构体、字符串数组
// ============================================================
struct Quiz
{
    QString question;  // 题干
    QString a;         // A 选项
    QString b;         // B 选项
    QString c;         // C 选项
    QString d;         // D 选项
    int answer;        // 正确答案：0=A, 1=B, 2=C, 3=D
};

#endif // QUIZ_H
