#ifndef QUESTIONBANK_H
#define QUESTIONBANK_H

#include "question.h"

#include <vector>

// 取某一类的全部题目
const std::vector<Question> &questionsOf(QuizCategory category);

// 全部题目（四类顺序拼接）
const std::vector<Question> &allQuestions();

// 某一类的题目数量
int questionCountOf(QuizCategory category);

#endif // QUESTIONBANK_H
