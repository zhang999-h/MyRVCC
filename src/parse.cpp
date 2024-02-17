#include "head.h"


// 局部和全局变量的域
typedef struct VarScope VarScope;
struct VarScope {
  VarScope* Next; // 下一变量
  char* Name;     // 变量名称
  Obj* Var;       // 对应的变量
};
// 结构体标签和联合体标签的域
typedef struct TagScope TagScope;
struct TagScope {
  TagScope* Next; // 下一标签域
  char* Name;     // 域名称
  Type* Ty;       // 域类型
};
// 表示一个块域
typedef struct Scope Scope;
struct Scope {
  Scope* Next;    // 指向上一级的域
  // C有两个域：变量域，结构体标签域
  VarScope* Vars; // 指向当前域内的变量
  TagScope* Tags; // 指向当前域内的结构体标签
};


Scope SCOPE = {};
// 所有的域的链表
static Scope* Scp = &SCOPE;
// 在解析时，全部的变量实例都被累加到这个列表里。
Obj* Locals;  // 局部变量
Obj* Globals; // 全局变量
// 进入域
static void enterScope(void) {
  Scope* Cur = (Scope*)calloc(1, sizeof(Scope));
  Cur->Next = Scp;
  Scp = Cur;
}
// 结束当前域
static void leaveScope(void) {
  Scp = Scp->Next;
}

// 将变量存入当前的域中
static VarScope* pushScope(char* Name, Obj* Var) {
  VarScope* S = (VarScope*)calloc(1, sizeof(VarScope));
  S->Name = Name;
  S->Var = Var;
  // 后来的在链表头部
  S->Next = Scp->Vars;
  Scp->Vars = S;
  return S;
}

// 通过Token查找标签
static Type* findTag(Token* Tok) {
  for (Scope* S = Scp; S; S = S->Next)
    for (TagScope* S2 = S->Tags; S2; S2 = S2->Next)
      if (equal(Tok, S2->Name))
        return S2->Ty;
  return NULL;
}
static void pushTagScope(Token* Tok, Type* Ty) {
  TagScope* S = (TagScope*)calloc(1, sizeof(TagScope));
  S->Name = strndup(Tok->Loc, Tok->Len);
  S->Ty = Ty;
  S->Next = Scp->Tags;
  Scp->Tags = S;
}
bool equal(Token* tok, const char* str)
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

static int getNumber(Token* tok)
{
  if (tok->Kind != TK_NUM)
    errorTok(tok, "expect a number");
  return tok->Val;
}

// 跳过指定的Str
static Token* skip(Token* Tok, const char* Str)
{
  if (!equal(Tok, Str))
    errorTok(Tok, "expect '%s'", Str);
  return Tok->Next;
}
// 消耗掉指定Token
bool consume(Token** Rest, Token* Tok, char* Str)
{
  // 存在
  if (equal(Tok, Str))
  {
    *Rest = Tok->Next;
    return true;
  }
  // 不存在
  *Rest = Tok;
  return false;
}

// 新建一个节点
static Node* newNode(NodeKind Kind, Token* Tok)
{
  Node* Nd = (Node*)calloc(1, sizeof(Node));
  Nd->Kind = Kind;
  Nd->Tok = Tok;
  return Nd;
}

// 新建一个二叉树节点
static Node* newBinary(NodeKind Kind, Node* LHS, Node* RHS, Token* Tok)
{
  Node* Nd = newNode(Kind, Tok);
  Nd->LHS = LHS;
  Nd->RHS = RHS;
  return Nd;
}

// 新建一个单叉树
static Node* newUnary(NodeKind Kind, Node* Expr, Token* Tok)
{
  Node* Nd = newNode(Kind, Tok);
  Nd->LHS = Expr;
  return Nd;
}

// 新建一个数字节点
static Node* newNum(int Val, Token* Tok)
{
  Node* Nd = newNode(ND_NUM, Tok);
  Nd->Val = Val;
  return Nd;
}
// 通过名称，查找一个变量
static Obj* findVar(Token* Tok)
{
  for (Scope* scope = Scp;scope;scope = scope->Next) {
    for (VarScope* varScope = scope->Vars;varScope;varScope = varScope->Next) {
      if (equal(Tok, varScope->Name)) {
        return varScope->Var;
      }
    }
  }
  return NULL;
}

// 新变量
static Node* newVarNode(Obj* Var, Token* Tok)
{
  Node* Nd = newNode(ND_VAR, Tok);
  Nd->Var = Var;
  return Nd;
}
// 新建变量
static Obj* newVar(char* Name, Type* Ty)
{
  Obj* Var = (Obj*)calloc(1, sizeof(Obj));
  Var->Name = Name; //
  Var->Ty = Ty;     //
  pushScope(Name, Var);
  return Var;
}
// 在链表中新增一个全局变量
static Obj* newGVar(char* Name, Type* Ty)
{
  Obj* Var = newVar(Name, Ty);
  Var->Next = Globals;
  Globals = Var;
  return Var;
}

// 在链表中新增一个局部变量
static Obj* newLVar(char* Name, Type* Ty)
{
  Obj* Var = newVar(Name, Ty);
  Var->IsLocal = true;
  // 将变量插入头部
  Var->Next = Locals;
  Locals = Var;
  return Var;
}
// 新增唯一名称
static char* newUniqueName(void)
{
  static int Id = 0;
  return format(".L..%d", Id++);
}

// 新增匿名全局变量
static Obj* newAnonGVar(Type* Ty) { return newGVar(newUniqueName(), Ty); }

// 新增字符串字面量
static Obj* newStringLiteral(char* Str, Type* Ty)
{
  Obj* Var = newAnonGVar(Ty);
  Var->InitData = Str;
  return Var;
}
// 获取标识符
static char* getIdent(Token* Tok)
{
  if (Tok->Kind != TK_IDENT)
    errorTok(Tok, "expected an identifier");
  return strndup(Tok->Loc, Tok->Len); // 参数 Tok->Loc 指向的字符串 前n个字符
}
// 将形参添加到Locals
static void createParamLVars(Type* Param)
{
  if (Param)
  {
    // 递归到形参最底部
    // 先将最底部的加入Locals中，之后的都逐个加入到顶部，保持顺序不变
    createParamLVars(Param->Next);
    // 添加到Locals中
    newLVar(getIdent(Param->Name), Param);
  }
}

// program = (functionDefinition | globalVariable)*
// functionDefinition = declspec declarator compoundStmt*
// declspec = "int" | "char" | structDecl | unionDecl
// declarator = "*"* ident typeSuffix
// typeSuffix = funcParams| ("[" num "]")* | ε
// funcParams = "(" (param ("," param)*)* ")"
// param = declspec declarator
// compoundStmt = "{"(declaration | stmt)* "}"//!!!!!!!
// declaration =
//    declspec (declarator ("=" expr)? ("," declarator ("=" expr)?)*)? ";"
// stmt = "return" expr ";"
//        | "if" "(" expr ")" stmt ("else" stmt)?
//        | "for" "(" exprStmt expr? ";" expr? ")" stmt
//        | "while" "(" expr ")" stmt
//        | compoundStmt
//        | exprStmt
// exprStmt = expr? ";"
// expr = assign ("," expr)?
// assign = equality ("=" assign)?
// equality = relational ("==" relational | "!=" relational)*
// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
// add = mul ("+" mul | "-" mul)*
// expr = mul ("+" mul | "-" mul)*
// mul = unary ("*" unary | "/" unary)*
// unary = ("+" | "-" | "*" | "&") unary | postfix
// structDecl = structUnionDecl
// unionDecl = structUnionDecl
// structUnionDecl = ident? ("{" structMembers)?
// structMembers = (declspec declarator (","  declarator)* ";")*
// postfix = primary ("[" expr "]" | "." ident)* | "->" ident)*
// primary = "(" "{" stmt+ "}" ")"
//         | "(" expr ")"
//         | "sizeof" unary
//         | ident funcArgs?
//         | str
//         | num
// funcall = ident "(" (assign ("," assign)*)? ")"
static Obj* function(Token** Rest, Token* Tok);
static Node* exprStmt(Token** Rest, Token* Tok);
static Node* expr(Token** Rest, Token* Tok);
static Node* equality(Token** Rest, Token* Tok);
static Node* relational(Token** Rest, Token* Tok);
static Node* add(Token** Rest, Token* Tok);
static Node* mul(Token** Rest, Token* Tok);
static Node* unary(Token** Rest, Token* Tok);
static Node* postfix(Token** Rest, Token* Tok);
static Node* primary(Token** Rest, Token* Tok);
static Node* assign(Token** Rest, Token* Tok);
static Node* compoundStmt(Token** Rest, Token* Tok);
static Type* declspec(Token** Rest, Token* Tok);
static Type* declarator(Token** Rest, Token* Tok, Type* Ty);
static Node* declaration(Token** Rest, Token* Tok);
static Node* funcall(Token** Rest, Token* Tok);
static Type* typeSuffix(Token** Rest, Token* Tok, Type* Ty);
static Type* funParams(Token** Rest, Token* Tok, Type* Ty);
static Type* structDecl(Token** Rest, Token* Tok);
static Type* unionDecl(Token** Rest, Token* Tok);

static Token* globalVariable(Token* Tok, Type* Basety)
{
  while (!equal(Tok, ";"))
  {
    consume(&Tok, Tok, ",");
    Type* Ty = declarator(&Tok, Tok, Basety);
    newGVar(getIdent(Ty->Name), Ty);
  }
  Tok = skip(Tok, ";");
  return Tok;
}
// functionDefinition = declspec declarator "{" compoundStmt*
static Token* function(Token* Tok, Type* BaseTy)
{
  // declarator? ident "(" ")"
  Type* Ty = declarator(&Tok, Tok, BaseTy);

  // 清空局部变量Locals
  Locals = NULL;
  // 函数名在Obj->Var中
  Obj* Fn = newGVar(getIdent(Ty->Name), Ty);
  Fn->IsFunction = true;
  enterScope();
  // 函数参数
  createParamLVars(Ty->Params);
  Fn->Params = Locals;
  // Tok = skip(Tok, "{");
  //  函数体存储语句的AST，Locals存储变量
  Fn->Body = compoundStmt(&Tok, Tok);
  Fn->Locals = Locals;
  leaveScope();
  return Tok;
}

// declspec = "int" | "char" | structDecl  | unionDecl
// declarator specifier
static Type* declspec(Token** Rest, Token* Tok)
{
  // "char"
  if (equal(Tok, "char"))
  {
    *Rest = Tok->Next;
    return &TyChar;
  }
  //"int"
  if (equal(Tok, "int"))
  {
    *Rest = Tok->Next;
    return &TyInt;
  }
  //structDecl
  if (equal(Tok, "struct"))
  {
    return structDecl(Rest, Tok->Next);
  }
  // unionDecl
  if (equal(Tok, "union"))
    return unionDecl(Rest, Tok->Next);

  errorTok(Tok, "typename expected");
  return NULL;
}

// declarator = "*"* ident typeSuffix
static Type* declarator(Token** Rest, Token* Tok, Type* Ty)
{
  // "*"*
  // 构建所有的（多重）指针
  while (consume(&Tok, Tok, "*"))
    Ty = pointerTo(Ty);

  if (Tok->Kind != TK_IDENT)
    errorTok(Tok, "expected a variable name");

  // typeSuffix
  Ty = typeSuffix(Rest, Tok->Next, Ty);

  // ident
  // 变量名
  Ty->Name = Tok;
  //*Rest = Tok;
  return Ty;
}

// structMembers = (declspec declarator (","  declarator)* ";")*
static void structMembers(Token** Rest, Token* Tok, Type* Ty) {
  Member Head = {};
  Member* Cur = &Head;
  while (!equal(Tok, "}")) {
    Type* BaseTy = declspec(&Tok, Tok);
    while (!equal(Tok, ";"))
    {
      // 第1个变量不必匹配 ","
      consume(&Tok, Tok, ",");
      // declarator
      // 声明获取到变量类型，包括变量名
      Type* DeclarTy = declarator(&Tok, Tok, BaseTy);
      Member* mem = (Member*)calloc(1, sizeof(Member));
      mem->Name = DeclarTy->Name;
      mem->Ty = DeclarTy;
      Cur->Next = mem;
      Cur = Cur->Next;
    }
    Tok = skip(Tok, ";");
  }
  *Rest = skip(Tok, "}");
  Ty->Mems = Head.Next;
}
// structUnionDecl = ident? ("{" structMembers)?
static Type* structUnionDecl(Token** Rest, Token* Tok) {
  // 读取结构体标签
  Token* Tag = NULL;
  if (Tok->Kind == TK_IDENT) {
    Tag = Tok;
    Tok = Tok->Next;
  }

  if (Tag && !equal(Tok, "{")) {
    Type* Ty = findTag(Tag);
    if (!Ty)
      errorTok(Tag, "unknown struct type");
    *Rest = Tok;
    return Ty;
  }
  //结构体成员定义
  Tok = skip(Tok, "{");
  Type* Ty = (Type*)calloc(1, sizeof(Type));
  structMembers(Rest, Tok, Ty);
  // 如果有名称就注册结构体类型
  if (Tag)
    pushTagScope(Tag, Ty);
  return Ty;
}

// structDecl = "{" structMembers
static Type* structDecl(Token** Rest, Token* Tok) {
  Type* Ty = structUnionDecl(Rest, Tok);
  Ty->Kind = TY_STRUCT;
  //对齐
  Ty->Align = 1;
  int Offset = 0, Size = 0;
  for (Member* mem = Ty->Mems;mem;mem = mem->Next) {
    Offset = alignTo(Offset, mem->Ty->Align);
    mem->Offset = Offset;
    Offset += mem->Ty->Size;
    if (Ty->Align < mem->Ty->Align)
      Ty->Align = mem->Ty->Align;
  }
  Ty->Size = alignTo(Offset, Ty->Align);

  return Ty;
}

// unionDecl = structUnionDecl
static Type* unionDecl(Token** Rest, Token* Tok) {
  Type* Ty = structUnionDecl(Rest, Tok);
  Ty->Kind = TY_UNION;

  // 联合体需要设置为最大的对齐量与大小，变量偏移量都默认为0
  for (Member* Mem = Ty->Mems; Mem; Mem = Mem->Next) {
    if (Ty->Align < Mem->Ty->Align)
      Ty->Align = Mem->Ty->Align;
    if (Ty->Size < Mem->Ty->Size)
      Ty->Size = Mem->Ty->Size;
  }
  // 将大小对齐
  Ty->Size = alignTo(Ty->Size, Ty->Align);
  return Ty;
}

// 获取结构体成员
static Member* getStructMember(Type* Ty, Token* Tok) {
  for (Member* Mem = Ty->Mems; Mem; Mem = Mem->Next)
    if (Mem->Name->Len == Tok->Len &&
      !strncmp(Mem->Name->Loc, Tok->Loc, Tok->Len))
      return Mem;
  errorTok(Tok, "no such member");
  return NULL;
}

// 构建结构体成员的节点
static Node* structRef(Node* LHS, Token* Tok) {
  //需要先增加Type,因为之前没加过
  addType(LHS);
  if (LHS->Ty->Kind != TY_STRUCT && LHS->Ty->Kind != TY_UNION)
    errorTok(LHS->Tok, "not a struct or union");

  Node* Nd = newUnary(ND_MEMBER, LHS, Tok);
  Nd->Mem = getStructMember(LHS->Ty, Tok);
  return Nd;
}

// typeSuffix = funcParams| ("[" num "]")* | ε
static Type* typeSuffix(Token** Rest, Token* Tok, Type* Ty)
{
  // ("(" funcParams? ")")?
  if (equal(Tok, "("))
  {
    return funParams(Rest, Tok, Ty);
  }
  if (equal(Tok, "["))
  {
    Tok = skip(Tok, "[");
    int len = getNumber(Tok);

    Tok = Tok->Next;
    Tok = skip(Tok, "]");
    Ty = typeSuffix(&Tok, Tok, Ty);
    Ty = arrayOf(Ty, len);
  }
  *Rest = Tok;
  return Ty;
}

// funcParams = "(" (param ("," param)*)* ")"
// param = declspec declarator
static Type* funParams(Token** Rest, Token* Tok, Type* Ty)
{
  Tok = Tok->Next;
  // 存储形参的链表
  Type Head = {};
  Type* Cur = &Head;

  while (!equal(Tok, ")"))
  {
    // funcParams = param ("," param)*
    // param = declspec declarator
    if (Cur != &Head)
      Tok = skip(Tok, ",");
    Type* BaseTy = declspec(&Tok, Tok);
    Type* DeclarTy = declarator(&Tok, Tok, BaseTy);
    // 将类型复制到形参链表一份
    Cur->Next = copyType(DeclarTy);
    Cur = Cur->Next;
  }

  // 封装一个函数节点
  Ty = funcType(Ty);
  // 传递形参
  Ty->Params = Head.Next;
  *Rest = Tok->Next;
  return Ty;
}

// declaration =
//    declspec (declarator ("=" expr)? ("," declarator ("=" expr)?)*)? ";"
static Node* declaration(Token** Rest, Token* Tok)
{
  // declspec
  // 声明的 基础类型
  Type* Basety = declspec(&Tok, Tok);

  Node Head = {};
  Node* Cur = &Head;
  // 对变量声明次数计数
  int I = 0;

  // (declarator ("=" expr)? ("," declarator ("=" expr)?)*)?
  while (!equal(Tok, ";"))
  {
    // 第1个变量不必匹配 ","
    if (I++ > 0)
      Tok = skip(Tok, ",");

    // declarator
    // 声明获取到变量类型，包括变量名
    Type* Ty = declarator(&Tok, Tok, Basety);
    Obj* Var = newLVar(getIdent(Ty->Name), Ty);

    // 如果不存在"="则为变量声明，不需要生成节点，已经存储在Locals中了
    if (!equal(Tok, "="))
      continue;

    // 解析“=”后面的Token
    Node* LHS = newVarNode(Var, Ty->Name);
    // 解析递归赋值语句
    Node* RHS = assign(&Tok, Tok->Next);
    Node* Node = newBinary(ND_ASSIGN, LHS, RHS, Tok);
    // 存放在表达式语句中
    Cur->Next = newUnary(ND_EXPR_STMT, Node, Tok);
    Cur = Cur->Next;
  }

  // 将所有表达式语句，存放在代码块中
  Node* Nd = newNode(ND_BLOCK, Tok);
  Nd->Body = Head.Next;
  *Rest = Tok->Next;
  return Nd;
}

// 解析语句
// stmt = "return" expr ";"
//        | "if" "(" expr ")" stmt ("else" stmt)?
//        | "for" "(" exprStmt expr? ";" expr? ")" stmt
//        | "while" "(" expr ")" stmt
//        | "{" compoundStmt
//        | exprStmt
static Node* stmt(Token** Rest, Token* Tok)
{
  // "return" expr ";"
  if (equal(Tok, "return"))
  {
    // Node *Nd = newUnary(ND_RETURN, expr(&Tok, Tok->Next));
    Node* Nd = newNode(ND_RETURN, Tok);
    Nd->LHS = expr(&Tok, Tok->Next);
    *Rest = skip(Tok, ";");
    return Nd;
  }
  // "{" compoundStmt
  if (equal(Tok, "{"))
    return compoundStmt(Rest, Tok);
  // 解析if语句
  // "if" "(" expr ")" stmt ("else" stmt)?
  if (equal(Tok, "if"))
  {
    Node* Nd = newNode(ND_IF, Tok);
    // "(" expr ")"，条件内语句
    Tok = skip(Tok->Next, "(");
    Nd->Cond = expr(&Tok, Tok);
    Tok = skip(Tok, ")");
    // stmt，符合条件后的语句
    Nd->Then = stmt(&Tok, Tok);
    // ("else" stmt)?，不符合条件后的语句
    if (equal(Tok, "else"))
      Nd->Els = stmt(&Tok, Tok->Next);
    *Rest = Tok;
    return Nd;
  }
  // "for" "(" exprStmt expr? ";" expr? ")" stmt
  if (equal(Tok, "for"))
  {
    Node* Nd = newNode(ND_FOR, Tok);
    Tok = skip(Tok->Next, "(");

    Nd->Init = exprStmt(&Tok, Tok);

    if (!equal(Tok, ";"))
      Nd->Cond = expr(&Tok, Tok);
    Tok = skip(Tok, ";");
    if (!equal(Tok, ")"))
      Nd->Inc = expr(&Tok, Tok);
    Tok = skip(Tok, ")");

    Nd->Then = stmt(Rest, Tok);
    //*Rest=Tok;
    return Nd;
  }
  // "while" "(" expr ")" stmt
  if (equal(Tok, "while"))
  {
    Node* Nd = newNode(ND_FOR, Tok);
    // "("
    Tok = skip(Tok->Next, "(");
    // expr
    Nd->Cond = expr(&Tok, Tok);
    // ")"
    Tok = skip(Tok, ")");
    // stmt
    Nd->Then = stmt(Rest, Tok);
    return Nd;
  }

  // exprStmt
  return exprStmt(Rest, Tok);
}
// 判断是否为类型名
static bool isTypename(Token* Tok)
{
  return equal(Tok, "char") || equal(Tok, "int") || equal(Tok, "struct")
    || equal(Tok, "union");
}

// 解析复合语句
// compoundStmt ="{" (declaration | stmt)* "}"
static Node* compoundStmt(Token** Rest, Token* Tok)
{
  Tok = skip(Tok, "{");
  Node* Nd = newNode(ND_BLOCK, Tok);
  // 进入新的域
  enterScope();
  // 这里使用了和词法分析类似的单向链表结构
  Node Head = {};
  Node* Cur = &Head;
  // (declaration | stmt)* "}"
  while (!equal(Tok, "}"))
  {
    // declaration
    if (isTypename(Tok))
      Cur->Next = declaration(&Tok, Tok);
    // stmt
    else
      Cur->Next = stmt(&Tok, Tok);
    Cur = Cur->Next;
    // 构造完AST后，为节点添加类型信息
    addType(Cur);
  }
  // 结束当前的域
  leaveScope();
  // Nd的Body存储了{}内解析的语句
  Nd->Body = Head.Next;
  *Rest = Tok->Next;
  return Nd;
}

// 解析表达式语句
// exprStmt = expr ";"
static Node* exprStmt(Token** Rest, Token* Tok)
{
  // ";"
  if (equal(Tok, ";"))
  {
    *Rest = Tok->Next;
    return newNode(ND_BLOCK, Tok);
  }

  // expr ";"
  // Node *Nd = newUnary(ND_EXPR_STMT, expr(&Tok, Tok));
  Node* Nd = newNode(ND_EXPR_STMT, Tok);
  Nd->LHS = expr(&Tok, Tok);
  *Rest = skip(Tok, ";");
  return Nd;
}

// 解析表达式
// expr = assign ("," expr)?
static Node* expr(Token** Rest, Token* Tok) {
  Node* Nd = assign(&Tok, Tok);
  // ("=" assign)?
  if (equal(Tok, ","))
  {
    // Nd = newBinary(ND_ASSIGN, Nd, assign(&Tok, Tok->Next));
    return Nd = newBinary(ND_COMMA, Nd, expr(Rest, Tok->Next), Tok);
  }

  *Rest = Tok;
  return Nd;
}

// 解析赋值
// assign = equality ("=" assign)?
static Node* assign(Token** Rest, Token* Tok) {
  // equality
  Node* Nd = equality(&Tok, Tok);

  // 可能存在递归赋值，如a=b=1
  // ("=" assign)?
  if (equal(Tok, "="))
  {
    // Nd = newBinary(ND_ASSIGN, Nd, assign(&Tok, Tok->Next));
    return Nd = newBinary(ND_ASSIGN, Nd, assign(Rest, Tok->Next), Tok);
  }

  *Rest = Tok;
  return Nd;
}
// 解析相等性
// equality = relational ("==" relational | "!=" relational)*
static Node* equality(Token** Rest, Token* Tok) {
  // relational
  Node* Nd = relational(&Tok, Tok);

  // ("==" relational | "!=" relational)*
  while (true)
  {
    Token* Start = Tok;
    // "==" relational
    if (equal(Tok, "=="))
    {
      Nd = newBinary(ND_EQ, Nd, relational(&Tok, Tok->Next), Start);
      continue;
    }

    // "!=" relational
    if (equal(Tok, "!="))
    {
      Nd = newBinary(ND_NE, Nd, relational(&Tok, Tok->Next), Start);
      continue;
    }

    *Rest = Tok;
    return Nd;
  }
}

// 解析比较关系
// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
static Node* relational(Token** Rest, Token* Tok) {
  // add
  Node* Nd = add(&Tok, Tok);

  // ("<" add | "<=" add | ">" add | ">=" add)*
  while (true) {
    Token* Start = Tok;
    // "<" add
    if (equal(Tok, "<"))
    {
      Nd = newBinary(ND_LT, Nd, add(&Tok, Tok->Next), Start);
      continue;
    }

    // "<=" add
    if (equal(Tok, "<="))
    {
      Nd = newBinary(ND_LE, Nd, add(&Tok, Tok->Next), Start);
      continue;
    }

    // ">" add
    // X>Y等价于Y<X
    if (equal(Tok, ">"))
    {
      Nd = newBinary(ND_LT, add(&Tok, Tok->Next), Nd, Start);
      continue;
    }

    // ">=" add
    // X>=Y等价于Y<=X
    if (equal(Tok, ">="))
    {
      Nd = newBinary(ND_LE, add(&Tok, Tok->Next), Nd, Start);
      continue;
    }

    *Rest = Tok;
    return Nd;
  }
}
// 解析各种加法
static Node* newAdd(Node* LHS, Node* RHS, Token* Tok) {
  // 为左右部添加类型
  addType(LHS);
  addType(RHS);

  // num + num
  if (isInteger(LHS->Ty) && isInteger(RHS->Ty))
    return newBinary(ND_ADD, LHS, RHS, Tok);

  // 不能解析 ptr + ptr
  if (LHS->Ty->Base && RHS->Ty->Base)
    errorTok(Tok, "invalid operands");

  // 将 num + ptr 转换为 ptr + num
  if (!LHS->Ty->Base && RHS->Ty->Base)
  {
    Node* Tmp = LHS;
    LHS = RHS;
    RHS = Tmp;
  }

  // ptr + num
  // 指针加法，ptr+1，这里的1不是1个字节，而是1个元素的空间，所以需要 ×基类Size 操作
  RHS = newBinary(ND_MUL, RHS, newNum(LHS->Ty->Base->Size, Tok), Tok);
  return newBinary(ND_ADD, LHS, RHS, Tok);
}

// 解析各种减法
static Node* newSub(Node* LHS, Node* RHS, Token* Tok) {
  // 为左右部添加类型
  addType(LHS);
  addType(RHS);

  // num - num
  if (isInteger(LHS->Ty) && isInteger(RHS->Ty))
    return newBinary(ND_SUB, LHS, RHS, Tok);

  // ptr - num
  if (LHS->Ty->Base && isInteger(RHS->Ty))
  {
    RHS = newBinary(ND_MUL, RHS, newNum(LHS->Ty->Size, Tok), Tok);
    addType(RHS);
    Node* Nd = newBinary(ND_SUB, LHS, RHS, Tok);
    // 节点类型为指针
    Nd->Ty = LHS->Ty;
    return Nd;
  }

  // ptr - ptr，返回两指针间有多少元素
  if (LHS->Ty->Base && RHS->Ty->Base) {
    Node* Nd = newBinary(ND_SUB, LHS, RHS, Tok);
    Nd->Ty = &TyInt;
    return newBinary(ND_DIV, Nd, newNum(LHS->Ty->Base->Size, Tok), Tok);
  }

  errorTok(Tok, "invalid operands");
  return NULL;
}
// 解析加减
// add = mul ("+" mul | "-" mul)*
static Node* add(Token** Rest, Token* Tok) {
  // mul
  Node* Nd = mul(&Tok, Tok);

  // ("+" mul | "-" mul)*
  while (true)
  {
    Token* Start = Tok;
    // "+" mul
    if (equal(Tok, "+"))
    {
      // Nd = newBinary(ND_ADD, Nd, mul(&Tok, Tok->Next), Start);
      Nd = newAdd(Nd, mul(&Tok, Tok->Next), Start);
      continue;
    }

    // "-" mul
    if (equal(Tok, "-"))
    {
      // Nd = newBinary(ND_SUB, Nd, mul(&Tok, Tok->Next), Start);
      Nd = newSub(Nd, mul(&Tok, Tok->Next), Start);
      continue;
    }

    *Rest = Tok;
    return Nd;
  }
}

// 解析乘除
// mul = unary ("*" unary | "/" unary)*
static Node* mul(Token** Rest, Token* Tok) {
  // unary
  Node* Nd = unary(&Tok, Tok);

  // ("*" unary | "/" unary)*
  while (true)
  {
    Token* Start = Tok;
    // "*" unary
    if (equal(Tok, "*"))
    {
      Nd = newBinary(ND_MUL, Nd, unary(&Tok, Tok->Next), Start);
      continue;
    }

    // "/" unary
    if (equal(Tok, "/"))
    {
      Nd = newBinary(ND_DIV, Nd, unary(&Tok, Tok->Next), Start);
      continue;
    }

    *Rest = Tok;
    return Nd;
  }
}

// unary = ("+" | "-" | "*" | "&") unary | postfix
static Node* unary(Token** Rest, Token* Tok) {
  // "+" unary
  if (equal(Tok, "+"))
    return unary(Rest, Tok->Next);
  // "-" unary
  if (equal(Tok, "-"))
    return newUnary(ND_NEG, unary(Rest, Tok->Next), Tok);

  // "&" unary
  if (equal(Tok, "&"))
    return newUnary(ND_ADDR, unary(Rest, Tok->Next), Tok);

  // "*" unary
  if (equal(Tok, "*"))
    return newUnary(ND_DEREF, unary(Rest, Tok->Next), Tok);

  // postfix
  return postfix(Rest, Tok);
}

// postfix = primary ("[" expr "]" | "." ident)* | "->" ident)*
static Node* postfix(Token** Rest, Token* Tok)
{
  // primary
  Node* Nd = primary(&Tok, Tok);

  while (true) {
    if (equal(Tok, "[")) {
      // x[y] 等价于 *(x+y)
      Token* Start = Tok;
      Node* Idx = expr(&Tok, Tok->Next);
      Tok = skip(Tok, "]");
      Nd = newUnary(ND_DEREF, newAdd(Nd, Idx, Start), Start);
      continue;
    }

    // "." ident
    if (equal(Tok, ".")) {
      Nd = structRef(Nd, Tok->Next);
      Tok = Tok->Next->Next;
      continue;
    }

    // "->" ident
    if (equal(Tok, "->")) {
      // x->y 等价于 (*x).y
      Nd = newUnary(ND_DEREF, Nd, Tok);
      Nd = structRef(Nd, Tok->Next);
      Tok = Tok->Next->Next;
    }
    *Rest = Tok;
    return Nd;
  }
}

// 解析括号、数字、变量
// primary = "(" "{" stmt+ "}" ")"
//         | "(" expr ")"
//         | "sizeof" unary
//         | ident funcArgs?
//         | str
//         | num
static Node* primary(Token** Rest, Token* Tok)
{
  // "(" "{" stmt+ "}" ")"
  if (equal(Tok, "(") && equal(Tok->Next, "{"))
  {
    // This is a GNU statement expresssion.
    Node* Nd = newNode(ND_STMT_EXPR, Tok);
    Nd->Body = compoundStmt(&Tok, Tok->Next)->Body;
    *Rest = skip(Tok, ")");
    return Nd;
  }
  // "(" expr ")"
  if (equal(Tok, "("))
  {
    Node* Nd = expr(&Tok, Tok->Next);
    *Rest = skip(Tok, ")");
    return Nd;
  }
  // "sizeof" unary
  if (equal(Tok, "sizeof"))
  {
    Node* Nd = unary(Rest, Tok->Next);
    addType(Nd);
    return newNum(Nd->Ty->Size, Tok);
  }
  // ident args?
  if (Tok->Kind == TK_IDENT)
  {
    // 函数调用
    // args = "(" ")"
    if (equal(Tok->Next, "("))
    {
      // Node *Nd = newNode(ND_FUNCALL, Tok);
      // // ident
      // Nd->FuncName = strndup(Tok->Loc, Tok->Len);
      // *Rest = skip(Tok->Next->Next, ")");
      return funcall(Rest, Tok);
    }
    // ident
    // 查找变量
    Obj* Var = findVar(Tok);
    // 如果变量不存在，就error
    if (!Var)
      errorTok(Tok, "undefined variable");
    *Rest = Tok->Next;
    return newVarNode(Var, Tok);
  }
  // num
  if (Tok->Kind == TK_NUM)
  {
    Node* Nd = newNum(Tok->Val, Tok);
    *Rest = Tok->Next;
    return Nd;
  }
  // str
  if (Tok->Kind == TK_STR)
  {
    Obj* Var = newStringLiteral(Tok->Str, Tok->Ty);
    *Rest = Tok->Next;
    return newVarNode(Var, Tok);
  }

  errorTok(Tok, "expected an expression");
  // error("expected an expression");
  return NULL;
}

// funcall = ident "(" (assign ("," assign)*)? ")"
static Node* funcall(Token** Rest, Token* Tok) {
  Node* Nd = newNode(ND_FUNCALL, Tok);
  // ident
  Nd->FuncName = strndup(Tok->Loc, Tok->Len);
  Tok = skip(Tok->Next, "(");
  Node head = {};
  Node* tmp = &head, args;

  while (!equal(Tok, ")"))
  {
    tmp->Next = assign(&Tok, Tok);
    tmp = tmp->Next;
    consume(&Tok, Tok, ",");
  }
  Nd->Args = head.Next;
  *Rest = skip(Tok, ")");
  return Nd;
}

static bool isFunction(Token* Tok) {
  return (equal(Tok->Next, "("));
}

// 语法解析入口函数
// program = (functionDefinition | globalVariable)*
Obj* parse(Token* Tok) {
  Globals = NULL;

  while (Tok->Kind != TK_EOF) {
    Type* BaseTy = declspec(&Tok, Tok);
    if (isFunction(Tok))
    {
      Tok = function(Tok, BaseTy);
    }
    else
    {
      // 全局变量
      Tok = globalVariable(Tok, BaseTy);
    }
  }

  return Globals;
}
// // program = functionDefinition*
// Function *parse(Token *Tok)
// {
//   // "{"
//   Function Head = {};
//   Function *Cur = &Head;
//   while (Tok->Kind != TK_EOF)
//   {
//     Cur->Next = function(&Tok, Tok);
//     Cur = Cur->Next;
//   }

//   // // 函数体存储语句的AST，Locals存储变量
//   // Function *Prog = (Function *)calloc(1, sizeof(Function));
//   // Prog->Body = compoundStmt(&Tok, Tok);
//   // Prog->Locals = Locals;
//   return Head.Next;
//   ;
// }