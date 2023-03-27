#include"head.h"

static bool equal(Token *tok,const char *str)
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



// 语法解析入口函数
Node *parse(Token *Tok) {
  Node *Nd = expr(&Tok, Tok);
  if (Tok->Kind != TK_EOF)
    error("extra token");
  return Nd;
}