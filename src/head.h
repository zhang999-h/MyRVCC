#ifndef ALL_HEAD_H
#define ALL_HEAD_H

//
// 共用头文件，定义了多个文件间共同使用的函数和数据
//
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <string.h>
// 为每个终结符都设置种类来表示
typedef enum
{
  TK_IDENT, // 标记符，可以为变量名、函数名等
  TK_PUNCT, // 操作符如： + -
  TK_NUM,   // 数字
  TK_EOF,   // 文件终止符，即文件的最后
} TokenKind;

// 终结符结构体
typedef struct Token Token;
struct Token
{
  TokenKind Kind; // 种类
  Token *Next;    // 指向下一终结符
  int Val;        // 值
  char *Loc;      // 在解析的字符串内的位置
  int Len;        // 长度
};

// AST的节点种类
typedef enum
{
  ND_ADD,       // +
  ND_SUB,       // -
  ND_MUL,       // *
  ND_DIV,       // /
  ND_NEG,       // 负号-
  ND_EQ,        // ==
  ND_NE,        // !=
  ND_LT,        // <
  ND_LE,        // <=
  ND_NUM,       // 整形
  ND_EXPR_STMT, // 表达式语句
  ND_ASSIGN,    // 赋值
  ND_VAR,       // 变量
} NodeKind;

// AST中二叉树节点
typedef struct Node Node;
struct Node
{
  Node *Next;    // 下一节点，指代下一语句
  NodeKind Kind; // 节点种类
  Node *LHS;     // 左部，left-hand side
  Node *RHS;     // 右部，right-hand side
  char Name;     // 存储ND_VAR的字符串
  int Val;       // 存储ND_NUM种类的值
};

void error(const char *Fmt, ...);

// 词法分析
Token *tokenize(char *P);

// 语法解析入口函数
Node *parse(Token *Tok);

// 代码生成入口函数
void codegen(Node *Nd);

#endif