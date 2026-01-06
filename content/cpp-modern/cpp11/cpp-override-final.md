---
title: 'override / final：虚函数覆写别再靠肉眼'
description: 'C++11 的 override 把“我就是要覆写它”写进语法里，final 则把继承链收住，让虚函数不再靠猜。'
---

我见过一种最冤的线上 bug。

功能没改。

逻辑也没改。

只是把一个类名重构了一下。

然后某个虚函数突然“失效”。

代码编译过。

测试也没覆盖到。

上线之后。

行为变了。

最后排查半天。

发现是覆写没覆上。

签名差了一点点。

比如少了个 `const`。

或者参数类型从 `int` 变成了 `long`。

这种错。

C++03 时代太常见了。

因为你写。

编译器也不提醒你。

它只会很老实地当作。

“哦，你写了一个新的函数。”

然后虚表里还是老的那个。

你以为你改了行为。

实际上你只是加了一个重载。

C++11 的 `override`。

就是为这种冤案来的。

## override：把意图写到语法里

```cpp
struct Base {
    virtual void f(int) const {}
};

struct Derived : Base {
    void f(int) const override {}
};
```

你写上 `override`。

等于对编译器说。

“我就是要覆写 Base::f”。

如果你覆写不上。

编译器就直接拦住。

```cpp
struct Bad : Base {
    void f(int) override {} // 少了 const
};
```

这次不再是“行为悄悄变了”。

而是“你根本编不过”。

这对工程太友好了。

因为你宁愿在白天编译时被骂。

也别在晚上线上被打脸。

## final：把继承链收住

有些类。

你就不希望它再被继承。

或者你不希望某个虚函数再被覆写。

C++11 给了 `final`。

```cpp
struct Base {
    virtual void g() {}
};

struct Closed final : Base {
    void g() override {}
};
```

`Closed` 这个类。

到这里就封口。

再继承会编译失败。

你也可以只封某个函数。

```cpp
struct Base {
    virtual void h() {}
};

struct Mid : Base {
    void h() final {}
};

struct Derived : Mid {
    void h() override {} // 不允许
};
```

这在框架代码里很常见。

你想留扩展点。

也想把不该动的地方封死。

final 让这事变成了“语法级约束”。

不是靠文档。

也不是靠口头约定。

## 小洞见

很多人把 override 当成“可选”。

我不这么看。

我更愿意把它当成一种保险。

你写虚函数覆写。

本质上是在赌一件事。

“签名完全一致”。

这个赌注太容易输。

因为人会手滑。

会重构。

会改 typedef。

会加 const。

会改引用。

但编译器不会。

所以我习惯是。

只要是覆写。

就写 `override`。

它不是为了炫技。

它是为了让未来少一点侦探工作。

让 bug 更早露头。
