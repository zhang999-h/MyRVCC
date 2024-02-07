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

extern char *CurrentInput; // 用来存储输入
typedef struct Node Node;
typedef struct Type Type;

// 为每个终结符都设置种类来表示
typedef enum
{
  TK_IDENT,   // 标记符，可以为变量名、函数名等
  TK_PUNCT,   // 操作符如： + -
  TK_NUM,     // 数字
  TK_EOF,     // 文件终止符，即文件的最后
  TK_KEYWORD, // 关键字
  TK_STR,     // 字符串字面量
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
  Type *Ty;       // TK_STR使用
  char *Str;      // 字符串字面量，包括'\0'
};

// 本地变量  或 函数
typedef struct Obj Obj;
struct Obj
{
  Obj *Next;    // 指向下一对象
  char *Name;   // 变量名
  Type *Ty;     // 变量类型
  bool IsLocal; // 是 局部或全局 变量

  // 局部变量
  int Offset; // fp的偏移量
  // 函数 或 全局变量
  bool IsFunction;

  // 函数
  Obj *Params;   // 形参
  Node *Body;    // 函数体
  Obj *Locals;   // 本地变量
  int StackSize; // 栈大小
                 // 全局变量
  char *InitData;
};

// // 函数
// typedef struct Function Function;
// struct Function
// {
//   Node *Body;     // 函数体
//   Obj *Locals;    // 本地变量
//   int StackSize;  // 栈大小
//   Function *Next; // 下一函数
//   char *Name;     // 函数名
//   Obj *Params;    // 形参
// };

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
  ND_FOR,       // "for"或者"while"
  ND_ADDR,      // 取地址 &
  ND_DEREF,     // 解引用 *
  ND_FUNCALL,   // 函数调用
} NodeKind;

// AST中二叉树节点
struct Node
{
  Node *Next;    // 下一节点，指代下一语句
  NodeKind Kind; // 节点种类
  Type *Ty;      // 节点中数据的类型

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

  // 函数调用
  char *FuncName; // 函数名
  Node *Args;     // 函数参数

  Token *Tok; // 节点对应的终结符
};

void error(const char *Fmt, ...);
void errorTok(Token *Tok, char *Fmt, ...);
void errorAt(char *Loc, char *Fmt, ...);
bool equal(Token *tok, const char *str);

//
// 类型系统
//

// 类型种类
typedef enum
{
  TY_CHAR,  // char字符类型
  TY_INT,   // int整型
  TY_PTR,   // 指针
  TY_FUNC,  // 函数
  TY_ARRAY, // 数组
} TypeKind;

struct Type
{
  TypeKind Kind; // 种类
  int Size;      // 大小, sizeof返回的值
  Type *Base;    // 指向的类型

  // 数组
  int ArrayLen; // 数组长度, 元素总个数
  // 变量名
  Token *Name;
  // 函数类型
  Type *ReturnTy; // 函数返回的类型
  Type *Params;   // 形参
  Type *Next;     // 下一类型
};

// 声明全局变量，定义在type.c中。
extern Type TyInt;
extern Type TyChar;

// 判断是否为整型
bool isInteger(Type *TY);
// 为节点内的所有节点添加类型
void addType(Node *Nd);

// 函数类型
Type *funcType(Type *ReturnTy);

Type *pointerTo(Type *Base);

// 复制类型
Type *copyType(Type *Ty);

// 数组类型
Type *arrayOf(Type *Base, int Size);

// 词法分析
Token *tokenize(char *P);

// 语法解析入口函数
Obj *parse(Token *Tok);

// 代码生成入口函数
void codegen(Obj *Prog);

#endif