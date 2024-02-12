#include "head.h"

char *CurrentInput;

Token *newToken(TokenKind kind, char *Start, char *End)
{
  Token *token = (Token *)malloc(sizeof(Token));
  // Token *token=new Token();
  token->Kind = kind;
  token->Loc = Start;
  token->Len = End - Start;
  return token;
}
// 返回一位十六进制转十进制
// hexDigit = [0-9a-fA-F]
// 16: 0 1 2 3 4 5 6 7 8 9  A  B  C  D  E  F
// 10: 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15
static int fromHex(char C)
{
  if ('0' <= C && C <= '9')
    return C - '0';
  if ('a' <= C && C <= 'f')
    return C - 'a' + 10;
  return C - 'A' + 10;
}
// 读取转义字符
static int readEscapedChar(char *&P)
{
  if (isdigit(*P))
  {
    int res = 0;
    int tmp = 0;
    for (int i = 1; i <= 3 && isdigit(*P); i++, P++)
    {
      tmp = *P - '0';
      if (tmp >= 8 && i == 1)
      {
        errorAt(P, "invalid Octal escape sequence");
      }
      if (tmp >= 8 && i != 1)
      {
        return res;
      }
      if (res * 8 + tmp > 255)
      {
        return res;
      }
      res = res * 8 + tmp;
    }
    return res;
  }
  if (*P == 'x')
  {
    *P++;
    if (!isxdigit(*P) )
      {
        errorAt(P, "invalid Octal escape sequence");
      }
    int res = 0;
    int tmp = 0;
    for (; isxdigit(*P); P++)
    {
      tmp = fromHex(*P);
      if (res * 16 + tmp > 255)
      {
        return res;
      }
      res = res * 16 + tmp;
    }
    return res;
  }
  char ch = *P;
  P++;
  switch (ch)
  {
  case 'a': // 响铃（警报）
    return '\a';
  case 'b': // 退格
    return '\b';
  case 't': // 水平制表符，tab
    return '\t';
  case 'n': // 换行
    return '\n';
  case 'v': // 垂直制表符
    return '\v';
  case 'f': // 换页
    return '\f';
  case 'r': // 回车
    return '\r';
  // 属于GNU C拓展
  case 'e': // 转义符
    return 27;
  default: // 默认将原字符返回
    return ch;
  }
}
// 读取到字符串字面量结尾
static char *stringLiteralEnd(char *P)
{
  // 识别字符串内的所有非"字符
  for (; *P != '"'; ++P)
  { // 遇到换行符和'\0'则报错
    if (*P == '\n' || *P == '\0')
      errorAt(P, "unclosed string literal");
    if (*P == '\\')
    {
      P++;
    }
  }
  return P;
}
// 读取字符串字面量
static Token *readStringLiteral(char *Start)
{
  char *P = Start + 1;
  char *End = stringLiteralEnd(P);
  // 用来存储最大位数的字符串字面量
  char *Buf = (char *)calloc(1, End - Start);
  // 实际的字符位数，一个转义字符为1位
  int Len = 0;

  // 将读取后的结果写入Buf
  for (char *P = Start + 1; P < End;)
  {
    if (*P == '\\')
    {
      Buf[Len++] = readEscapedChar(++P);
    }
    else
    {
      Buf[Len++] = *P;
      P++;
    }
  }
  Token *Tok = newToken(TK_STR, Start, End + 1);
  // 构建 char[] 类型
  Tok->Ty = arrayOf(&TyChar, Len + 1);

  Tok->Str = Buf;
  return Tok;
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
static bool isKeyword(Token *Tok)
{
  // 关键字列表
  const char *Kw[] = {"return", "if", "else", "for", "while", "int", "sizeof", "char"};

  // 遍历关键字列表匹配
  for (int I = 0; I < sizeof(Kw) / sizeof(*Kw); ++I)
  {
    if (equal(Tok, Kw[I]))
      return true;
  }

  return false;
}

// 将名为“return”的终结符转为KEYWORD
static void convertKeywords(Token *Tok)
{
  for (Token *T = Tok; T->Kind != TK_EOF; T = T->Next)
  {
    if (isKeyword(T))
      T->Kind = TK_KEYWORD;
  }
}

Token *tokenize(char *P)
{
  CurrentInput = P;
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
    // 解析字符串字面量
    if (*P == '"')
    {
      Cur->Next = readStringLiteral(P);
      Cur = Cur->Next;
      P += Cur->Len;
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
    if (isIdent1(*P))
    {
      char *Start = P;
      do
      {
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
