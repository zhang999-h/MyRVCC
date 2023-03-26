#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <string.h>
// 为每个终结符都设置种类来表示
typedef enum
{
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

// 输出错误信息
// static文件内可以访问的函数
// Fmt为传入的字符串， ... 为可变参数，表示Fmt后面所有的参数
static void error(char *Fmt, ...)
{
  // 定义一个va_list变量
  va_list VA;
  // VA获取Fmt后面的所有参数
  va_start(VA, Fmt);
  // vfprintf可以输出va_list类型的参数
  vfprintf(stderr, Fmt, VA);
  // 在结尾加上一个换行符
  fprintf(stderr, "\n");
  // 清除VA
  va_end(VA);
  // 终止程序
  exit(1);
}

static bool equal(Token *tok, char *str)
{
  if (memcmp(tok->Loc, str, tok->Len) == 0 && strlen(str) == tok->Len)
  {
    return true;
  }
  else
  {
    return false;
  }
}

Token *newToken(TokenKind kind, char *Start, char *End)
{
  Token *token = (Token *)malloc(sizeof(Token));
  // Token *token=new Token();
  token->Kind = kind;
  token->Loc = Start;
  token->Len = End - Start;
  return token;
}

// 判断Str是否以SubStr开头
static bool startsWith(char *Str, char *SubStr)
{
  // 比较LHS和RHS的N个字符是否相等
  return strncmp(Str, SubStr, strlen(SubStr)) == 0;
}

// 读取操作符
static int readPunct(char *Ptr)
{
  // 判断2字节的操作符
  if (startsWith(Ptr, "==") || startsWith(Ptr, "!=") || startsWith(Ptr, "<=") ||
      startsWith(Ptr, ">="))
    return 2;

  // 判断1字节的操作符
  return ispunct(*Ptr) ? 1 : 0;
}

static Token *tokenize(char *P)
{
  Token *Head = new Token();
  Token *Cur = Head;
  while (*P)
  {
    // 跳过所有空白符如：空格、回车
    if (isspace(*P))
    {
      ++P;
      continue;
    }

    // 解析数字
    if (isdigit(*P))
    {
      // 初始化，类似于C++的构造函数
      // 我们不使用Head来存储信息，仅用来表示链表入口，这样每次都是存储在Cur->Next
      // 否则下述操作将使第一个Token的地址不在Head中。
      Cur->Next = newToken(TK_NUM, P, P);
      // 指针前进
      Cur = Cur->Next;
      const char *OldPtr = P;
      Cur->Val = strtoul(P, &P, 10);
      Cur->Len = P - OldPtr;
      continue;
    }

    int PunctLen = readPunct(P);
    if (PunctLen)
    {
      Cur->Next = newToken(TK_PUNCT, P, P + PunctLen);
      Cur = Cur->Next;
      P += PunctLen;
      continue;
    }

    // 处理无法识别的字符
    error("invalid token: %c", *P);
  }

  // 解析结束，增加一个EOF，表示终止符。
  Cur->Next = newToken(TK_EOF, P, P);
  // Head无内容，所以直接返回Next
  return Head->Next;
}

static int getNumber(Token *tok)
{
  if (tok->Kind != TK_NUM)
    error("expect a number");
  return tok->Val;
}

// 跳过指定的Str
static Token *skip(Token *Tok, char *Str)
{
  if (!equal(Tok, Str))
    error("expect '%s'", Str);
  return Tok->Next;
}

//
// 生成AST（抽象语法树），语法解析
//

// AST的节点种类
typedef enum
{
  ND_ADD, // +
  ND_SUB, // -
  ND_MUL, // *
  ND_DIV, // /
  ND_NEG, // 负号-
  ND_EQ,  // ==
  ND_NE,  // !=
  ND_LT,  // <
  ND_LE,  // <=
  ND_NUM, // 整形
} NodeKind;

// AST中二叉树节点
typedef struct Node Node;
struct Node
{
  NodeKind Kind; // 节点种类
  Node *LHS;     // 左部，left-hand side
  Node *RHS;     // 右部，right-hand side
  int Val;       // 存储ND_NUM种类的值
};

// 新建一个节点
static Node *newNode(NodeKind Kind)
{
  Node *Nd = (Node *)calloc(1, sizeof(Node));
  Nd->Kind = Kind;
  return Nd;
}

// 新建一个二叉树节点
static Node *newBinary(NodeKind Kind, Node *LHS, Node *RHS)
{
  Node *Nd = newNode(Kind);
  Nd->LHS = LHS;
  Nd->RHS = RHS;
  return Nd;
}

// 新建一个数字节点
static Node *newNum(int Val)
{
  Node *Nd = newNode(ND_NUM);
  Nd->Val = Val;
  return Nd;
}

// expr = equality
// equality = relational ("==" relational | "!=" relational)*
// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
// add = mul ("+" mul | "-" mul)*
// expr = mul ("+" mul | "-" mul)*
// mul = primary ("*" primary | "/" primary)*
// primary = "(" expr ")" | num
static Node *expr(Token **Rest, Token *Tok);
static Node *equality(Token **Rest, Token *Tok);
static Node *relational(Token **Rest, Token *Tok);
static Node *add(Token **Rest, Token *Tok);
static Node *mul(Token **Rest, Token *Tok);
static Node *primary(Token **Rest, Token *Tok);

// 解析表达式
// expr = equality
static Node *expr(Token **Rest, Token *Tok) { return equality(Rest, Tok); }

// 解析相等性
// equality = relational ("==" relational | "!=" relational)*
static Node *equality(Token **Rest, Token *Tok)
{
  // relational
  Node *Nd = relational(&Tok, Tok);

  // ("==" relational | "!=" relational)*
  while (true)
  {
    // "==" relational
    if (equal(Tok, "=="))
    {
      Nd = newBinary(ND_EQ, Nd, relational(&Tok, Tok->Next));
      continue;
    }

    // "!=" relational
    if (equal(Tok, "!="))
    {
      Nd = newBinary(ND_NE, Nd, relational(&Tok, Tok->Next));
      continue;
    }

    *Rest = Tok;
    return Nd;
  }
}

// 解析比较关系
// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
static Node *relational(Token **Rest, Token *Tok)
{
  // add
  Node *Nd = add(&Tok, Tok);

  // ("<" add | "<=" add | ">" add | ">=" add)*
  while (true)
  {
    // "<" add
    if (equal(Tok, "<"))
    {
      Nd = newBinary(ND_LT, Nd, add(&Tok, Tok->Next));
      continue;
    }

    // "<=" add
    if (equal(Tok, "<="))
    {
      Nd = newBinary(ND_LE, Nd, add(&Tok, Tok->Next));
      continue;
    }

    // ">" add
    // X>Y等价于Y<X
    if (equal(Tok, ">"))
    {
      Nd = newBinary(ND_LT, add(&Tok, Tok->Next), Nd);
      continue;
    }

    // ">=" add
    // X>=Y等价于Y<=X
    if (equal(Tok, ">="))
    {
      Nd = newBinary(ND_LE, add(&Tok, Tok->Next), Nd);
      continue;
    }

    *Rest = Tok;
    return Nd;
  }
}

// 解析加减
// add = mul ("+" mul | "-" mul)*
static Node *add(Token **Rest, Token *Tok)
{
  // mul
  Node *Nd = mul(&Tok, Tok);

  // ("+" mul | "-" mul)*
  while (true)
  {
    // "+" mul
    if (equal(Tok, "+"))
    {
      Nd = newBinary(ND_ADD, Nd, mul(&Tok, Tok->Next));
      //
      continue;
    }

    // "-" mul
    if (equal(Tok, "-"))
    {
      Nd = newBinary(ND_SUB, Nd, mul(&Tok, Tok->Next));
      continue;
    }

    *Rest = Tok;
    return Nd;
  }
}

// 解析乘除
// mul = primary ("*" primary | "/" primary)*
static Node *mul(Token **Rest, Token *Tok)
{
  // primary
  Node *Nd = primary(&Tok, Tok);

  // ("*" primary | "/" primary)*
  while (true)
  {
    // "*" primary
    if (equal(Tok, "*"))
    {
      Nd = newBinary(ND_MUL, Nd, primary(&Tok, Tok->Next));
      continue;
    }

    // "/" primary
    if (equal(Tok, "/"))
    {
      Nd = newBinary(ND_DIV, Nd, primary(&Tok, Tok->Next));
      continue;
    }

    *Rest = Tok;
    return Nd;
  }
}

// 解析括号、数字
// primary = "(" expr ")" | num
static Node *primary(Token **Rest, Token *Tok)
{
  // "(" expr ")"
  if (equal(Tok, "("))
  {
    Node *Nd = expr(&Tok, Tok->Next);
    *Rest = skip(Tok, ")");
    return Nd;
  }

  // num
  if (Tok->Kind == TK_NUM)
  {
    Node *Nd = newNum(Tok->Val);
    *Rest = Tok->Next;
    return Nd;
  }

  error("expected an expression");
  return NULL;
}

//
// 语义分析与代码生成
//

// 记录栈深度
static int Depth;

// 压栈，将结果临时压入栈中备用
// sp为栈指针，栈反向向下增长，64位下，8个字节为一个单位，所以sp-8
// 当前栈指针的地址就是sp，将a0的值压入栈
// 不使用寄存器存储的原因是因为需要存储的值的数量是变化的。
static void push(void)
{
  printf("  addi sp, sp, -8\n");
  printf("  sd a0, 0(sp)\n");
  Depth++;
}

// 弹栈，将sp指向的地址的值，弹出到a1
static void pop(char *Reg)
{
  printf("  ld %s, 0(sp)\n", Reg);
  printf("  addi sp, sp, 8\n");
  Depth--;
}

// 生成表达式
static void genExpr(Node *Nd)
{
  // 加载数字到a0
  if (Nd->Kind == ND_NUM)
  {
    printf("  li a0, %d\n", Nd->Val);
    return;
  }

  // 递归到最右节点
  genExpr(Nd->RHS);
  // 将结果压入栈
  push();
  // 递归到左节点
  genExpr(Nd->LHS);
  // 将结果弹栈到a1
  pop("a1");

  // 生成各个二叉树节点
  switch (Nd->Kind)
  {
  case ND_ADD: // + a0=a0+a1
    printf("  add a0, a0, a1\n");
    return;
  case ND_SUB: // - a0=a0-a1
    printf("  sub a0, a0, a1\n");
    return;
  case ND_MUL: // * a0=a0*a1
    printf("  mul a0, a0, a1\n");
    return;
  case ND_DIV: // / a0=a0/a1
    printf("  div a0, a0, a1\n");
    return;
  case ND_EQ:
  case ND_NE:
    // a0=a0^a1，异或指令
    printf("  xor a0, a0, a1\n");

    if (Nd->Kind == ND_EQ)
      // a0==a1
      // a0=a0^a1, sltiu a0, a0, 1
      // 等于0则置1
      printf("  seqz a0, a0\n");
    else
      // a0!=a1
      // a0=a0^a1, sltu a0, x0, a0
      // 不等于0则置1
      printf("  snez a0, a0\n");
    return;
  case ND_LT:
    printf("  slt a0, a0, a1\n");
    return;
  case ND_LE:
    // a0<=a1等价于  !(a1<a0)
    // a0=a1<a0, a0=a0^1
    printf("  slt a0, a1, a0\n");
    printf("  xori a0, a0, 1\n");
    return;
  default:
    break;
  }

  error("invalid expression");
}

int main(int Argc, char **Argv)
{
  if (Argc != 2)
  {
    // 异常处理，提示参数数量不对。
    // fprintf，格式化文件输出，往文件内写入字符串
    // stderr，异常文件（Linux一切皆文件），用于往屏幕显示异常信息
    // %s，字符串
    error("%s: invalid number of arguments", Argv[0]);
  }
  // 解析Argv[1]，生成终结符流
  Token *Tok = tokenize(Argv[1]);
  // 解析终结符流
  Node *Node = expr(&Tok, Tok);

  if (Tok->Kind != TK_EOF)
    error("extra token");

  // 声明一个全局main段，同时也是程序入口段
  printf("  .globl main\n");
  // main段标签
  printf("main:\n");
  // 遍历AST树生成汇编
  genExpr(Node);
  /*
  // li为addi别名指令，加载一个立即数到寄存器中
  // 这里我们将算式分解为 num (op num) (op num)...的形式
  // 所以先将第一个num传入a0
  printf("  li a0, %d\n", getNumber(Tok));
  Tok = Tok->Next;

  // 解析 (op num)
  while (Tok->Kind != TK_EOF)
  {
    if (equal(Tok, "+"))
    {
      Tok = Tok->Next;
      printf("  addi a0, a0, %d\n", getNumber(Tok));
      Tok = Tok->Next;
      continue;
    }

    // 不是+，则判断-
    // 没有subi指令，但是addi支持有符号数，所以直接对num取反
    Tok = skip(Tok, "-");
    printf("  addi a0, a0, -%d\n", getNumber(Tok));
    Tok = Tok->Next;
  }
*/
  // ret为jalr x0, x1, 0别名指令，用于返回子程序
  // 返回的为a0的值
  printf("  ret\n");

  return 0;
  // printf("  .globl main\n");

  // printf("main:\n");

  // printf("  li a0,%d\n",atoi(Argv[1]));

  // printf("  ret\n");
}
