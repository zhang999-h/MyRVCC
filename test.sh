#!/bin/bash
cd build
  ninja
  cd ..


# 将下列代码编译为tmp2.o，"-xc"强制以c语言进行编译
#cat <<EOF | gcc -xc -c -o func.o -
cat <<EOF | $RISCV/bin/riscv64-unknown-linux-gnu-gcc -xc -c -o func.o -

int ret3() { return 3; }
int ret5() { return 5; }
int add(int x, int y) { return x+y; }
int sub(int x, int y) { return x-y; }
int add6(int a, int b, int c, int d, int e, int f) {
  return a+b+c+d+e+f;
}
EOF

# 声明一个函数
assert() {
  # 程序运行的 期待值 为参数1
  expected="$1"
  # 输入值 为参数2
  input="$2"

  # 运行程序，传入期待值，将生成结果写入tmp.s汇编文件。
  # 如果运行不成功，则会执行exit退出。成功时会短路exit操作
  echo "$input" |./build/MyRVCC -o rvcc.s - || exit
  #echo "int main(){ return -10+20; }" |./build/MyRVCC - > rvcc.s || exit
  #./build/MyRVCC "$input" > rvcc.s||exit
  # 编译rvcc产生的汇编文件
  #gcc -o tmp tmp.s
  #$RISCV/bin/riscv64-unknown-linux-gnu-gcc -static -o rvcc rvcc.s
  #gcc -static -o rvcc rvcc.s tmp2.o
  $RISCV/bin/riscv64-unknown-linux-gnu-gcc -static -o rvcc rvcc.s func.o

  # 运行生成出来目标文件
  #./tmp
   qemu-riscv64 -L $RISCV/sysroot ./rvcc
  # $RISCV/bin/spike --isa=rv64gc $RISCV/riscv64-unknown-linux-gnu/bin/pk ./tmp

  # 获取程序返回值，存入 实际值
  actual="$?"

  # 判断实际值，是否为预期值
  if [ "$actual" = "$expected" ]; then
    echo "$input => $actual"
  else
    echo "$input => $expected expected, but got $actual"
    exit 1
  fi
}
  
# assert 期待值 输入值
# [1] 返回指定数值
#assert 0 0
#assert 42 42

# [2] 支持+ -运算符
#assert 34 '12-34+56'
# [3] 支持空格
#assert 5 ' 2+3 '

# [5] 支持* / ()运算符
#assert 47 '5+6*7'
#assert 15 '5*(9-6)'
#assert 17 '1-8/(2*2)+3*6'

#    'int main() { return  ;}'
  # [57] 支持long类型
  assert  6   '
  int sub_long(long a, long b, long c) {
    return a - b - c;
  }
  int main() {long x=9,y=1,z=2; return sub_long(x,y,z);}
  '
  assert 16 'int main() { return  ({ struct {char a; long b;} x; sizeof(x); });}'
  assert 8 'int main() { return ({ long x; sizeof(x); }) ;}'
  # [56] 将int的大小由8改为4
  assert 4 'int main() { return ({ int x; sizeof(x); }) ;}'
  assert 4 'int main() { return ({ int x; sizeof x; }) ;}'
  # [55] 支持结构体赋值 //结构体赋值应该把内容也拷贝过去  TODO：不同结构体类型赋值判断
  assert 3 'int main() { return  ({ struct {int a,b;} x,y; x.a=3; y=x; y.a; });}'
  assert 7 'int main() { return  ({ struct t {int a,b;}; struct t x; x.a=7; struct t y; struct t *z=&y; *z=x; y.a; });}'
  assert 7 'int main() { return  ({ struct t {int a,b;}; struct t x; x.a=7; struct t y, *p=&x, *q=&y; *q=*p; y.a; });}'
  assert 5 'int main() { return  ({ struct t {char a, b;} x, y; x.a=5; y=x; y.a; });}'
  assert 3 'int main() { return  ({ union {int a,b;} x,y; x.a=3; y.a=5; y=x; y.a; });}'
  assert 5 'int main() { return  ({ struct t {struct {int a;} ta;} x, y; x.ta.a=5; y=x; y.ta.a; });}'
  assert 3 'int main() { return  ({ union {struct {int a,b;} c;} x,y; x.c.b=3; y.c.b=5; y=x; y.c.b; });}'
  #[54] 支持union
  assert 8 'int main() { return ({ union { int a; char b[6]; } x; sizeof(x); });}'
  assert 3 'int main() { return ({ union { int a; char b[4]; } x; x.a = 515; x.b[0]; });}'
  assert 2 'int main() { return ({ union { int a; char b[4]; } x; x.a = 515; x.b[1]; });}'
  assert 0 'int main() { return ({ union { int a; char b[4]; } x; x.a = 515; x.b[2]; });}'
  assert 0 'int main() { return ({ union { int a; char b[4]; } x; x.a = 515; x.b[3]; });}'
  # [53] 支持->操作符
  assert 3 'int main() { return ({ struct t {char a;} x; struct t *y = &x; x.a=3; y->a; });}' ;
  assert 3 'int main() { return ({ struct t {char a;} x; struct t *y = &x; y->a=3; x.a; });}' ;
  #[52] 支持结构体标签
  assert 8 'int main() { struct t {int a; int b;} x; struct t y;  return sizeof(y);}'
  assert 8 'int main() {struct t {int a; int b;}; struct t y;  return sizeof(y);}'
  assert 2 'int main() { return ({ struct t {char a[2];}; { struct t {char a[4];}; } struct t y; sizeof(y); }); }'
  assert 3 'int main() { return ({ struct t {int x;}; int t=1; struct t y; y.x=2; t+y.x; }) ;}'

# [51] 对齐局部变量
  assert 1 'int main() { int x; char y; int z; char *a=&y; char *b=&z; return b-a;}'
  assert 7 'int main() { int x; int y; char z; char *a=&y; char *b=&z;  return b-a; }'
# important:
# 变量对齐 (rvcc中变量后来局上)
# -1  z
# -2  ...
# -8  y
# ...
# -16 x

#50 对齐结构体成员变量
  assert 12 'int main() { struct {char a; int b;char c;} x;return sizeof(x); }'
  assert 8 'int main() { struct {char a; char b;int c;} x;return sizeof(x); }'
  assert 8 'int main() { struct {char a; int b;} x;return sizeof(x); }'
  assert 8 'int main() {struct {int a; char b;} x;return sizeof(x); }'
  # [48] 支持 , 运算符
  assert 3 'int main() { return (1,2,3); }' 
  assert 5 'int main() { int i=2, j=3; (i=5,j)=6; i; return i; }'
  assert 6 'int main() { int i=2, j=3; (i=5,j)=6; return j; }'
#[49] 支持struct
  assert 1 'int main() { struct {int a; int b;} x; x.a=1; x.b=2;return x.a; }'
  assert 2 'int main() { struct {int a; int b;} x; x.a=1; x.b=2;return x.b; }'
  assert 1 'int main() { struct {char a; int b; char c;} x; x.a=1; x.b=2; x.c=3;return x.a; }'
  assert 2 'int main() { struct {char a; int b; char c;} x; x.b=1; x.b=2; x.c=3;return x.b; }'
  assert 3 'int main() { struct {char a; int b; char c;} x; x.a=1; x.b=2; x.c=3;return x.c; }'

  assert 0 'int main() { struct {char a; char b;} x[3]; char *p=x; p[0]=0;return x[0].a; }'
  assert 1 'int main() { struct {char a; char b;} x[3]; char *p=x; p[1]=1;return x[0].b; }'
  assert 2 'int main() { struct {char a; char b;} x[3]; char *p=x; p[2]=2;return x[1].a; }'
  assert 3 'int main() { struct {char a; char b;} x[3]; char *p=x; p[3]=3;return x[1].b; }'

  assert 6 'int main() { struct {char a[3]; char b[5];} x; char *p=&x; x.a[0]=6;return p[0]; }'
  assert 7 'int main() { struct {char a[3]; char b[5];} x; char *p=&x; x.b[0]=7;return p[3]; }'

  assert 6 'int main() { struct { struct { char b; } a; } x; x.a.b=6;return x.a.b; }'

  assert 4 'int main() { struct {int a;} x;return sizeof(x); }'
  assert 8 'int main() { struct {int a; int b;} x;return sizeof(x); }'
  assert 8 'int main() { struct {int a, b;} x;return sizeof(x); }'
  assert 12 'int main() { struct {int a[3];} x;return sizeof(x); }'
  assert 16 'int main() { struct {int a;} x[4];return sizeof(x); }'
  assert 24 'int main() { struct {int a[3];} x[2];return sizeof(x); }'
  assert 2 'int main() { struct {char a; char b;} x;return sizeof(x); }'

  assert 0 'int main() { struct {} x;return sizeof(x); }'

# [44] 处理域
assert 2 'int main() { int x=2; { int x=3; } return x; }'
assert 2 'int main() { int x=2; { int x=3; } { int y=4; return x; }}'
assert 3 'int main() { int x=2; { x=3; } return x; }'
# [43] 支持注释
assert 2 'int main() { /* return 1; */
             return 2; }'
assert 2 'int main() { // return 1;
             return 2; }'
# [6] 支持一元运算的+ -
assert 10 'int main(){ return -10+20; }'
assert 10 'int main(){ return - -10; }'
assert 10 'int main(){ return - - +10; }'
assert 48 'int main(){ return ------12*+++++----++++++++++4; }'

# [7] 支持条件运算符
assert 0 'int main(){0==1;}'
assert 1 'int main(){42==42;}'
assert 1 'int main(){0!=1;}'
assert 0 'int main(){42!=42;}'
assert 1 'int main(){1>=1;}'
assert 0 'int main(){1>=2;}'
assert 1 'int main(){5==2+3;}'
assert 0 'int main(){6==4+3;}'
assert 1 'int main(){0*9+5*2==4+4*(6/3)-2;}'
#assert 1 '0<1;'
#assert 0 '1<1;'
#assert 0 '2<1;'
assert 1 'int main(){0<=1;}'
assert 1 'int main(){1<=1;}'
assert 0 'int main(){2<=1;}'
assert 1 'int main(){1>0;}'

# [9] 支持;分割语句
#assert 3 '1; 2; 3;'
#assert 12 '12+23;12+99/3;78-66;'


# [10] 支持单字母变量
assert 3 'int main(){ int a=3;return a; }'
assert 8 'int main(){ int a=3,z=5;return a+z; }'
assert 6 'int main(){ int a,b; a=b=3;return a+b; }'
assert 5 'int main(){ int a=3,b=4,a=1;return a+b; }'
#assert 5 'a=3;b=4;a=1;a+b;'
# [11] 支持多字母变量
assert 3 'int main(){ int foo=3;return foo; }'
assert 74 'int main(){ int foo2=70; int bar4=4;return foo2+bar4; }'

# [12] 支持return

assert 2 'int main(){1; return 2; 3;}'
assert 3 'int main(){1; 2; return 3;}'
assert 74 'int main(){int foo2=70;int bar4=4;return foo2+bar4;}'
assert 1 'int main(){return 1; 2; 3;}'


# [13] 支持{...}
assert 3 'int main(){ {1; {2;} return 3;} }'
# 如果运行正常未提前退出，程序将显示OK
# [14] 支持空语句
assert 5 'int main(){ ;;; return 5; }'

# [15] 支持if语句
assert 3 'int main(){ if (0) return 2; return 3; }'
assert 3 'int main(){ if (1-1) return 2; return 3; }'
assert 2 'int main(){ if (1) return 2; return 3; }'
assert 2 'int main(){ if (2-1) return 2; return 3; }'
assert 4 'int main(){ if (0) { 1; 2; return 3; } else { return 4; } }'
assert 3 'int main(){ if (1) { 1; 2; return 3; } else { return 4; } }'
# [16] 支持for语句
assert 55 'int main(){int i=0;int j=0; for (i=0; i<=10; i=i+1) j=i+j; return j; }'
assert 3 'int main(){ for (;;) {return 3;} return 5; }'

# [17] 支持while语句 复用了FOR的节点 
assert 10 'int main(){int i=0; while(i<10) { i=i+1; } return i; }'


# [20] 支持一元& *运算符
assert 3 'int main(){int x=3; return *&x; }'
assert 3 'int main(){int x=3;int *y=&x;int **z=&y; return **z; }'
#assert 5 '{ x=3; y=5; return *(&x+8); }'
#assert 3 '{ x=3; y=5; return *(&y-8); }'
assert 5 'int main(){int x=3;int *y=&x; *y=5; return x; }'
# assert 7 '{ x=3; y=5; *(&x+8)=7; return y; }'
# assert 7 '{ x=3; y=5; *(&y-8)=7; return x; }'
assert 8 'int main(){int x=3;int *y=&x;  x=*y+5; return x; }'


# [21] 支持指针的算术运算
assert 3 'int main(){int x=3;int y=5; return *(&y-1); }'
assert 5 'int main(){int x=3;int y=5; return *(&x+1); }'
assert 7 'int main(){int x=3;int y=5; *(&y-1)=7; return x; }'
assert 7 'int main(){int x=3;int y=5; *(&x+1)=7; return y; }'


# [22] 支持int关键字
assert 8 'int main(){ int x, y; x=3; y=5; return x+y; }'
assert 8 'int main(){ int x=3, y=5; return x+y; }'

# [23] 支持零参函数调用
assert 3 'int main(){ return ret3(); }'
assert 5 'int main(){ return ret5(); }'
assert 8 'int main(){ return ret3()+ret5(); }'


# [24] 支持最多6个参数的函数调用
assert 8 'int main(){ return add(3, 5); }'
assert 2 'int main(){ return sub(5, 3); }'
assert 21 'int main(){ return add6(1,2,3,4,5,6); }'
assert 66 'int main(){ return add6(1,2,add6(3,4,5,6,7,8),9,10,11); }'
assert 136 'int main(){ return add6(1,2,add6(3,add6(4,5,6,7,8,9),10,11,12,13),14,15,16); }'

# [26] 支持最多6个参数的函数定义
assert 7 'int main() { return add2(3,4); } int add2(int x, int y) { return x+y; }'
assert 1 'int main() { return sub2(4,3); } int sub2(int x, int y) { return x-y; }'
assert 55 'int main() { return fib(9); } int fib(int x) { if (x<=1) return 1; return fib(x-1) + fib(x-2); }'

# [27] 支持一维数组
assert 3 'int main() { int x[2]; int *y=&x; *y=3; return *x; }'
assert 3 'int main() { int x[3]; *x=3; *(x+1)=4; *(x+2)=5; return *x; }'
assert 4 'int main() { int x[3]; *x=3; *(x+1)=4; *(x+2)=5; return *(x+1); }'
assert 5 'int main() { int x[3]; *x=3; *(x+1)=4; *(x+2)=5; return *(x+2); }'


# [28] 支持多维数组
assert 0 'int main() { int x[2][3]; int *y=x; *y=0; return **x; }'
assert 1 'int main() { int x[2][3]; int *y=x; *(y+1)=1; return *(*x+1); }'
assert 2 'int main() { int x[2][3]; int *y=x; *(y+2)=2; return *(*x+2); }'
assert 3 'int main() { int x[2][3]; int *y=x; *(y+3)=3; return **(x+1); }'
assert 4 'int main() { int x[2][3]; int *y=x; *(y+4)=4; return *(*(x+1)+1); }'
assert 5 'int main() { int x[2][3]; int *y=x; *(y+5)=5; return *(*(x+1)+2); }'


# [29] 支持 [] 操作符
assert 3 'int main() { int x[3]; *x=3; x[1]=4; x[2]=5; return *x; }'
assert 4 'int main() { int x[3]; *x=3; x[1]=4; x[2]=5; return *(x+1); }'
assert 5 'int main() { int x[3]; *x=3; x[1]=4; x[2]=5; return *(x+2); }'
assert 5 'int main() { int x[3]; *x=3; x[1]=4; x[2]=5; return *(x+2); }'
assert 5 'int main() { int x[3]; *x=3; x[1]=4; 2[x]=5; return *(x+2); }'

assert 0 'int main() { int x[2][3]; int *y=x; y[0]=0; return x[0][0]; }'
assert 1 'int main() { int x[2][3]; int *y=x; y[1]=1; return x[0][1]; }'
assert 2 'int main() { int x[2][3]; int *y=x; y[2]=2; return x[0][2]; }'
assert 3 'int main() { int x[2][3]; int *y=x; y[3]=3; return x[1][0]; }'
assert 4 'int main() { int x[2][3]; int *y=x; y[4]=4; return x[1][1]; }'
assert 5 'int main() { int x[2][3]; int *y=x; y[5]=5; return x[1][2]; }'
# [30] 支持 sizeof
assert 4 'int main() { int x; return sizeof(x); }'
assert 4 'int main() { int x; return sizeof x; }'
assert 8 'int main() { int *x; return sizeof(x); }'
assert 16 'int main() { int x[4]; return sizeof(x); }'
assert 48 'int main() { int x[3][4]; return sizeof(x); }'
assert 16 'int main() { int x[3][4]; return sizeof(*x); }'
assert 4 'int main() { int x[3][4]; return sizeof(**x); }'
assert 5 'int main() { int x[3][4]; return sizeof(**x) + 1; }'
assert 5 'int main() { int x[3][4]; return sizeof **x + 1; }'
assert 4 'int main() { int x[3][4]; return sizeof(**x + 1); }'
assert 4 'int main() { int x=1; return sizeof(x=2); }'
assert 1 'int main() { int x=1; sizeof(x=2); return x; }'

#[31] 全局变量不属于任何函数，但是全局变量可以在函数外初始化，
#也就是说 Obj 数据结构也需要 Node 结点用作 codeGen 中生成代码，所以不如融合

# [32] 支持全局变量
assert 0 'int x; int main() { return x; }'
assert 3 'int x; int main() { x=3; return x; }'
assert 7 'int x; int y; int main() { x=3; y=4; return x+y; }'
assert 7 'int x, y; int main() { x=3; y=4; return x+y; }'
assert 0 'int x[4]; int main() { x[0]=0; x[1]=1; x[2]=2; x[3]=3; return x[0]; }'
assert 1 'int x[4]; int main() { x[0]=0; x[1]=1; x[2]=2; x[3]=3; return x[1]; }'
assert 2 'int x[4]; int main() { x[0]=0; x[1]=1; x[2]=2; x[3]=3; return x[2]; }'
assert 3 'int x[4]; int main() { x[0]=0; x[1]=1; x[2]=2; x[3]=3; return x[3]; }'

assert 4 'int x; int main() { return sizeof(x); }'
assert 16 'int x[4]; int main() { return sizeof(x); }'

# [33] 支持char类型
assert 1 'int main() { char x=1; return x; }'
assert 1 'int main() { char x=1; char y=2; return x; }'
assert 2 'int main() { char x=1; char y=2; return y; }'

assert 1 'int main() { char x; return sizeof(x); }'
assert 10 'int main() { char x[10]; return sizeof(x); }'
assert 1 'int main() { return sub_char(7, 3, 3); } int sub_char(char a, char b, char c) { return a-b-c; }'
# [34] 支持字符串字面量
assert 0 'int main() { return ""[0]; }'
assert 1 'int main() { return sizeof(""); }'

assert 97 'int main() { return "abc"[0]; }'
assert 98 'int main() { return "abc"[1]; }'
assert 99 'int main() { return "abc"[2]; }'
assert 0 'int main() { return "abc"[3]; }'
assert 4 'int main() { return sizeof("abc"); }'

# [36] 支持转义字符
assert 7 'int main() { return "\a"[0]; }'
assert 8 'int main() { return "\b"[0]; }'
assert 9 'int main() { return "\t"[0]; }'
assert 10 'int main() { return "\n"[0]; }'
assert 11 'int main() { return "\v"[0]; }'
assert 12 'int main() { return "\f"[0]; }'
assert 13 'int main() { return "\r"[0]; }'
assert 27 'int main() { return "\e"[0]; }'

assert 106 'int main() { return "\j"[0]; }'
assert 107 'int main() { return "\k"[0]; }'
assert 108 'int main() { return "\l"[0]; }'

assert 7 'int main() { return "\ax\ny"[0]; }'
assert 120 'int main() { return "\ax\ny"[1]; }'
assert 10 'int main() { return "\ax\ny"[2]; }'
assert 121 'int main() { return "\ax\ny"[3]; }'
# [37] 支持八进制转义字符
assert 0 'int main() { return "\0"[0]; }'
assert 16 'int main() { return "\20"[0]; }'
assert 65 'int main() { return "\101"[0]; }'
assert 104 'int main() { return "\1500"[0]; }'
# [38] 支持十六进制转义字符
assert 0 'int main() { return "\x00"[0]; }'
assert 119 'int main() { return "\x77"[0]; }'
assert 165 'int main() { return "\xA5"[0]; }'
assert 255 'int main() { return "\x00ff"[0]; }'

# [39] 添加语句表达式
assert 0 'int main() { return ({ 0; }); }'
assert 2 'int main() { return ({ 0; 1; 2; }); }'
assert 1 'int main() { ({ 0; return 1; 2; }); return 3; }'
assert 6 'int main() { return ({ 1; }) + ({ 2; }) + ({ 3; }); }'
assert 3 'int main() { return ({ int x=3; x; }); }'

echo OK