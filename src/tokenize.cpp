#include "head.h"

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
static bool startsWith(char *Str, const char *SubStr)
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
// 判断标记符的首字母规则
// [a-zA-Z_]
static bool isIdent1(char C)
{
  // a-z与A-Z在ASCII中不相连，所以需要分别判断
  return ('a' <= C && C <= 'z') || ('A' <= C && C <= 'Z') || C == '_';
}

// 判断标记符的非首字母的规则
// [a-zA-Z0-9_]
static bool isIdent2(char C)
{
  return isIdent1(C) || ('0' <= C && C <= '9');
}

// 判断是否为关键字
static bool isKeyword(Token *Tok) {
  // 关键字列表
  const char *Kw[] = {"return", "if", "else"};

  // 遍历关键字列表匹配
  for (int I = 0; I < sizeof(Kw) / sizeof(*Kw); ++I) {
    if (equal(Tok, Kw[I]))
      return true;
  }

  return false;
}


// 将名为“return”的终结符转为KEYWORD
static void convertKeywords(Token *Tok) {
  for (Token *T = Tok; T->Kind != TK_EOF; T = T->Next) {
    if (isKeyword(T))
      T->Kind = TK_KEYWORD;
  }
}

Token *tokenize(char *P)
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
    // 解析标记符或关键字
    // [a-zA-Z_][a-zA-Z0-9_]*
    if (isIdent1(*P)) {
      char *Start = P;
      do {
        ++P;
      } while (isIdent2(*P));
      Cur->Next = newToken(TK_IDENT, Start, P);
      Cur = Cur->Next;
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
