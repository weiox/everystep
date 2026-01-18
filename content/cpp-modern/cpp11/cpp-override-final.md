---
title: 'override / final：虚函数覆写别再靠肉眼'
description: 'C++11 的 override 把“我就是要覆写它”写进语法里，final 则把继承链收住，让虚函数不再靠猜。'
---

那会儿还没有这些“把意图写进语法”的小工具。

大家写的还是 C with classes。

写着写着。

项目就长大了。

然后线上啪一下。

你才发现。

最贵的不是崩溃。

是“它居然没按我想的跑”。

### 那些年，覆写靠的是记性

当年写面向对象，虚函数（`virtual`）是个很诱人的东西。

你把一个函数标成 `virtual`，意思是：这个函数允许在“派生类”（也就是子类）里被改写。
你拿着基类引用去调用时，运行时应该跳到子类那份实现。

你可以把它想成。
对象里有个小开关。
它指向“我应该调用哪一份实现”。

你不用知道它内部怎么放。
你只要记住：只有 `virtual` + 引用/指针这套组合，才会真的“晚一点再决定调用谁”。

听起来很美。
但它有个隐藏前提：子类那份函数，必须和基类那份“长得一模一样”。

参数、返回值、`const` 都得对上。
差一丁点，编译器也不急，它会当作：哦，你又写了一个同名的新函数。

这就是重载（overload）。
同名，但参数列表不一样。

### 先把几个词说清楚

很多人第一次在 C++ 里学面向对象，脑子里会同时装着三件事。
类、函数名、还有一堆 `*` 和 `&`。

我先用最小的例子把“虚函数到底在干嘛”说清楚。

```cpp
struct Handler {
    virtual int handle() const { return 0; }
};

struct ApiHandler : Handler {
    int handle() const { return 1; }
};

int run(const Handler& h) {
    return h.handle();
}
```

`Handler` 是基类。
`ApiHandler` 是派生类。

`run()` 收的是 `const Handler&`，但你传进去的可以是 `ApiHandler`。
只要 `handle()` 真的覆写成功，`run(ApiHandler{})` 就会返回 `1`。

这里有个新手很容易忽略的细节。
为什么我偏要写 `const Handler&`，不直接写 `Handler`？

### 为什么一定要用引用或指针

如果你按 C 的习惯传值。
多态就没了。

```cpp
int run(Handler h) {
    return h.handle();
}
```

`run(ApiHandler{})` 这次会返回 `0`。
因为你把一个 `ApiHandler` 传进来时，它会被“切”成一个 `Handler`。

这就叫对象切片（slicing）。
简单说就是：子类那一部分被丢了，只剩下基类那一层。

所以 C++ 里只要你想用虚函数。
几乎都会通过指针或引用来用。

这里有两个关键词。
覆写（override）是“子类把基类那个 `virtual` 函数换成自己的实现”。

重载（overload）是“同名，但参数列表不同，于是变成另一套函数”。
它不要求基类有 `virtual`，也不会改变虚函数调用的落点。

### const 为什么会把人坑死

刚学 C++ 时，很多人把 `const` 当成“可有可无的修饰”。
但放在成员函数上，它是签名的一部分。

```cpp
struct X {
    void f() {}
    void f() const {}
};
```

这不是重复定义。
这是两个不同的函数。

所以你少写一个 `const`。
在覆写里就等于换了一把锁。
钥匙当然插不进去。

### 同名会把基类的重载藏起来

还有一个坑更阴。
你不一定是“没覆写上”。

你可能是把基类的另一个重载给藏起来了。

```cpp
struct Base {
    virtual void f(int) const {}
    void f(double) const {}
};

struct Derived : Base {
    void f(int) const override {}
};
```

这时 `Derived` 里一出现 `f`。
`Base` 里同名的 `f(double)` 会被“藏住”。

你写 `Derived d; d.f(3.14);`。
它可能会去调用 `f(int)`，把 `3.14` 变成 `3`。

如果你想把基类的重载也带进来。
你需要再写一句。

```cpp
struct Derived : Base {
    using Base::f;
    void f(int) const override {}
};
```

`using Base::f;` 这句话的意思很朴素。
把 `Base` 里的那组 `f`，在这里重新“露出来”。

### 旧时代的自救：靠约定

在 C++11 之前，大家也不是完全躺平。
只是手段都偏“人肉”。

比如写代码时复制粘贴基类签名。
比如 code review 里专门盯 `const` 和参数类型。

但项目一大，人一多，重构一来。
这种约定就会漏。

而且它最坏的地方在于。
漏了也不一定当场炸。

它可能编译通过。
测试也可能刚好没覆盖到那条分支。

最后就会变成线上那种。
你看着日志一脸懵。

### 旧时代的自救：靠宏假装有 override

有意思的是。
在 C++11 还没落地之前，很多大项目早就自己搞过一套。

他们会写 `OVERRIDE`、`FINAL` 这种宏。
让代码先长得像“可检查的覆写”，再慢慢等编译器跟上。

Chromium 这类大项目就用过 `OVERRIDE` / `FINAL`。
Qt 这边也有 `Q_DECL_OVERRIDE` / `Q_DECL_FINAL` 这种写法。

你不需要记住这些宏。
你只要感受一下当年的工程气味。
大家已经被这个坑烦透了。

比如你能在一些老代码里看到这种风格（意思示意）。

```cpp
#define OVERRIDE /* nothing */

struct Base {
    virtual int f() const { return 0; }
};

struct Derived : Base {
    int f() const OVERRIDE { return 1; }
};
```

在支持的编译器上，`OVERRIDE` 会被展开成某种扩展关键字。
在不支持的编译器上，它就先当空气。

这种做法很“土”。
但很真实。

因为工程团队等不起。
他们需要的是现在就能少出事故。

### 经典坑 1：少了 const

假设你写了个小 HTTP 服务。
你想让不同的处理器去“处理请求”。

```cpp
struct Handler {
    virtual int handle() const { return 0; }
};

struct ApiHandler : Handler {
    int handle() { return 1; } // 少了 const
};

int run(const Handler& h) {
    return h.handle();
}
```

这段代码能编译。
也很像“能跑”。

但 `run(ApiHandler{})` 返回的是 `0`。
因为 `ApiHandler::handle()` 少了 `const`，它没覆写到基类的那个虚函数。

它只是多出来的一个新函数。
你以为你改了线上行为，实际上你只是给自己留了个坑。

### 经典坑 2：参数类型悄悄变了

这种也很常见。
你改了一个类型，觉得“差不多”，但签名已经不是同一个函数了。

```cpp
struct Handler {
    virtual int handle(long code) const { return 0; }
};

struct ApiHandler : Handler {
    int handle(int code) const { return 1; } // int 不是 long
};

int run(const Handler& h) {
    return h.handle(200L);
}
```

你心里想的是：`int` 和 `long` 不就都是整数么。
但覆写要求的是“签名完全一致”，不是“看起来差不多”。

所以这次还是没覆写。
`run(ApiHandler{})` 依旧会走到基类那份实现。

### 经典坑 3：引用和值

刚从 C 过来的人，很容易把“传值”和“传引用”当成性能问题。
但在覆写这里，它首先是“接口契约”。

```cpp
struct Handler {
    virtual int handle(int& code) const { return 0; }
};

struct ApiHandler : Handler {
    int handle(int code) const { return 1; } // & 不见了
};
```

同样的故事。
能编译。
也能让你误以为自己改到了虚函数。

### 经典坑 4：默认参数不跟着“虚”

这个坑特别像段子。
因为你会觉得自己被语言背刺了。

```cpp
struct Base {
    virtual int f(int x = 1) const { return x; }
};

struct Derived : Base {
    int f(int x = 2) const override { return x; }
};

int run(const Base& b) {
    return b.f();
}
```

这里覆写是成功的。
但 `run(Derived{})` 会返回 `1`。

原因很简单。
默认参数是“编译时决定的”，它跟着静态类型走。

这里的静态类型，说白了就是。
你在代码里写出来的那个类型。

`run()` 里看见的是 `Base`。
所以它用的是 `Base` 的默认值。

但函数体走的还是 `Derived::f`。
只是参数被塞进来的是 `1`。

这也是为什么老项目里常见一条规矩。
虚函数别写默认参数。
要默认值就自己在函数体里处理。

### override：把“我就是要覆写它”写死

C++11 做了一件很朴素、很工程的事：给你一个关键字 `override`。

你写上它，就是把话说重一点。
“这不是重载，我就是要覆写。”

```cpp
struct Handler {
    virtual int handle() const { return 0; }
};

struct ApiHandler : Handler {
    int handle() override { return 1; } // 这里会直接编译失败
};
```

这次编译器终于不沉默了。
它会告诉你：覆写失败。

你会被迫把签名改对。

```cpp
struct ApiHandler : Handler {
    int handle() const override { return 1; }
};
```

你看。
没有什么玄学。

就是把“我想干什么”写进语法里。
然后让编译器当坏人。

### 你甚至可以给析构函数写 override

这个很多新手不知道。
但它在真实项目里挺常见。

```cpp
struct Base {
    virtual ~Base() {}
};

struct Derived : Base {
    ~Derived() override {}
};
```

它的意思不是“析构更虚”。
而是一样的诉求：我就是要覆写基类那个虚析构。

如果基类析构不是 `virtual`。
这里也会被编译器拦下。

### override 不是 C++ 独创：隔壁早这么干了

这类坑并不是 C++ 专属。
很多语言都踩过，然后都走向同一个方向：让编译器替你验签。

先说 Java。
它很早就有 `@Override` 这个注解。

```java
class Base { int f() { return 0; } }

class Derived extends Base {
    @Override int f() { return 1; }
}
```

`@Override` 的作用不是“让它能覆写”。
Java 本来就能覆写。

它的作用是：如果你写错了签名，编译器立刻报错。
让你别把 bug 留到运行时。

再看 C#。
它干脆把覆写写成关键字。

```csharp
class Base { public virtual int F() => 0; }

class Derived : Base { public override int F() => 1; }
```

你不写 `override`，就没法覆写。
意思很明确：别猜，写清楚。

所以你可以把 C++11 的 `override` 理解成。
向这些语言借的“工程保险”。

### C++11 为什么还让你“可选”

你可能会问。
既然 C# 不写 `override` 就不让覆写。
那 C++ 为啥不学到底？

一个很现实的原因是。
C++ 要背很多历史包袱。
老代码海量存在。

它没法一刀切地要求“所有覆写都必须标注”。
那样升级成本太高。

所以 C++11 选了一个折中。
不强制你写。

但你一旦写了。
编译器就替你“验签”。

这也是为什么很多团队会把它变成自己的规矩。
只要覆写就必须写 `override`。
让规矩落到语法上。

### 再给一个更阴的坑：基类忘了写 virtual

这个坑更阴。
因为错的人可能不是你。

```cpp
struct Handler {
    int handle() const { return 0; } // 忘了写 virtual
};

struct ApiHandler : Handler {
    int handle() const override { return 1; } // 这里会编译失败
};
```

如果没有 `override`。
你会写出一个“看起来像覆写”的函数，然后上线等死。

有了 `override`。
编译器会告诉你：你覆写不了，因为基类根本不是虚函数。

### 关键结论

写覆写。

就写 `override`。

### final：把继承链收住

另一个当年常见的尴尬是：你写了一个类，你以为它会一直这么用。

结果过两年，有人在它下面又继承了一层。
然后又在那层里覆写了关键虚函数，最后系统行为变得像一锅粥。

`final` 就是用来收口的。

你可以把整个类封死。

```cpp
struct Handler {
    virtual int handle() const { return 0; }
};

struct DefaultHandler final : Handler {
    int handle() const override { return 0; }
};
```

`DefaultHandler` 到这里就结束。
谁再想继承它，编译器会当场拦下。

你也可以只封一个虚函数。

```cpp
struct Handler {
    virtual int handle() const { return 0; }
};

struct StableHandler : Handler {
    int handle() const final { return 0; }
};
```

意思是：你可以继承我。
但这个入口别动。

### final 也能用在成员函数上，专门收口扩展点

很多框架代码会留几个“钩子”。
让你改一部分。

但有些关键路径不想让你碰。

```cpp
struct Base {
    virtual int hook() const { return 0; }
    virtual int run() const { return hook(); }
};

struct App : Base {
    int hook() const override { return 1; }
    int run() const final { return 42; }
};
```

你可以继续继承 `App`。
但你别指望再把 `run()` 的行为改掉。

### final 也不是 C++ 独创：Java 的 final，C# 的 sealed

如果你写过一点 Java，你可能见过 `final class`。
意思差不多：这个类到我为止。

```java
final class Closed {}
```

C# 这边叫 `sealed`。
也是同一个意思。

```csharp
sealed class Closed : Base {}
```

而且在 C# 里。
你甚至可以封住“某个 override”。

```csharp
class Base { public virtual int F() => 0; }

class Mid : Base { public sealed override int F() => 1; }
```

这句 `sealed override`。
就很像 C++ 里的 `final` 成员函数。

C++11 选择了 `final` 这个词。
语义也很贴：封住，到此为止。

### 横向对比小结

Java 和 C# 早就把“覆写意图”当成可检查的东西。

Java 用注解。
C# 用关键字，甚至把它做成强制。

C++11 走了折中路线。
不强制你写，但提供 `override` / `final` 让你把关键意图钉死在语法里。

### 一个现实理由：final 有时也能让代码更快

这不是让你去追性能。
只是顺便告诉你：语言设计者也在乎这点。

当一个虚函数被标成 `final`，编译器更有机会确定“不会再被覆写”。
它有时就能把一次虚函数调用，变成一次更直接的调用。

### 一个顺手的洞见

Bjarne Stroustrup 在书里反复强调过一个意思。
让错误越早发生越好。

`override` 和 `final` 就是这种工具。
它们不是为了“写得更像 C++”，而是把“靠肉眼检查的约定”，变成“编译器能执行的规则”。

### 最后再强调一句

最吓人的 bug。

不是程序报错。

是它安安静静地跑错。
