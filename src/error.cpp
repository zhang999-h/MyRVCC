#include"head.h"
// 去除了static用以在多个文件间访问
// 输出错误信息
// static文件内可以访问的函数
// Fmt为传入的字符串， ... 为可变参数，表示Fmt后面所有的参数
void error(const char* Fmt, ...)
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
// // 输出错误出现的位置，并退出
// static void verrorAt(char *Loc, char *Fmt, va_list VA) {
//   // 先输出源信息
//   fprintf(stderr, "%s\n", CurrentInput);

//   // 输出出错信息
//   // 计算出错的位置，Loc是出错位置的指针，CurrentInput是当前输入的首地址
//   int Pos = Loc - CurrentInput;
//   // 将字符串补齐为Pos位，因为是空字符串，所以填充Pos个空格。
//   fprintf(stderr, "%*s", Pos, "");
//   fprintf(stderr, "^ ");
//   vfprintf(stderr, Fmt, VA);
//   fprintf(stderr, "\n");
//   va_end(VA);
// }
// 输出例如下面的错误，并退出
// foo.c:10: x = y + 1;
//               ^ <错误信息>
static void verrorAt(int LineNo, char* Loc, char* Fmt, va_list VA) {
  // 查找包含loc的行
  char* Line = Loc;
  // Line递减到当前行的最开始的位置
  // Line<CurrentInput, 判断是否读取到文件最开始的位置
  // Line[-1] != '\n'，Line字符串前一个字符是否为换行符（上一行末尾）
  while (CurrentInput < Line && Line[-1] != '\n')
    Line--;

  // End递增到行尾的换行符
  char* End = Loc;
  while (*End != '\n')
    End++;

  // 输出 文件名:错误行
  // Indent记录输出了多少个字符
  int Indent = fprintf(stderr, "%s:%d: ", CurrentFilename, LineNo);
  // 输出Line的行内所有字符（不含换行符）
  fprintf(stderr, "%.*s\n", (int)(End - Line), Line);

  // 计算错误信息位置，在当前行内的偏移量+前面输出了多少个字符
  int Pos = Loc - Line + Indent;
  // 将字符串补齐为Pos位，因为是空字符串，所以填充Pos个空格。
  fprintf(stderr, "%*s", Pos, "");
  fprintf(stderr, "^ ");
  vfprintf(stderr, Fmt, VA);
  fprintf(stderr, "\n");
  va_end(VA);
}

// 字符解析出错
void errorAt(char* Loc, char* Fmt, ...) {
  int LineNo = 1;
  for (char *P = CurrentInput; P < Loc; P++)
    if (*P == '\n')
      LineNo++;
  va_list VA;
  va_start(VA, Fmt);
  verrorAt(LineNo, Loc, Fmt, VA);
  exit(1);
}

// Tok解析出错
void errorTok(Token* Tok, char* Fmt, ...) {
  va_list VA;
  va_start(VA, Fmt);
  verrorAt(Tok->LineNo, Tok->Loc, Fmt, VA);
  exit(1);
}
