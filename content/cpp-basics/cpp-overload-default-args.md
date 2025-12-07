---
title: "重载与默认参数：让接口更灵活"
description: "讲清楚函数重载的规则（为什么返回类型不能区分重载），以及默认参数如何简化接口设计。"
order: 40
---

写 C 的时候，想给 `int` 和 `double` 各写一个"求绝对值"函数？  
不好意思，你得老老实实起两个不同的名字。

到了 C++，事情就轻松多了——你可以让好几个函数共用同一个名字，只靠参数列表来区分。这就是**函数重载**。

还有一种场景：有些函数的某个参数，十次调用有九次都传一样的值。**默认参数**就是让你可以把这个"常见值"写进签名里，省得每次都敲一遍。

这篇文章会聊清楚几件事：

- 重载到底怎么玩？
- 为啥返回类型不能用来区分重载？（这个坑很多人踩过）
- 默认参数的规则和注意事项
- 重载和默认参数，什么时候用哪个？


---

## 函数重载

### 先说说 C 语言里有多累

在 C 里，你想给不同类型写"求绝对值"，只能这么搞：

```cpp
int abs_int(int x) {
    return (x >= 0) ? x : -x;
}

double abs_double(double x) {
    return (x >= 0) ? x : -x;
}
```

调用的时候，你得自己记住现在用的是哪个版本：

```cpp
int    ai = abs_int(-5);
double ad = abs_double(-3.14);
```

从机器的角度看，这完全 OK。  
但从人类的角度看——有点烦。

明明都是"求绝对值"这一件事，我为啥要在函数名里反复强调类型？


### C++ 说：名字可以一样，参数不同就行

C++ 的思路是：**用"函数名 + 参数类型"来区分不同版本**，而不是只看名字。

于是你可以这么写：

```cpp
int abs(int x) {
    return (x >= 0) ? x : -x;
}

double abs(double x) {
    return (x >= 0) ? x : -x;
}
```

调用的时候，就很自然了：

```cpp
int    ai = abs(-5);      // 走 int 版本
double ad = abs(-3.14);   // 走 double 版本
```

编译器看到 `abs(-5)`，发现你传的是 `int`，就去找 `abs(int)`；  
看到 `abs(-3.14)`，发现你传的是 `double`，就去找 `abs(double)`。

这个"编译器帮你挑版本"的过程，有个正经名字叫**重载决议（overload resolution）**。


### 参数个数不同也能重载

除了类型不同，参数个数不同也可以构成重载。

比如写几个"算平均值"的函数：

```cpp
double average(double a, double b) {
    return (a + b) / 2.0;
}

double average(double a, double b, double c) {
    return (a + b + c) / 3.0;
}
```

调用的时候：

```cpp
double m2 = average(10.0, 20.0);
double m3 = average(10.0, 20.0, 30.0);
```

编译器数一数你传了几个参数，就知道该调哪个版本了。


### 返回类型不能区分重载

这里有个坑，很多人都踩过：**返回类型不能用来区分重载**。

下面这两个声明，不能同时存在：

```cpp
int    parse(const std::string& text);
double parse(const std::string& text);  // 编译报错：光返回类型不同不行
```

为啥？你想想这个场景：

```cpp
parse("123");  // 我就调一下，返回值不要了
```

编译器懵了——你到底想要 `int` 版本还是 `double` 版本？它猜不出来。

所以 C++ 规定：函数的"签名"只看**名字 + 参数类型列表**，返回类型不算。


### 重载的设计原则

从设计角度看，好的重载应该是**"同一件事的不同形态"**。

比如：

- `abs(int)` 和 `abs(double)` —— 都是"求绝对值"
- `print(int)` 和 `print(const std::string&)` —— 都是"打印"
- `average(a, b)` 和 `average(a, b, c)` —— 都是"求平均值"

如果两个函数的行为在语义上完全不一样，哪怕参数列表刚好也不同，也别硬塞到同一个名字下面。

不然读代码的人会很崩溃："这俩 `process()` 到底在干嘛？"


---

## 默认参数

### 为啥需要它？

有些函数，存在一个"最常见的用法"。每次都把所有参数写全，其实挺啰嗦的。

举个例子：你写了一个画分隔线的函数，可以指定长度和用什么字符来画。

```cpp
void draw_line(int length, char ch) {
    for (int i = 0; i < length; ++i) {
        std::cout << ch;
    }
    std::cout << '\n';
}
```

用起来大概是这样：

```cpp
draw_line(10, '-');
draw_line(5,  '*');
```

但你仔细想想自己的调用习惯——十次有九次都是用 `'-'` 画分隔线，只有偶尔才换成别的字符。

每次都写 `'-'`，是不是有点烦？


### 默认参数登场

能不能省掉第二个参数，让它默认就是 `'-'`？

可以，这就是默认参数干的事：

```cpp
void draw_line(int length, char ch = '-') {
    for (int i = 0; i < length; ++i) {
        std::cout << ch;
    }
    std::cout << '\n';
}
```

`char ch = '-'` 的意思是：如果调用方没传 `ch`，就自动补上 `'-'`。

于是调用方式就多了一种简写：

```cpp
draw_line(10);        // 等价于 draw_line(10, '-')
draw_line(5, '*');    // 想换字符？手动指定就行
```


### 默认参数的规则

用默认参数有几条规矩，不遵守的话编译器会教你做人。

**1. 默认值必须从右往左连续**

```cpp
void foo(int a, int b = 10, int c = 20);  // OK
void foo(int a = 10, int b, int c = 20);  // 报错：b 没默认值，但左边的 a 有
```

原因很简单：调用时参数是从左往右填的。  
如果中间有"空洞"，编译器不知道你省略的到底是哪一个。

**2. 默认值在每次调用时求值**

```cpp
void foo(int x = 10);                    // OK：字面量
void foo(int x = some_global_var);       // OK：全局变量
void foo(int x = compute_something());   // 注意：每次调用都会执行这个函数
```

第三种写法要小心，`compute_something()` 不是只算一次，而是每次调用 `foo()` 都会跑一遍。

**3. 头文件和源文件分开写时，默认参数只写一次**

如果你把声明放在头文件、定义放在源文件，**默认参数只写在声明那边**：

```cpp
// utils.hpp
void draw_line(int length, char ch = '-');  // 声明带默认参数

// utils.cpp
void draw_line(int length, char ch) {       // 定义不重复写
    for (int i = 0; i < length; ++i) {
        std::cout << ch;
    }
    std::cout << '\n';
}
```

两边都写的话，万一写得不一样，就等着看编译器报错吧。


### 签名其实还是一个

从类型系统的角度看，带默认参数的函数仍然只有一个签名：`draw_line(int, char)`。

编译器只是在调用点帮你把缺的参数补上，函数本身并没有变成好几个版本。

这和重载不一样——重载是真的有多个不同签名的函数存在。


---

## 重载 vs 默认参数

什么时候用重载，什么时候用默认参数？

### 适合用默认参数的场景

- 某个参数有一个"最常见的值"，偶尔才需要改
- 不同调用之间，函数的行为本质上是一样的

```cpp
void log(const std::string& msg, LogLevel level = LogLevel::Info);
```

大部分日志都是 Info 级别，偶尔才需要 Warning 或 Error。用默认参数刚刚好。


### 适合用重载的场景

- 参数类型完全不同
- 不同版本的实现逻辑差异较大

```cpp
void print(int x);
void print(double x);
void print(const std::string& s);
```

这三个 `print` 内部实现可能完全不一样，用重载更合适。


### 有时候两者都行

比如前面的 `average` 例子，用重载：

```cpp
double average(double a, double b);
double average(double a, double b, double c);
```

也可以用默认参数来模拟——但会很别扭：

```cpp
// 这种写法不太好
double average(double a, double b, double c = ???);  // c 默认是啥？0？那算出来不对啊
```

"第三个参数可选"这种语义，重载表达得更清晰。


---

## 小结

**重载**让你可以用同一个名字表达"同一件事的不同形态"：

- 靠参数类型或参数个数来区分
- 返回类型不能用来区分（这是个坑）
- 好的重载在语义上应该是同一件事

**默认参数**让你把"最常见的用法"固化到签名里：

- 省略的参数会被编译器自动补上
- 默认值必须从右往左连续
- 头文件和源文件分开写时，只在声明侧写一次

这两个特性都是为了让接口更灵活、调用更自然。  
选哪个？看你想表达的是"同一件事的多种形态"，还是"大部分时候用这个值"。
