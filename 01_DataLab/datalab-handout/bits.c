/* 
 * CS:APP Data Lab 
 * 
 * <Please put your name and userid here>
 * 
 * bits.c - Source file with your solutions to the Lab.
 *          This is the file you will hand in to your instructor.
 *
 * WARNING: Do not include the <stdio.h> header; it confuses the dlc
 * compiler. You can still use printf for debugging without including
 * <stdio.h>, although you might get a compiler warning. In general,
 * it's not good practice to ignore compiler warnings, but in this
 * case it's OK.  
 */

#if 0
/*
 * Instructions to Students:
 *
 * STEP 1: Read the following instructions carefully.
 */

You will provide your solution to the Data Lab by
editing the collection of functions in this source file.

INTEGER CODING RULES:
 
  Replace the "return" statement in each function with one
  or more lines of C code that implements the function. Your code 
  must conform to the following style:
 
  int Funct(arg1, arg2, ...) {
      /* brief description of how your implementation works */
      int var1 = Expr1;
      ...
      int varM = ExprM;

      varJ = ExprJ;
      ...
      varN = ExprN;
      return ExprR;
  }

  Each "Expr" is an expression using ONLY the following:
  1. Integer constants 0 through 255 (0xFF), inclusive. You are
      not allowed to use big constants such as 0xffffffff.
  2. Function arguments and local variables (no global variables).
  3. Unary integer operations ! ~（第一个是逻辑非，经过测试，true为1，false为0，第二个是按位取反）
  4. Binary integer operations & ^ | + << >>
    
  Some of the problems restrict the set of allowed operators even further.
  Each "Expr" may consist of multiple operators. You are not restricted to
  one operator per line.

  You are expressly forbidden to:
  1. Use any control constructs such as if, do, while, for, switch, etc.
  2. Define or use any macros.
  3. Define any additional functions in this file.
  4. Call any functions.
  5. Use any other operations, such as &&, ||, -, or ?:
  6. Use any form of casting.
  7. Use any data type other than int.  This implies that you
     cannot use arrays, structs, or unions.

 
  You may assume that your machine:
  1. Uses 2s complement, 32-bit representations of integers.
  2. Performs right shifts arithmetically.
  3. Has unpredictable behavior when shifting if the shift amount
     is less than 0 or greater than 31.


EXAMPLES OF ACCEPTABLE CODING STYLE:
  /*
   * pow2plus1 - returns 2^x + 1, where 0 <= x <= 31
   */
  int pow2plus1(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     return (1 << x) + 1;
  }

  /*
   * pow2plus4 - returns 2^x + 4, where 0 <= x <= 31
   */
  int pow2plus4(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     int result = (1 << x);
     result += 4;
     return result;
  }

FLOATING POINT CODING RULES

For the problems that require you to implement floating-point operations,
the coding rules are less strict.  You are allowed to use looping and
conditional control.  You are allowed to use both ints and unsigneds.
You can use arbitrary integer and unsigned constants. You can use any arithmetic,
logical, or comparison operations on int or unsigned data.

You are expressly forbidden to:
  1. Define or use any macros.
  2. Define any additional functions in this file.
  3. Call any functions.
  4. Use any form of casting.
  5. Use any data type other than int or unsigned.  This means that you
     cannot use arrays, structs, or unions.
  6. Use any floating point data types, operations, or constants.


NOTES:
  1. Use the dlc (data lab checker) compiler (described in the handout) to 
     check the legality of your solutions.
  2. Each function has a maximum number of operations (integer, logical,
     or comparison) that you are allowed to use for your implementation
     of the function.  The max operator count is checked by dlc.
     Note that assignment ('=') is not counted; you may use as many of
     these as you want without penalty.
  3. Use the btest test harness to check your functions for correctness.
  4. Use the BDD checker to formally verify your functions
  5. The maximum number of ops for each function is given in the
     header comment for each function. If there are any inconsistencies 
     between the maximum ops in the writeup and in this file, consider
     this file the authoritative source.

/*
 * STEP 2: Modify the following functions according the coding rules.
 * 
 *   IMPORTANT. TO AVOID GRADING SURPRISES:
 *   1. Use the dlc compiler to check that your solutions conform
 *      to the coding rules.
 *   2. Use the BDD checker to formally verify that your solutions produce 
 *      the correct answers.
 */


#endif
//1
/* 
 * bitXor - x^y using only ~ and & 
 *   Example: bitXor(4, 5) = 1
 *   Legal ops: ~ &
 *   Max ops: 14
 *   Rating: 1
 */
int bitXor(int x, int y) {
    // xor运算的定义：x^y = (~x&y)|(x&~y)
    // 其中x|y = ~(~x&~y)
    int a = ~x&y;
    int b = x&~y;
    return ~(~a&~b);
}
/* 
 * tmin - return minimum two's complement integer 
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 4
 *   Rating: 1
 */
int tmin(void) {
    // 左移n位，右边就会多出n个零，故只需左移31位。
    // 因为上面说了我们可以假设int占32位，所以直接使用常量即可，不必去求int的大小。
    return 1<<31;
}
//2
/*
 * isTmax - returns 1 if x is the maximum, two's complement number,
 *     and 0 otherwise 
 *   Legal ops: ! ~ & ^ | +
 *   Max ops: 10
 *   Rating: 1
 */
int isTmax(int x) {
    // return x == ~(1<<31);
    // 错误的实现，不能使用==运算。
    // 左移一位再加一，而左移也就是*2，x*2=x+x。
    int a = x+x+1; // 仅当x为Tmax时，可以通过该表达式构造出-1。
    // 上面的表达式会给x==-1钻空子，所以要检查一下：
    return !!(x^-1) & !(a+1);
}
/* 
 * allOddBits - return 1 if all odd-numbered bits in word set to 1
 *   where bits are numbered from 0 (least significant) to 31 (most significant)
 *   Examples allOddBits(0xFFFFFFFD) = 0, allOddBits(0xAAAAAAAA) = 1
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 12
 *   Rating: 2
 */
int allOddBits(int x) {
    // 使用的int常量的范围是[0, 255]，255即0xFF。
    // 利用与运算，所有，只要有一个奇数位不为1，则为0。
    int a = x & (x>>16);
    a = a & (a>>8);
    a = a & (a>>4);
    a = a & (a>>2);
    return !!(a&2);
}
/* 
 * negate - return -x 
 *   Example: negate(1) = -1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 5
 *   Rating: 2
 */
int negate(int x) {
    // 在模2^n系统中，一个数x的相反数-x=2^n-x，将该式子化简后，也就是对x取反加1。
    return (~x)+1;
}
//3
/* 
 * isAsciiDigit - return 1 if 0x30 <= x <= 0x39 (ASCII codes for characters '0' to '9')
 *   Example: isAsciiDigit(0x35) = 1.
 *            isAsciiDigit(0x3a) = 0.
 *            isAsciiDigit(0x05) = 0.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 15
 *   Rating: 3
 */
int isAsciiDigit(int x) {
    // 检查第二个字节为3，第一个字节不大于9即可。9==0b1001，大于9的数，第3位为1，且第1或2位不为0。
    // 仍然利用xor运算，相同为0的性质。
    // 注意还利用了&运算，保证在第3位为1的情况下，若第1、2位不为0才产出非0，否则第3位为0，即使第1、2位不为0，此时也会产出0。
    int a = (x>>3)&1;
    int b = (x>>1)&3; // 在a为1的前提下，若x是ascii digit，则b为0，否则b为01, 10, 11。
    return !(((x>>4)^3) | (a & b) | (a<<1 & b));
}
/* 
 * conditional - same as x ? y : z 
 *   Example: conditional(2,4,5) = 4
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 16
 *   Rating: 3
 */
int conditional(int x, int y, int z) {
    // https://wdxtub.com/csapp/thick-csapp-lab-1/2016/04/16/
    /*
    *if x!=0,mask=0x00000000,so y&~mask==y and z&mask==0
    *if x==0,mask=0xffffffff,so y&~mask==0 and z&mask==z
    */
    int mask= ~!x+1; // 关键：将条件x转换为全0或全1的掩码。
    return (y & ~mask)|(z & mask);
}
/* 
 * isLessOrEqual - if x <= y  then return 1, else return 0 
 *   Example: isLessOrEqual(4,5) = 1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 24
 *   Rating: 3
 */
int isLessOrEqual(int x, int y) {
    // https://wdxtub.com/csapp/thick-csapp-lab-1/2016/04/16/
    // 要考虑符号。
    // 注意开头让我们假设右移是算术右移，补的是符号位。所以若一个数为负数，通过右移得到的sign=-1。
    // int sign_x = x>>31;
    // int sign_y = y>>31;
    // 正确应该的表达式如下：
    int sign_x = (x>>31)&1;
    int sign_y = (y>>31)&1;
    // 不要陷入一个误区，认为比较就是要按每一位比较大小，
    // 实际上，我们可以通过将x, y做某些运算，从运算结果**侧面反映**出两者的大小关系。
    // 后者大大简化了问题，这里的运算可以选择做减法，在补码表示/模运算系统中，x-y=x+(~y+1)

    // 若符号不同，正数大于负数。
    int r1 = (sign_x^sign_y) & sign_x;
    // 若符号相同，看x-y，若x-y<=0，则x<=y。即结果为0，或符号为1。
    int a = x+(~y+1);
    int r2 = (!(sign_x^sign_y)) & ((!(a^0)) | (a>>31));
    return r1 | r2;

    // (a>>31)&1 等价于 a>>31。
}
//4
/* 
 * logicalNeg - implement the ! operator, using all of 
 *              the legal operators except !
 *   Examples: logicalNeg(3) = 0, logicalNeg(0) = 1
 *   Legal ops: ~ & ^ | + << >>
 *   Max ops: 12
 *   Rating: 4 
 */
int logicalNeg(int x) {
    // 只要有1就返回0，只要，用或运算。
    // 另一个想法是，全为0就返回1，全，用与运算，但是这里0与与运算互斥。
    int a = x | (x>>16);
    a = a | (a>>8);
    a = a | (a>>4);
    a = a | (a>>2);
    a = a | (a>>1);
    return !a;
}
/* howManyBits - return the minimum number of bits required to represent x in
 *             two's complement
 *  Examples: howManyBits(12) = 5
 *            howManyBits(298) = 10
 *            howManyBits(-5) = 4
 *            howManyBits(0)  = 1
 *            howManyBits(-1) = 1
 *            howManyBits(0x80000000) = 32
 *  Legal ops: ! ~ & ^ | + << >>
 *  Max ops: 90
 *  Rating: 4
 */
int howManyBits(int x) {
    // https://wdxtub.com/csapp/thick-csapp-lab-1/2016/04/16/
    int absX = x^(x>>31); // get absolute of x，借助算术右移和xor运算的性质。
    int isZero = !absX;
    // notZeroMask==-1 if x!=0 else 0。由此，不能简单地用-1直接赋值给notZeroMask，而要根据absX计算出notZeroMask。
    int notZeroMask = ((!!absX)<<31)>>31;
    int bit_16, bit_8, bit_4, bit_2, bit_1;
    bit_16 = !!(absX>>16)<<4; // 如果高16位有值，则至少需要16=2^4=1<<4个位来表示x。
    // see if the high 16 bits have value, if have,then we need at least 16 bits.
    // if the highest 16 bits have value, then right shift 16 to see the exact place,
    // otherwise they are all zero, right shift nothing and we should only consider the low 16 bits.
    absX >>= bit_16; // bit_16或为0或为16。
    bit_8 = !!(absX>>8)<<3; // 若bit_16为16，则考察高16位的高8位，否则考察低16位地高8位。若有值，则至少需要2^3+bit_16个位来表示x。
    absX >>= bit_8;
    bit_4 = !!(absX>>4)<<2;
    absX >>= bit_4;
    bit_2 = !!(absX>>2)<<1;
    absX >>= bit_2;
    bit_1 = !!(absX>>1);
    int nBits = bit_16+bit_8+bit_4+bit_2+bit_1+2; // at least we need one bit for 1 to tmax,
    // and we need another bit for sign.
    return isZero | (notZeroMask & nBits);

    // 题外话：
    // x^0=x, x^x=0, 由此x^x^x=x，即奇数个x抑或得到x本身，偶数个x抑或得到0。
    // x^(-1)相当于对x的每一位取反。
    // 要明白0和1在二进制上的区别，0=000...000，1=000...001。
    // !运算将非0->1, 0->1。
    // !!运算将非0->1, 0->0。
}
//float
/* 
 * floatScale2 - Return bit-level equivalent of expression 2*f for
 *   floating point argument f.
 *   Both the argument and result are passed as unsigned int's, but
 *   they are to be interpreted as the bit-level representation of
 *   single-precision floating point values.
 *   When argument is NaN, return argument
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
unsigned floatScale2(unsigned uf) {
    // You can use arbitrary integer and unsigned constants.
    unsigned sign = (uf>>31)&1;
    unsigned exp = (uf>>23)&0xFF; // 怎么快速知道右移23位，想象成挤掉右边23位的frac即可。
    unsigned frac = uf&0x7FFFFF;
    if (exp==0 && frac==0) {
    return uf; // 0
    }
    if (exp==0xFF) {
    // 不管还是NaN(frac!=0)还是Infinity(frac==0)，都返回uf。
    return uf; 
    }
    int denorm = (exp==0);
    if (denorm) {
    frac += frac;
    } else {
    exp = exp+1;
    }
    // printf("sign=%x, exponent=%x, fraction=%x\n", sign, exp, frac);
    return sign<<31 | exp<<23 | frac;
}
/* 
 * floatFloat2Int - Return bit-level equivalent of expression (int) f
 *   for floating point argument f.
 *   Argument is passed as unsigned int, but
 *   it is to be interpreted as the bit-level representation of a
 *   single-precision floating point value.
 *   Anything out of range (including NaN and infinity) should return
 *   0x80000000u.
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
int floatFloat2Int(unsigned uf) {
    unsigned sign = (uf>>31)&1;
    unsigned exp = (uf>>23)&0xFF;
    unsigned frac = uf&0x7FFFFF;
    if (exp==0xFF) {
    // NaN or Infinity.
    return 0x80000000u;
    }
    int bias = (1<<7)-1;
    int denorm = (exp==0);
    int uexp = denorm? 1-bias: exp-bias;
    int mantissa = denorm? frac: frac|(1<<23);
    // 巧妙之处在于，我们认为mantissa已经是M左移23位的结果了。
    uexp -= 23;
    if (uexp > 0) {
    if (uexp>31) {
      return 0x80000000u; // 注意，如果左移或右移位数小于0或大于31，结果是不确定的。
    }
    mantissa <<= uexp;
    } else if (uexp < 0)  {
    if (uexp<-31) {
      return 0; // 注意，如果左移或右移位数小于0或大于31，结果是不确定的。
    }
    mantissa >>= (-uexp); // 右移，除以2^(-uexp)。
    }
    return sign? (~mantissa+1): mantissa;
}
/* 
 * floatPower2 - Return bit-level equivalent of the expression 2.0^x
 *   (2.0 raised to the power x) for any 32-bit integer x.
 *
 *   The unsigned value that is returned should have the identical bit
 *   representation as the single-precision floating-point number 2.0^x.
 *   If the result is too small to be represented as a denorm, return
 *   0. If too large, return +INF.
 * 
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. Also if, while 
 *   Max ops: 30 
 *   Rating: 4
 */
// ./btest -f floatPower2 -T 15
// 如果限时10s会超时。
unsigned floatPower2(int x) {
    // 求2.0^x，其实等价于1.0*2^E，其中S==0, M==1.0, frac==0。
    unsigned sign = 0;
    unsigned frac = 0;
    // unsigned exp = x+127; // 不能用unsigned，这里exp不是从给定的浮点数中提取出来的。
    int exp = x+127;
    if (exp>=0XFF) {
    return 0x7f800000; // Infinity，即exp==0xFF, frac==0。
    }
    if (exp<0) {
    return 0;
    }
    return sign<<31 | exp<<23 | frac;

    // 1<<i，得到的结果是第i位上为1，i是0-based。
}

// 实现(float)x，首先确定符号，再从高位开始，找到第一个1：
// int i = 31;
// int mask = 1<<31;
// while (i>=0) {
//   if (x&mask!=0) break;
//   i-=1;
//   mask>>=1;
// }
// 这里因为0<=i<=31，所以转换过来的浮点数都是规格化的，然后
// 设置exp=i+127;frac=x&((1<<i)-1)，最后拼起来。
// 这里其实还要对frac进行舍入，因为frac只能占23位，但它可能大于23位。
// 这边要做的是向偶数舍入，所以只有在XXXX.YYYYY100的形势时，舍入的位置是最后一个Y，并且只有紧随其后的1个1，才要做向偶数舍入，就是让最后一个Y为0.
// 如果不是这些情况，那么就看舍掉的最高位的那个是1还是0,1就进位。
