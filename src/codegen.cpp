#include "head.h"
//
// 语义分析与代码生成
//
static void genExpr(Node *Nd);

// 记录栈深度
static int Depth;

Function *CurFn;
// 用于函数参数的寄存器们
static char *ArgReg[] = {"a0", "a1", "a2", "a3", "a4", "a5"};

// 代码段计数
static int count(void)
{
    static int I = 1;
    return I++;
}


// 压栈，将结果临时压入栈中备用
// sp为栈指针，栈反向向下增长，64位下，8个字节为一个单位，所以sp-8
// 当前栈指针的地址就是sp，将a0的值压入栈
// 不使用寄存器存储的原因是因为需要存储的值的数量是变化的。
static void push(void)
{
    printf("  # 压栈，将a0的值存入栈顶\n");
    printf("  addi sp, sp, -8\n");
    printf("  sd a0, 0(sp)\n");
    Depth++;
}

// 弹栈，将sp指向的地址的值，弹出到a1
static void pop(const char *Reg)
{
    printf("  # 弹栈，将栈顶的值存入%s\n", Reg);
    printf("  ld %s, 0(sp)\n", Reg);
    printf("  addi sp, sp, 8\n");
    Depth--;
}
// 加载a0指向的值
static void load(Type *Ty) {
  if (Ty->Kind == TY_ARRAY)//数组当变量时需要的是它的地址
    return;

  printf("  # 读取a0中存放的地址，得到的值存入a0\n");
  printf("  ld a0, 0(a0)\n");
}

// 将a0存入栈顶的一个地址存上
static void store(void) {
  pop("a1");
  printf("  # 将a0的值，写入到a1中存放的地址\n");
  printf("  sd a0, 0(a1)\n");
};
// 对齐到Align的整数倍
static int alignTo(int N, int Align)
{
    // (0,Align]返回Align
    return (N + Align - 1) / Align * Align;
}

// 计算给定节点的绝对地址
// 如果报错，说明节点不在内存中
static void genAddr(Node *Nd)
{
    switch (Nd->Kind)
    {
    // 变量
    case ND_VAR:
    {
        // // 偏移量=是两个字母在ASCII码表中的距离加1后乘以8，*8表示每个变量需要八个字节单位的内存
        // int Offset = (Nd->Name - 'a' + 1) * 8;
        // printf("  addi a0, fp, %d\n", -Offset);
        // 偏移量是相对于fp的
        printf("  # 获取变量%s的栈内地址为%d(fp)\n", Nd->Var->Name,
               Nd->Var->Offset);
        printf("  addi a0, fp, %d\n", Nd->Var->Offset);

        return;
    }
        // 解引用*  //当如*y作为左值时
    case ND_DEREF:
        genExpr(Nd->LHS);
        return;
    default:
        break;
    }
    errorTok(Nd->Tok, "not an lvalue");
}
// 根据变量的链表计算出偏移量
static void assignLVarOffsets(Function *Prog)
{
    int Offset = 0;
    // 读取所有变量
    for (Obj *Var = Prog->Locals; Var; Var = Var->Next)
    {
        // 每个变量分配8字节
        Offset += Var->Ty->Size;
        // 为每个变量赋一个偏移量，或者说是栈中地址
        Var->Offset = -Offset;
    }
    // 将栈对齐到16字节
    Prog->StackSize = alignTo(Offset, 16);
}
// 生成表达式
static void genExpr(Node *Nd)
{
    switch (Nd->Kind)
    {
    // 加载数字到a0
    case ND_NUM:
        printf("  # 将%d加载到a0中\n", Nd->Val);
        printf("  li a0, %d\n", Nd->Val);
        return;
    // 对寄存器取反
    case ND_NEG:
        genExpr(Nd->LHS);
        // neg a0, a0是sub a0, x0, a0的别名, 即a0=0-a0
        printf("  # 对a0值进行取反\n");
        printf("  neg a0, a0\n");
        return;
    // 变量
    case ND_VAR:
        // 计算出变量的地址，然后存入a0
        genAddr(Nd);
        load(Nd->Ty);
        return;
        // 解引用
    case ND_DEREF: // 解引用理解起来难，实现简单，如*y ：现在y中其实是地址，只需要再把地址做一次寻址操作就行了
        genExpr(Nd->LHS);
        // 只需要再把地址做一次寻址操作就行了
        load(Nd->Ty);
        return;
    // 取地址
    case ND_ADDR:
        genAddr(Nd->LHS);
        return;
    // // 赋值
    // case ND_ASSIGN:
    //     // 左部是左值，保存值到的地址
    //     genAddr(Nd->LHS);
    //     push();
    //     // 右部是右值，为表达式的值
    //     genExpr(Nd->RHS);
    //     pop("a1");
    //     printf("  sd a0, 0(a1)\n");
    // 因为赋值操作有一个返回值，所以需要一个语句，这就是为什么不用下面的递归
    // return;

    // 函数调用
    case ND_FUNCALL:
    {
        // 记录参数个数
        int NArgs = 0;
        // 计算所有参数的值，正向压栈
        for (Node *Arg = Nd->Args; Arg; Arg = Arg->Next)
        {
            genExpr(Arg);
            push();
            NArgs++;
        }

        // 反向弹栈，a0->参数1，a1->参数2……
        for (int i = NArgs - 1; i >= 0; i--)
            pop(ArgReg[i]);

        printf("\n  # 调用函数%s\n", Nd->FuncName);
        printf("  call %s\n", Nd->FuncName);
        return;
    }
    default:
        break;
    }
    /*******************************************

               * 特殊的非叶子结点

     *******************************************/
    switch (Nd->Kind)
    {
    // 赋值
    // 因为变量操作会返回值而不是地址，所以需要一个语句，这就是为什么不用下面的递归
    case ND_ASSIGN:
        // 左部是左值，保存值到的地址
        genAddr(Nd->LHS);
        push();
        // 右部是右值，为表达式的值
        genExpr(Nd->RHS);
        store();
        return;
    default:
        break;
    }
    /*******************************************

               * 非叶子结点

     *******************************************/
    // 右子树保存在a1,左子树在a0
    //  递归到最右节点
    genExpr(Nd->RHS);
    // 将结果压入栈
    push();
    // 递归到左节点
    genExpr(Nd->LHS);
    // 将结果弹栈到a1
    pop("a1");

    // 生成各个二叉树节点
    // 这些左右子树都有一层及以上
    switch (Nd->Kind)
    {
    case ND_ADD: // + a0=a0+a1
        printf("  # a0+a1，结果写入a0\n");
        printf("  add a0, a0, a1\n");
        return;
    case ND_SUB: // - a0=a0-a1
        printf("  # a0-a1，结果写入a0\n");
        printf("  sub a0, a0, a1\n");
        return;
    case ND_MUL: // * a0=a0*a1
        printf("  # a0×a1，结果写入a0\n");
        printf("  mul a0, a0, a1\n");
        return;
    case ND_DIV: // / a0=a0/a1
        printf("  # a0÷a1，结果写入a0\n");
        printf("  div a0, a0, a1\n");
        return;
    case ND_EQ:
    case ND_NE:
        // a0=a0^a1，异或指令
        printf("  # 判断是否a0%sa1\n", Nd->Kind == ND_EQ ? "=" : "≠");
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
        printf("  # 判断a0<a1\n");
        printf("  slt a0, a0, a1\n");
        return;
    case ND_LE:
        // a0<=a1等价于  !(a1<a0)
        // a0=a1<a0, a0=a0^1
        printf("  # 判断是否a0≤a1\n");
        printf("  slt a0, a1, a0\n");
        printf("  xori a0, a0, 1\n");
        return;
    // 赋值
    case ND_ASSIGN:
        printf("  sd a1, 0(a0)\n");
    default:
        break;
    }

    errorTok(Nd->Tok, "invalid expression");
}

// 生成语句
static void genStmt(Node *Nd)
{
    switch (Nd->Kind)
    {
    // 生成return语句
    case ND_RETURN:
        genExpr(Nd->LHS);
        // 无条件跳转语句，跳转到.L.return段
        // j offset是 jal x0, offset的别名指令
        printf("# 返回语句\n");
        printf("  j .L.return.%s\n", CurFn->Name);
        return;
    // 生成表达式语句
    case ND_EXPR_STMT:
        genExpr(Nd->LHS);
        return;
        // 生成代码块，遍历代码块的语句链表
    case ND_BLOCK:
        for (Node *N = Nd->Body; N; N = N->Next)
            genStmt(N);
        return;
        // 生成if语句
    case ND_IF:
    {
        // 代码段计数
        int C = count();
        printf("\n# =====分支语句%d==============\n", C);
        // 生成条件内语句
        printf("\n# Cond表达式%d\n", C);
        genExpr(Nd->Cond);
        // 判断结果是否为0，为0则跳转到else标签
        printf("  # 若a0为0，则跳转到分支%d的.L.else.%d段\n", C, C);
        printf("  beqz a0, .L.else.%d\n", C);
        // 生成符合条件后的语句
        printf("\n# Then语句%d\n", C);
        genStmt(Nd->Then);
        // 执行完后跳转到if语句后面的语句
        printf("  # 跳转到分支%d的.L.end.%d段\n", C, C);
        printf("  j .L.end.%d\n", C);
        // else代码块，else可能为空，故输出标签
        printf("\n# Else语句%d\n", C);
        printf("# 分支%d的.L.else.%d段标签\n", C, C);
        printf(".L.else.%d:\n", C);
        // 生成不符合条件后的语句
        if (Nd->Els)
            genStmt(Nd->Els);
        // 结束if语句，继续执行后面的语句
        printf("\n# 分支%d的.L.end.%d段标签\n", C, C);
        printf(".L.end.%d:\n", C);
        return;
    }
    // 生成for或while循环语句
    case ND_FOR:
    {
        // 代码段计数
        int C = count();
        printf("\n# =====循环语句%d===============\n", C);
        // 生成初始化语句
        if (Nd->Init)
        {
            printf("\n# Init语句%d\n", C);
            genStmt(Nd->Init);
        }
        // 输出循环头部标签
        printf("\n# 循环%d的.L.begin.%d段标签\n", C, C);
        printf(".L.begin.%d:\n", C);
        // 处理循环条件语句
        if (Nd->Cond)
        {
            // 生成条件循环语句
            printf("# Cond表达式%d\n", C);
            genExpr(Nd->Cond);
            // 判断结果是否为0，为0则跳转到结束部分
            printf("  # 若a0为0，则跳转到循环%d的.L.end.%d段\n", C, C);
            printf("  beqz a0, .L.end.%d\n", C);
        }
        // 生成循环体语句
        printf("\n# Then语句%d\n", C);
        genStmt(Nd->Then);
        // 处理循环递增语句
        if (Nd->Inc)
        {
            printf("\n# Inc语句%d\n", C);
            // 生成循环递增语句
            genExpr(Nd->Inc);
        }
        // 跳转到循环头部
        printf("  # 跳转到循环%d的.L.begin.%d段\n", C, C);
        printf("  j .L.begin.%d\n", C);
        // 输出循环尾部标签
        printf("\n# 循环%d的.L.end.%d段标签\n", C, C);
        printf(".L.end.%d:\n", C);
        return;
    }
    default:
        break;
    }

    errorTok(Nd->Tok, "invalid statement");
}

// 代码生成入口函数，包含代码块的基础信息
void codegen(Function *Prog)
{
    // 为每个函数单独生成代码
    for (Function *Fn = Prog; Fn; Fn = Fn->Next)
    {
        CurFn = Fn; // 用于在程序中return
        // 声明一个全局main段，同时也是程序入口段
        printf("  # 定义全局%s段\n", Fn->Name);
        printf("  .globl %s\n", Fn->Name);
        // main段标签
        printf("\n# =====%s段程序开始===============\n", Fn->Name);
        printf("# %s段标签，也是入口段\n", Fn->Name);
        printf("%s:\n", Fn->Name);

        // 栈布局
        //-------------------------------// sp
        //              ra
        //-------------------------------// ra = sp-8
        //              fp
        //-------------------------------// fp = sp-16
        //             变量
        //-------------------------------// sp = sp-16-StackSize
        //           表达式计算
        //-------------------------------//

        // Prologue, 前言
        // 将ra寄存器压栈,保存ra的值
        printf("  # 将ra寄存器压栈,保存ra的值\n");
        printf("  addi sp, sp, -16\n");
        printf("  sd ra, 8(sp)\n");
        // 将fp压入栈中，保存fp的值
        printf("  # 将fp压栈，fp属于“被调用者保存”的寄存器，需要恢复原值\n");
        // printf("  addi sp, sp, -8\n");
        printf("  sd fp, 0(sp)\n");
        // 将sp写入fp
        printf("  # 将sp的值写入fp\n");
        printf("  mv fp, sp\n");
        // 分配变量
        assignLVarOffsets(Fn);
        // 偏移量为实际变量所用的栈大小
        printf("  # sp腾出StackSize大小的栈空间\n");
        printf("  addi sp, sp, -%d\n", Fn->StackSize);

        int I = 0;
        for (Obj *Var = Fn->Params; Var; Var = Var->Next)
        {
            printf("  # 将%s寄存器的值存入%s的栈地址\n", ArgReg[I], Var->Name);
            printf("  sd %s, %d(fp)\n", ArgReg[I++], Var->Offset);
        }
        
        // 生成语句链表的代码
        printf("\n# =====%s段主体===============\n", Fn->Name);
        genStmt(Fn->Body);

        // assert(Depth == 0);

        // Epilogue，后语
        // 输出return段标签
        printf("\n# =====%s结束===============\n", Fn->Name);
        printf("# return段标签\n");
        printf(".L.return.%s:\n", Fn->Name);
        // 将fp的值改写回sp
        printf("  # 将fp的值写回sp\n");
        printf("  mv sp, fp\n");
        // 将最早fp保存的值弹栈，恢复fp。
        printf("  # 将最早fp保存的值弹栈，恢复fp和sp\n");
        printf("  ld fp, 0(sp)\n");
        // printf("  addi sp, sp, 8\n");
        //  将ra寄存器弹栈,恢复ra的值
        printf("  # 将ra寄存器弹栈,恢复ra的值\n");
        printf("  ld ra, 8(sp)\n");
        printf("  addi sp, sp, 16\n");
        // ret为jalr x0, x1, 0别名指令，用于返回子程序
        // 返回的为a0的值
        printf("  # 返回a0值给系统调用\n");
        printf("  ret\n");
    }
    // assert(Depth == 0);
}