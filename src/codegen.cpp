#include "head.h"
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
static void pop(const char *Reg)
{
    printf("  ld %s, 0(sp)\n", Reg);
    printf("  addi sp, sp, 8\n");
    Depth--;
}

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
    if (Nd->Kind == ND_VAR)
    {
        // // 偏移量=是两个字母在ASCII码表中的距离加1后乘以8，*8表示每个变量需要八个字节单位的内存
        // int Offset = (Nd->Name - 'a' + 1) * 8;
        // printf("  addi a0, fp, %d\n", -Offset);
        // 偏移量是相对于fp的
        printf("  addi a0, fp, %d\n", Nd->Var->Offset);

        return;
    }

    error("not an lvalue");
}
// 根据变量的链表计算出偏移量
static void assignLVarOffsets(Function *Prog)
{
    int Offset = 0;
    // 读取所有变量
    for (Obj *Var = Prog->Locals; Var; Var = Var->Next)
    {
        // 每个变量分配8字节
        Offset += 8;
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
        printf("  li a0, %d\n", Nd->Val);
        return;
    // 对寄存器取反
    case ND_NEG:
        genExpr(Nd->LHS);
        // neg a0, a0是sub a0, x0, a0的别名, 即a0=0-a0
        printf("  neg a0, a0\n");
        return;
    // 变量
    case ND_VAR:
        // 计算出变量的地址，然后存入a0
        genAddr(Nd);
        // 访问a0地址中存储的数据，存入到a0当中
        printf("  ld a0, 0(a0)\n");
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
        //因为赋值操作有一个返回值，所以需要一个语句，这就是为什么不用下面的递归
        //return;
    default:
        break;
    }
/*******************************************

           * 特殊的非叶子结点

 *******************************************/
    switch (Nd->Kind)
    {
    // 赋值
    //因为变量操作会返回值而不是地址，所以需要一个语句，这就是为什么不用下面的递归
    case ND_ASSIGN:
        // 左部是左值，保存值到的地址
        genAddr(Nd->LHS);
        push();
        // 右部是右值，为表达式的值
        genExpr(Nd->RHS);
        pop("a1");
        printf("  sd a0, 0(a1)\n");
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
    // 赋值
    case ND_ASSIGN:
        printf("  sd a1, 0(a0)\n");
    default:
        break;
    }

    error("invalid expression");
}

// 生成语句
static void genStmt(Node *Nd)
{
    if (Nd->Kind == ND_EXPR_STMT)
    {
        genExpr(Nd->LHS);
        return;
    }

    error("invalid statement");
}

// 代码生成入口函数，包含代码块的基础信息
void codegen(Function *Prog)
{
    // 声明一个全局main段，同时也是程序入口段
    printf("  .globl main\n");
    // main段标签
    printf("main:\n");

    // 栈布局
    //-------------------------------// sp
    //              fp
    //-------------------------------// fp = sp-8
    //             变量
    //-------------------------------// sp = sp-8-StackSize
    //           表达式计算
    //-------------------------------//

    // Prologue, 前言
    // 将fp压入栈中，保存fp的值
    printf("  addi sp, sp, -8\n");
    printf("  sd fp, 0(sp)\n");
    // 将sp写入fp
    printf("  mv fp, sp\n");
    // 分配变量
    assignLVarOffsets(Prog);
    // 偏移量为实际变量所用的栈大小
    printf("  addi sp, sp, -%d\n", Prog->StackSize);

    // genExpr(Nd);
    // 循环遍历所有的语句
    for (Node *N = Prog->Body; N; N = N->Next)
    {
        genStmt(N);
        // assert(Depth == 0);
    }

    // Epilogue，后语
    // 将fp的值改写回sp
    printf("  mv sp, fp\n");
    // 将最早fp保存的值弹栈，恢复fp。
    printf("  ld fp, 0(sp)\n");
    printf("  addi sp, sp, 8\n");

    // ret为jalr x0, x1, 0别名指令，用于返回子程序
    // 返回的为a0的值
    printf("  ret\n");

    // assert(Depth == 0);
}