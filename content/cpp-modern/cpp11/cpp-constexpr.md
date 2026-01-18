---
title: 'constexpr 与常量表达式：把“能算的”提前算掉'
description: '从一次“按位与替代取模”的线上坑讲起：用 C++11 的 constexpr 和 static_assert 把前提写进编译器，让错误尽量在编译期爆掉。'
---

那会儿你写一个“常量”。

多半就是一行 `#define`。

写上去。

编译过。

就当它永远不会变。

直到有一天。

它变了。

而你是在凌晨两点才知道的。

### 一个小项目，线上啪一下

当年我们做过那种“小得不能再小”的服务。

一个进程，里面塞了一个环形缓冲区。
你可以把它想成一个数组，写到头了就绕回去。

我们拿它放点日志，放点指标。
平时看起来很乖。

上线一压，数据开始“少一截”。
你以为是并发，也以为是指针，甚至怀疑过硬盘。

最后发现。

是一个写着“常量”的东西。

### 没有它之前，我们怎么凑合

那时大家对“编译器能帮我检查”这件事，期待值很低。
编译器能替你看住边界？想多了。

更多时候，我们是在跟未来的人做口头交易。
我把前提写在注释里，你以后改代码时，麻烦你记得它。

```cpp
#include <cstddef>

#define BUF_SIZE 1024 // 约定：必须是 2 的幂
#define MASK (BUF_SIZE - 1)

inline std::size_t wrap(std::size_t i) {
    return i & MASK;
}
```

顺手说一句：`std::size_t` 就是 C 里的 `size_t`，常用来放大小和下标。

这里的“小聪明”很老派。
当 `BUF_SIZE` 是 2 的幂时，`i & MASK` 等价于 `i % BUF_SIZE`。
所谓 2 的幂，就是 1、2、4、8、16 这种数。

于是我们把 `%` 换成了 `&`，然后心里默念一句：别改它。

（有人会顺口丢一句 Knuth：*premature optimization is the root of all evil*。
但当年你写性能路径时，真没空听劝。）

### 坑是怎么踩出来的

后来需求来了。
产品说：缓冲区别写死了，给个配置吧。
于是“常量”从宏，变成了变量。

但那行 `&` 没人动。
你以为只是把数字挪到配置里。

```cpp
#include <cstddef>

std::size_t kBufSize = 1000; // 来自配置
std::size_t kMask = kBufSize - 1;

std::size_t wrap(std::size_t i) {
    return i & kMask;
}
```

这段代码的问题不在语法，问题在前提。
`kBufSize` 不是 2 的幂时，`&` 就不再等价于 `%`。

更阴的是，它不一定立刻炸。
你本地跑几次看不出来，线上一忙，索引绕回的点开始飘，数据就开始丢。

### 当年我们怎么补锅

补锅当然可以很粗暴。
比如加一句运行时检查，不满足就直接拒绝启动。

```cpp
#include <cstddef>
#include <cstdlib>

std::size_t kBufSize = 1000; // 来自配置

if ((kBufSize & (kBufSize - 1)) != 0) {
    std::abort();
}
```

这能救命，但它也很脆。
你总能在某个版本里看到这样的改动：

“这个检查影响启动时间，先去掉吧。”

然后坑就又回来了。

### 新手最容易误会的一点：`const` 不等于“编译期”

学完 C 以后，你很容易觉得：那我不用宏了。

我写 `const`。

不就行了吗。

```cpp
const int kN = 16;
int a[kN];
```

这个例子在右边是字面量、编译器能直接算出来时，通常是能过的。
因为它碰巧也是“编译期就能确定的整数”。

新手真正会踩的坑，是下面这种。

```cpp
#include <cstddef>

std::size_t read_config();

const std::size_t kBufSize = read_config();

int buf[kBufSize];
```

你写了 `const`。
你也确实“不打算改”。

但编译器还是会摇头。
因为 `read_config()` 这件事，得等程序跑起来才知道结果。

你可能还会遇到另一种“更气人”的情况。
有些编译器会把某些写法当扩展放行，让你误以为标准就是这么玩的。

在 C++ 里，`const` 更像一句话：这块内存别改。
而“编译期就能算出来”，是另一句更苛刻的话。

这两句话，经常同时出现。
但它们不是一回事。

### C++11 之前，我们都用过哪些土办法

没有 `constexpr` 的年代，大家其实一直在找“编译期常量”。

只不过手法很土。

也很分裂。

### 横着对比一下：它们到底差在哪

如果你刚学完 C++，最容易被绕晕的不是语法。

是同一件事有五六种写法。

而且每种写法都有人说“这是正统”。

我给你一个工程视角的分法。

宏是“最早发生的替换”。
它能解决“必须是编译期”的问题，但它不讲类型，也不讲作用域。

`const` 更像“运行时的承诺”。
它说的是：这块内存别改。
它有时候也能碰巧满足编译期，但你不能指望它每次都满足。

`enum` 是老派的“编译期整数”。
你要一个小整数常量、还要编译期，`enum` 很好用。
但它只能放整数，也写不出过程。

模板元编程是“把计算藏进类型系统”。
它威力很大，但也最容易把你带进黑魔法。
你写得越像数学，编译器给你的报错越像天书。

`constexpr` 的目标更朴素。
它想把“编译期能算”这件事，变成你能正常写的函数、正常写的常量。
能在编译期用，就在编译期用。
需要在运行时用，它也不拦你。

`static_assert` 则是把门的。
你不拿它算东西。
你拿它把前提钉死。

#### 宏：最便宜，也最不讲理

```cpp
#define BUF_SIZE 1024
```

宏的好处是：它一定发生在编译之前。

宏的坏处也一样：它发生在编译之前。

它不讲类型。

它不讲作用域。

它更不会跟你讨论“这个前提到底是谁负责守”。

#### `enum`：老派但很实用的“编译期整数”

很多老 C++ 代码里，你会见到这种写法。

```cpp
enum { kBufSize = 1024 };

int buf[kBufSize];
```

这看起来像是在用枚举。

其实是在借枚举的一个副作用：枚举值必须是编译期能确定的整数。

所以它能当数组大小。

也能当 `switch` 的 `case`。

但它的缺点也很明显。

它只能放整数。

它也写不出“计算过程”。

#### 模板元编程：把计算藏进类型里

再往后，聪明人开始把“计算”塞进模板。

因为模板参数也要求编译期就能确定。

```cpp
#include <cstddef>

template <std::size_t N>
struct Buffer {
    char data[N];
};

Buffer<1024> b;
```

这类写法能干很多事。

甚至能在编译期算阶乘、算斐波那契。

Boost.MPL、Loki 那一批库，就是在这个路线上把编译器当计算器用。

只是它的代价也很真实。

错误信息像天书。

编译时间像罚站。

你一不小心就开始写“给编译器看的程序”。

#### 编译期断言：早就有人想要，只是语法太丑

在 `static_assert` 还没进标准前，大家也不是没做过“编译期拒绝”。

最常见的是用数组大小来逼编译器报错。

```cpp
#define BUF_SIZE 1024
enum { kOk = 1 / ((BUF_SIZE & (BUF_SIZE - 1)) == 0) };
```

这类技巧很机灵。

但也很不体面。

你写完自己都不想再看第二眼。

对比一下 C11。

C 语言自己也补过这条能力，叫 `_Static_assert`。

```c
_Static_assert(sizeof(int) == 4, "int must be 4 bytes");
```

这也是个很好的参照。

需求是共通的。

只是 C++ 直到 C++11 才把它变成了标准关键字：`static_assert`。

后来 Boost 也提供过 `BOOST_STATIC_ASSERT` 这种东西。

这不是巧合。

是需求一直在那里。

只是标准一直没给一个“像话的语法”。

### C++11 给的两个小东西

C++11 之后，事情开始变得更像“语言”，而不是“约定”。
它给了我们两个很朴素的工具：`constexpr` 和 `static_assert`。

先把词拆开。
`constexpr` 你可以把它当成一句话：这个值如果能在编译时算出来，那就让编译器现在就算。

“编译时”是什么意思。
就是你按下编译那一下，程序还没跑，编译器已经把结果算出来了。

“常量表达式”也没那么玄。
就是一段表达式，编译器现在就能把它的值确定下来。

`static_assert` 更直白。
条件不满足，就别生成可执行文件，让编译直接失败。

### 先把“常量表达式”说清楚

“常量表达式”这词听起来像论文。

其实你可以把它当成编译器的脾气。
有些位置，编译器必须现在就拿到一个确定的值，比如数组大小、`case` 标签、模板参数。

你给它一个“运行时才知道”的值，它就会拒绝。
所以关键不在于你写没写 `const`，关键在于：编译器能不能在编译那一下把它算出来。

### `constexpr` 到底承诺了什么

`constexpr` 更像是你对编译器的一个声明。

我这个东西，目标是让你能在编译期算出来。
能算就算，算不了也没关系，它还可以在运行时照样用。

```cpp
constexpr int add(int a, int b) {
    return a + b;
}

constexpr int k = add(1, 2);

int runtime(int x) {
    return add(x, 2);
}
```

同一段函数。

传进来的是字面量，它就可能在编译期算。

传进来的是运行时变量，它就老老实实运行时算。

你不用写两份代码。

你可以把这当成一条经验。

当你发现自己想写宏。

先停一下。

问一句：我是不是只是想要“编译期就能确定”。

### C++11 的 `constexpr` 其实挺“抠门”

这里有个历史包袱。

C++11 是第一次把“编译期函数”塞进标准。

委员会很谨慎。

所以 C++11 时代的 `constexpr` 函数限制很多。

你通常只能写一个很直接的 `return`。

循环、局部变量、复杂分支这些，在后来的标准里才慢慢放开。

这也是为什么老代码里，你会看到很多递归。

```cpp
constexpr int factorial(int n) {
    return (n <= 1) ? 1 : (n * factorial(n - 1));
}

constexpr int k6 = factorial(6);
```

这段不是为了装逼。

是因为当年很多写法，就只能这么写。

后来标准慢慢放开。

比如 C++14 起，`constexpr` 函数就能写得更像正常函数了。

但 C++11 这篇文章里，你先记住它当年的样子就够用。

### 旁边那条线：别的语言怎么处理“编译期计算”

讲到这儿，你可能会好奇：只有 C++ 这么拧巴吗。

也不是。

很多语言都想过同一件事。

比如 D 有 CTFE（编译期函数执行）。

比如 Rust 也有 `const fn`。

C++11 的选择更保守。

它不想一下子把“编译器里跑程序”这件事开到最大。

它更像是在说：先给一个能落地、能和旧代码共存的版本。

然后再慢慢放权。

### 从坑里爬出来：把约定交给编译器

我们要的其实很简单。

不是更快。

是把那句“必须是 2 的幂”，从注释里搬出来，搬到编译器面前。

```cpp
#include <cstddef>

constexpr bool is_power_of_two(std::size_t x) {
    return x != 0 && (x & (x - 1)) == 0;
}

constexpr std::size_t kBufSize = 1024;
static_assert(is_power_of_two(kBufSize), "kBufSize must be power of two");

constexpr std::size_t kMask = kBufSize - 1;

std::size_t wrap(std::size_t i) {
    return i & kMask;
}
```

这段代码最值钱的地方，是它“很不客气”。

只要有人把 `kBufSize` 改成 1000。

编译器就会直接说不。

你不需要等到线上。

也不需要等到凌晨两点。

#### 再往前一步：把“前提”塞进模板参数

如果你的缓冲区大小本来就应该是“设计期就定死的”。
那你完全可以把它放进模板参数里。

```cpp
#include <cstddef>

template <std::size_t N>
std::size_t wrap(std::size_t i) {
    static_assert(is_power_of_two(N), "N must be power of two");
    return i & (N - 1);
}
```

你可以把它理解成一种写法。

把“配置”从运行时挪回编译期。

这不是所有项目都适用。

但一旦适用，它很省心。

### 再给你几组更具体的场景

你学 `constexpr`，如果只盯着“能不能更快”，很容易学歪。

更常见的用法其实很朴素。

就是：有些地方，语言语法天然只收“编译期能确定的值”。

你不把值提前算出来，你就只能继续写宏。

#### 场景一：数组大小

```cpp
#include <cstddef>

constexpr std::size_t kN = 16;
int a[kN];
```

这里的重点不是 `constexpr` 这个单词。

重点是：数组大小这件事，编译器必须现在就知道。

#### 场景二：`switch` 的 `case`

```cpp
constexpr int kNotFound = 404;

int handle(int code) {
    switch (code) {
    case kNotFound:
        return 0;
    default:
        return 1;
    }
}
```

`case` 标签也要求编译期能确定。

你用宏当然能写。

但 `constexpr` 会更像语言一点。

#### 场景三：模板参数

```cpp
#include <cstddef>

template <std::size_t N>
struct Fixed {
    char data[N];
};

constexpr std::size_t kN = 32;
Fixed<kN> f;
```

这里也一样。

模板参数不是运行时的东西。

你想传进去，就得在编译期把数准备好。

#### 场景四：把前提写成“编译期规则”

`static_assert` 最常见的用法就是这样。

你把一句团队共识，改成一条硬规则。

```cpp
#include <cstddef>

constexpr std::size_t kAlignment = 8;
static_assert((kAlignment & (kAlignment - 1)) == 0, "alignment must be power of two");
```

你甚至不需要真的去“用它算什么”。

你只是让编译器替你把门。

#### 场景五：位掩码这种“老 C 习惯”，也能写得更像语言

```cpp
#include <cstdint>

constexpr std::uint32_t bit(int n) {
    return 1u << n;
}

constexpr std::uint32_t kFlags = bit(0) | bit(3);
static_assert((kFlags & bit(3)) != 0, "flag 3 must be set");
```

这类代码你在 C 里多半会写成宏。

换成 `constexpr` 以后，它至少有类型，也更容易被工具理解。

#### 场景六：`sizeof` 这种“老派常量”，也可以配合 `static_assert`

```cpp
#include <cstdint>

struct Header {
    std::uint32_t magic;
    std::uint16_t version;
    std::uint16_t flags;
};

static_assert(sizeof(Header) == 8, "header size changed");
```

这就是典型的小项目现场。

你写一个协议头。

你希望它别被人悄悄改坏。

那就让编译器帮你盯着。

#### 场景七：把小计算写成函数，而不是写成宏

```cpp
#include <cstddef>

constexpr std::size_t mask(std::size_t n) {
    return n - 1;
}

constexpr std::size_t kMask = mask(1024);
```

这段代码很无聊。

但它的气质跟宏完全不一样。

它有类型。

它有作用域。

它也更容易被 IDE、被重构工具理解。

#### 场景八：很小的查表，也可以先给编译器

```cpp
constexpr int kTable[4] = {1, 2, 4, 8};

constexpr int pick(int i) {
    return kTable[i];
}
```

你别被 “查表” 这俩字吓到。

本质就是：把一组不会变的数据，放到编译期就能确定的位置。

真正的大表怎么生成，是另一个故事。

但你至少知道路在哪儿。

### 一句话记住

能写进编译器的前提，就别写在注释里。

`constexpr` 负责把“能算的”提前算掉。

`static_assert` 负责把“能错的”提前炸掉。

### 留一个亮点

我后来越来越不把 `constexpr` 当成性能招式。

性能当然重要。

但工程里更贵的是误解。

`constexpr` + `static_assert` 的组合，其实是在做一件很“社交”的事。

你把团队里的口头共识，改成了一条机器规则。

从此以后，大家不用靠记忆协作。

靠编译器就行。
