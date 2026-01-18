---
title: '用户自定义字面量：给单位和领域值一个“类型入口”'
description: 'C++11 的用户自定义字面量让 10ms、42km 这种表达变成类型安全的代码，也让“单位”不再只是注释。'
---

十几年前。

大家写 C。

后来写 C with classes。

代码里最常见的“类型”。

就俩。

`int`。

`double`。

数字一多。

单位就开始飘。

今天你写的是“500”。

明天别人读成了“500”。

然后线上啪一下。

大家开始对着日志发呆。

直到有人问了一句。

“这 500 到底是啥单位？”

### 线上啪一下：一个最小复现

我当年踩过一次。

很小的项目。

一个小服务。

就一个配置项：超时。

灰度一上。

接口忽快忽慢。

日志看不出毛病。

最后发现。

问题就藏在“500”里。

```cpp
void set_timeout_seconds(int seconds);

int timeout = 500;              // 我脑子里是“毫秒”
set_timeout_seconds(timeout);   // 但接口要的是“秒”
```

这段代码看起来很正常。

也很 C。

但它会让你的超时变成 500 秒。

你的监控会先红。

你的人会后红。

### 当年的人怎么想：先靠约定

一开始大家的解决方案很朴素。

把单位写进变量名。

```cpp
void set_timeout_us(int us);

int timeout_ms = 500;

int timeout_us = timeout_ms * 1000;
set_timeout_us(timeout_us);
```

这样“看起来”清楚一点。

但你还是得手动乘。

而且一忙起来。

你就会把 `_ms` 忘掉。

### 再后来：靠宏，靠祈祷

有人说。

那就写个宏吧。

```cpp
#define MS(x) ((x) * 1000)

set_timeout_us(MS(timeout_ms));
```

调用点确实短了。

但宏不进类型系统。

编译器也帮不上你。

你写错了。

它照样给你编过去。

而且宏还有一堆老毛病。

括号没套好就炸。

表达式被求值两次也炸。

你写着写着。

就又回到了“靠经验”。

关键问题一直没变。

单位只存在于人的脑子里。

### 另一个更早的来源：其实 C 就已经在用“后缀”了

你可能没把它当一回事。

但你其实天天在写。

```cpp
auto a = 10u;     // unsigned
auto b = 10ull;   // unsigned long long
auto c = 3.14f;   // float
```

这些都是“字面量后缀”。

只不过。

它们是语言内置的。

你不能自己发明一个。

而 C++11 的用户自定义字面量。

可以理解成。

把这套后缀机制开放给你。

你不只是在说“这是 unsigned”。

你还可以说。

“这是毫秒”。

“这是 KB”。

甚至“这是业务里的钱”。

很多语言也都有类似的传统。

比如 Java 有 `1L`。

C# 有 `1m`。

都是同一个味道。

### 你可能会问：那会儿就没人想把单位“类型化”吗

当然有人想。

而且招数也不少。

只是每一招都有代价。

最早的版本。

就是靠约定和注释。

你已经见过了。

变量名写 `_ms`。

注释写“单位是毫秒”。

能救。

但救得不稳。

后来有人更狠。

把单位直接做成一个类型。

```cpp
struct Milliseconds {
    int v;
};

void set_timeout(Milliseconds t);

set_timeout(Milliseconds{500});
```

这招的好处是。

你很难把毫秒当秒传进去。

因为类型不一样。

但它也很啰嗦。

调用点全是大括号。

你写多了会烦。

别人看多了也烦。

再后来。

社区开始做库。

比如 Boost 里就有一整套时间类型。

还有更“硬核”的 Boost.Units。

它会把米、秒、千克这种东西都做成类型。

代价也很明显。

类型很漂亮。

模板也很厚。

对刚起步的小项目来说。

有点像“为了装一个门铃，先买一台挖掘机”。

### C++11 给的一条路：让数字带上后缀

到 C++11 的时候。

标准委员会终于给了一个挺“语言层面”的招。

用户自定义字面量。

先别被名字吓到。

我拆开说。

“字面量”就是你在代码里直接写死的那个东西。

比如 `42`、`3.14`、`'a'`、`"hi"`。

“自定义”就是你可以给它加个尾巴。

让编译器看到这个尾巴，就走你写的规则。

一句话。

把单位写进语法里。

### 顺便补一段历史：`<chrono>` 从哪儿来

如果你听过一句老话。

标准库里很多东西，都是先在 Boost 里长大的。

`<chrono>` 也差不多。

它的整体思路，很像 Boost.Chrono 那一套。

先把“时间长度”变成一个类型。

比如 `std::chrono::milliseconds`。

然后让不同单位之间的换算。

交给库。

你不再手写 `* 1000`。

你写。

```cpp
auto t = std::chrono::milliseconds(500);
```

它已经比 C 时代的 `int timeout = 500;` 强很多了。

但你会发现。

它还是“像在调用函数”。

而不是“像在写数字”。

### 自己做一个 `_ms`

我们先做最常见的。

毫秒。

```cpp
#include <chrono>

constexpr std::chrono::milliseconds operator"" _ms(unsigned long long v) {
    return std::chrono::milliseconds(v);
}
```

这里的关键是 `operator"" _ms`。

它的意思是：当你写 `123_ms` 时，把 `123` 交给这个函数。

返回值不是 `int`。

是一个明确的类型：`std::chrono::milliseconds`。

然后你就能写。

```cpp
auto timeout = 500_ms;
```

读代码的人不用猜。

编译器也不用猜。

### 把语法拆开：`500_ms` 到底干了啥

你可以把它当成一种“编译器帮你写函数调用”。

就像这样。

```cpp
auto timeout = operator"" _ms(500ULL);
```

所以你写的不是魔法。

只是少打了几个字。

而且少打的那几个字。

恰好把“单位”钉死了。

### 为什么参数是 `unsigned long long`

因为你写的是整数。

`500_ms` 里的 `500` 是一个整数“字面量”。

标准规定。

这种字面量交给字面量运算符时。

会以 `unsigned long long` 的形式传进来。

你不用纠结。

把它当成“足够大的无符号整数”就行。

如果你写的是小数。

对应的入口通常是 `long double`。

```cpp
constexpr double operator"" _deg(long double v) {
    return static_cast<double>(v) * 3.141592653589793 / 180.0;
}
```

如果你写的是字符串。

入口一般是“字符指针 + 长度”。

```cpp
int operator"" _tag(const char* s, std::size_t n);
```

你现在不需要把这些都背下来。

你只要记住。

你写的字面量是什么形状。

就会走到对应形状的那个 `operator""`。

顺便一提。

你当然可以写负数。

```cpp
auto t = -500_ms;
```

这里的负号是外面的。

先算出 `500_ms`。

再取负。

### `constexpr` 是干嘛的

你在 C 里可能没见过这个词。

我也不想一上来讲定义。

你先记一个直觉。

能在编译期算出来的东西。

就别拖到运行期。

```cpp
constexpr auto t = 500_ms;
```

这行的味道很像。

`constexpr int x = 500;`

它在告诉编译器。

这玩意是个“常量”。

能提前算就提前算。

你写 UDL 的时候把它标成 `constexpr`。

通常不会吃亏。

### 一个很容易忽略的规则：后缀为什么要以下划线开头

你可能见过标准库写 `500ms`。

没有下划线。

那是标准库的特权。

对我们自己写的 UDL。

不以下划线开头的后缀。

是留给“未来标准库可能会用”的。

所以你自己写。

老老实实用 `_ms`、`_KB` 这种。

省得哪天升级编译器。

突然撞车。

### 你不一定要自己写：C++14 以后标准库就送了

这点很多新手会误会。

以为 `500ms` 是 C++11 的东西。

其实不是。

C++11 给的是“机制”。

标准库真正把 `ms/s/min/h` 这些后缀送进来。

是在 C++14 以后。

用法大概长这样。

```cpp
#include <chrono>
using namespace std::chrono_literals;

auto t1 = 500ms;
auto t2 = 2s;
```

你会发现。

标准库的写法更短。

但它也要求你把那个命名空间“引进来”。

这是一种交换。

短一点。

也更容易命名冲突一点。

### 它怎么把坑“堵死”：让错误更难写出来

假设你给接口定的是毫秒。

```cpp
void set_timeout(std::chrono::milliseconds t);

set_timeout(500_ms);
```

这时候你同事再想写 `set_timeout(500)`。

编译器会直接拦下来。

它不是在挑剔。

它是在帮你们统一口径。

有人把这类设计总结成一句英文。

Make illegal states unrepresentable。

翻成大白话就是。

别让“错的东西”那么容易写出来。

### 再做一个 `_KB`

另一类高发区是内存大小。

你写 `4096`。

别人不知道你要的是字节还是 KB。

```cpp
#include <cstddef>

constexpr std::size_t operator"" _KB(unsigned long long v) {
    return static_cast<std::size_t>(v) * 1024;
}
```

然后你就能写。

```cpp
auto buf_size = 4_KB;
```

这行代码读起来像一句话。

而不是一道心算题。

### 再给你几个“真会出事”的例子

很多时候。

你不是缺一个 `_ms`。

你是缺一个“把领域词塞进类型系统的入口”。

比如角度。

你在图形学里常见的坑。

是把“度”和“弧度”混了。

```cpp
constexpr double operator"" _deg(long double v) {
    return static_cast<double>(v) * 3.141592653589793 / 180.0;
}

double rad = 90.0_deg;
```

你不需要记公式。

读代码的人也不用猜。

再比如网络协议里的大小。

有人喜欢写“最多收 1MB”。

然后代码里出现一个 `1048576`。

顺手提醒一句。

很多系统里口头说的 “1MB”。

实际按的是 `1024 * 1024`。

这也是单位容易打架的原因之一。

```cpp
constexpr std::size_t operator"" _MB(unsigned long long v) {
    return static_cast<std::size_t>(v) * 1024 * 1024;
}

auto max_body = 1_MB;
```

你看见 `1_MB`。

脑子里就不会把它当成“一个神秘的 1048576”。

再比如距离。

你可能写过这种东西。

“3 公里”。

最后代码里变成了 `3000`。

```cpp
constexpr std::size_t operator"" _km(unsigned long long v) {
    return static_cast<std::size_t>(v) * 1000;
}

auto distance = 3_km;
```

你不用在心里补单位。

读者也不用猜。

还有一种更“业务”的。

钱。

新手最容易把 `double` 当钱用。

然后被 0.1 折磨一辈子。

你可以先从“分”开始。

```cpp
struct Cents {
    long long v;
};

constexpr Cents operator"" _cents(unsigned long long v) {
    return Cents{static_cast<long long>(v)};
}

auto fee = 199_cents;
```

这不是说 UDL 能解决金融系统。

它只是提醒你。

把领域值当 `double`。

迟早要还债。

### 你要知道的边界：它不是“给所有数字贴标签”

用户自定义字面量只对“字面量”生效。

也就是说。

你可以写 `500_ms`。

但你不能指望一个运行时变量自动变成 `n_ms`。

运行时的数。

老老实实用构造函数就行。

```cpp
int n = 500;
auto t = std::chrono::milliseconds(n);
```

这也是为什么。

UDL 最适合用在“常量”。

配置默认值。

协议常量。

测试用例里的固定数字。

那种你希望读代码就能看懂的地方。

### 另一个新手常踩的点：作用域和命名污染

你把 `operator"" _ms` 放到全局。

当然最方便。

也最容易让全工程都带上它。

更稳一点的写法是。

把它放进一个小命名空间。

需要的时候再引进来。

```cpp
namespace units {
constexpr std::chrono::milliseconds operator"" _ms(unsigned long long v) {
    return std::chrono::milliseconds(v);
}
}

using units::operator"" _ms;
```

你不需要一上来就理解“名字查找规则”。

你只要记住。

让后缀可控。

比让后缀满天飞要好。

字面量后缀更适合用在。

单位。

协议常量。

领域值。

那些“一旦误读就会出事”的地方。

### 真正的价值：它解决的不是语法，是沟通

最贵的 bug 往往不是“算错”。

是“理解不一致”。

你以为的毫秒。

和他以为的秒。

都很合理。

但系统不管你们谁合理。

系统只管结果。

一句话收尾。

编译器不懂你的注释。

但它懂你的类型。
