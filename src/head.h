#ifndef ALL_HEAD_H
#define ALL_HEAD_H
// 使用POSIX.1标准
// 使用了strndup函数
#define _POSIX_C_SOURCE 200809L
//
// 共用头文件，定义了多个文件间共同使用的函数和数据
//
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <string.h>

extern char *CurrentInput;//用来存储输入

// 为每个终结符都设置种类来表示
typedef enum
{
  TK_IDENT,   // 标记符，可以为变量名、函数名等
  TK_PUNCT,   // 操作符如： + -
  TK_NUM,     // 数字
  TK_EOF,     // 文件终止符，即文件的最后
  TK_KEYWORD, // 关键字
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

typedef struct Node Node;

// 本地变量
typedef struct Obj Obj;
struct Obj
{
  Obj *Next;  // 指向下一对象
  char *Name; // 变量名
  int Offset; // fp的偏移量
};

// 函数
typedef struct Function Function;
struct Function
{
  Node *Body;    // 函数体
  Obj *Locals;   // 本地变量
  int StackSize; // 栈大小
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
  ND_RETURN,    // 返回
  ND_BLOCK,     // { ... }，代码for块
  ND_IF,        // "if"，条件判断
  ND_FOR        // "for"或者"while"
} NodeKind;

// AST中二叉树节点
struct Node
{
  Node *Next;    // 下一节点，指代下一语句
  NodeKind Kind; // 节点种类
  

  Node *LHS; // 左部，left-hand side
  Node *RHS; // 右部，right-hand side
  // char Name;     // 存储ND_VAR的字符串
  Obj *Var; // 存储ND_VAR种类的变量
  int Val;  // 存储ND_NUM种类的值
  // 代码块
  Node *Body;
  // "if"语句
  // "if"语句 或者 "for"语句
  Node *Cond; // 条件内的表达式
  Node *Then; // 符合条件后的语句，for语句的代码块也在Then中
  Node *Els;  // 不符合条件后的语句
  Node *Init; // 初始化语句for的初始化语句
  Node *Inc;  // 递增语句for的递增

  Token *Tok; // 节点对应的终结符
};

void error(const char *Fmt, ...);
void errorTok(Token *Tok, char *Fmt, ...) ;


bool equal(Token *tok, const char *str);

// 词法分析
Token *tokenize(char *P);

// 语法解析入口函数
Function *parse(Token *Tok);

// 代码生成入口函数
void codegen(Function *Prog);

#endif