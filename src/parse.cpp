#include "head.h"

static bool equal(Token *tok, const char *str)
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

static int getNumber(Token *tok)
{
  if (tok->Kind != TK_NUM)
    error("expect a number");
  return tok->Val;
}

// 跳过指定的Str
static Token *skip(Token *Tok, const char *Str)
{
  if (!equal(Tok, Str))
    error("expect '%s'", Str);
  return Tok->Next;
}

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

// 新建一个单叉树
static Node *newUnary(NodeKind Kind, Node *Expr)
{
  Node *Nd = newNode(Kind);
  Nd->LHS = Expr;
  return Nd;
}

// 新建一个数字节点
static Node *newNum(int Val)
{
  Node *Nd = newNode(ND_NUM);
  Nd->Val = Val;
  return Nd;
}
// program = stmt*
// stmt = exprStmt
// exprStmt = expr ";"
// expr = assign
// assign = equality ("=" assign)?
// equality = relational ("==" relational | "!=" relational)*
// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
// add = mul ("+" mul | "-" mul)*
// expr = mul ("+" mul | "-" mul)*
// mul = primary ("*" primary | "/" primary)*
// primary = "(" expr ")" | num
static Node *exprStmt(Token **Rest, Token *Tok);
static Node *expr(Token **Rest, Token *Tok);
static Node *equality(Token **Rest, Token *Tok);
static Node *relational(Token **Rest, Token *Tok);
static Node *add(Token **Rest, Token *Tok);
static Node *mul(Token **Rest, Token *Tok);
static Node *primary(Token **Rest, Token *Tok);
static Node *assign(Token **Rest, Token *Tok);

// 新变量
static Node *newVarNode(char Name)
{
  Node *Nd = newNode(ND_VAR);
  Nd->Name = Name;
  return Nd;
}

// 解析语句
// stmt = exprStmt
static Node *stmt(Token **Rest, Token *Tok) { return exprStmt(Rest, Tok); }

// 解析表达式语句
// exprStmt = expr ";"
static Node *exprStmt(Token **Rest, Token *Tok)
{
  Node *Nd = newUnary(ND_EXPR_STMT, expr(&Tok, Tok));
  *Rest = skip(Tok, ";");
  return Nd;
}

// 解析表达式
// expr = assign
static Node *expr(Token **Rest, Token *Tok) { return assign(Rest, Tok); }

// 解析赋值
// assign = equality ("=" assign)?
static Node *assign(Token **Rest, Token *Tok)
{
  // equality
  Node *Nd = equality(&Tok, Tok);

  // 可能存在递归赋值，如a=b=1
  // ("=" assign)?
  if (equal(Tok, "="))
    Nd = newBinary(ND_ASSIGN, Nd, assign(&Tok, Tok->Next));
  *Rest = Tok;
  return Nd;
}
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

// 解析括号、数字、变量
// primary = "(" expr ")" | ident|num
static Node *primary(Token **Rest, Token *Tok)
{
  // "(" expr ")"
  if (equal(Tok, "("))
  {
    Node *Nd = expr(&Tok, Tok->Next);
    *Rest = skip(Tok, ")");
    return Nd;
  }
  // ident
  if (Tok->Kind == TK_IDENT)
  {
    Node *Nd = newVarNode(*Tok->Loc);
    *Rest = Tok->Next;
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

// 语法解析入口函数
// program = stmt*
Node *parse(Token *Tok)
{
  // 这里使用了和词法分析类似的单向链表结构
  Node Head = {};
  Node *Cur = &Head;
  // stmt*
  while (Tok->Kind != TK_EOF)
  {
    Cur->Next = stmt(&Tok, Tok);
    Cur = Cur->Next;
  }
  return Head.Next;
}