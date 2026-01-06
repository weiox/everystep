---
title: "using 声明：改变继承成员的访问权限"
description: "从早期 C++ 的‘复用焦虑’讲起：为什么大家需要把基类的某个成员单独挑出来公开，以及 using 声明当年是怎么把人从转发样板代码里救出来的。"
---

### 先说个老故事

有些语法。

长得像小修小补。

当年却真救过人。


你写着写着。

会突然发现。

你想复用一个基类。

但你不想把它的所有东西都“送出去”。


更麻烦的是。

你还不想为了这点事。

去改基类。


### 回到 C with Classes 的年代

80 年代的贝尔实验室。

C++ 还经常被叫作 C with Classes。

那时候的工程正在变大。

大到“复制粘贴 + 靠自觉”已经有点顶不住了。


Bjarne 的思路一直很克制。

能复用 C 的工具链就复用。

能把成本留在“你用到的地方”就别外溢。


于是 C++ 一边给你继承。

一边给你访问控制：`public` / `protected` / `private`。

这俩东西合在一起。

马上就会逼出一个很具体的问题。


### 没有 using 声明之前：一层转发，写到手酸

你想复用基类实现。

但你只想暴露其中一两个成员。


最直觉的办法。

就是再写一层“转发函数”。


代码也不难。

只是很烦。


### 一段代码，把问题复现出来

先来一个基类。

它像一个“底层把手”。

能干很多事。


```cpp
class Fd {
public:
    void close();
    int native() const;
};
```

然后你写一个派生类。

你决定用 `private` 继承。

意思是：我只是想复用实现。

我不打算把它当成“is-a”。

`is-a` 你可以粗暴理解成。

“它就是一种 `Fd`”。

比如 `Socket` 在任何需要 `Fd` 的地方都能直接顶上去。

但这里我们不想这么承诺。


```cpp
class Socket : private Fd {
public:
    void send(const char*);
};

int main() {
    Socket s;
    s.close();
}
```

这句 `s.close()`。

编译器会拒绝你。

因为 `private` 继承会把 `Fd` 的 `public` 成员。

在 `Socket` 里变成 `private`。


于是老代码常见的补丁是这样。


```cpp
class Socket : private Fd {
public:
    void close() { Fd::close(); }
};
```

能用。

但你已经开始复制粘贴接口了。

接口一多。

你就会开始祈祷自己别漏改。


### using 声明：把“这个名字”搬到派生类

后来 C++ 给了一个很朴素的工具。

`using Base::member;`。


它做的事很像搬家。

把基类里的某个成员名字。

搬到派生类的作用域里。


关键点在于。

你把它写在哪个访问区（`public`/`protected`/`private`）。

它在派生类里就按那个访问级别对外开放。


还是刚才那个例子。

我们只把 `close` 挑出来。


```cpp
class Socket : private Fd {
public:
    using Fd::close;
};

int main() {
    Socket s;
    s.close();
}
```

现在这句 `s.close()` 就顺了。

你不用再写那层转发函数。

也不用改 `Fd`。


### 它到底“改变”了什么

这件事很容易被误会成。

“我把基类的访问权限改了”。


不是。

基类还是基类。

`Fd::close` 依然是 `Fd` 的 `public`。


using 声明只是说。

在 `Socket` 这个类型的对外接口里。

我愿意开一扇门。

让外界可以通过 `Socket` 这条路去调用它。


### 还有一个 using：继承构造函数

有些人第一次看到 `using Base::Base;`。

会以为是另一套东西。

其实它还是 using 声明。

只是“搬”的不是普通成员函数。

而是构造函数。


在 C++11 之前。

你想让派生类复用基类的构造函数。

一般只能这么转发。


```cpp
class Base {
public:
    explicit Base(int);
};

class Derived : public Base {
public:
    explicit Derived(int x) : Base(x) {}
};
```

能用。

但就是样板。

而且构造函数一多。

你就开始复制粘贴。


后来标准给了你一条省事路。


```cpp
class Derived : public Base {
public:
    using Base::Base;
};
```

意思是。

把 `Base` 的构造函数。

作为 `Derived` 的构造函数引进来。


但它也有边界。

它只管基类那一段。

你自己新加的成员。

还是得自己交代。


### 再来一个更常见的场景：把 protected 翻出来

有些基类会把某个成员设成 `protected`。

意思是：只给子类用。

不给外部用。


但你写着写着。

会希望某个派生类把它公开。

因为这个派生类更“具体”。


```cpp
class Base {
protected:
    void reset();
};

class Derived : public Base {
public:
    using Base::reset;
};

int main() {
    Derived d;
    d.reset();
}
```

`Base::reset` 还是 `protected`。

只是 `Derived` 选择把它当成自己的 `public` 接口。


### 一个顺手的坑：名字隐藏

老 C++ 里有个挺阴的坑。

你在派生类里声明了一个同名函数。

基类的同名重载会被“整组隐藏”。


```cpp
struct Base {
    void f(int);
    void f(double);
};

struct D : Base {
    void f(int);
};

int main() {
    D d;
    d.f(3.14);
}
```

这句 `d.f(3.14)` 往往会编不过。

因为 `Base::f(double)` 被隐藏了。


这时候也可以用 `using` 把整组名字拉回来。


```cpp
struct D : Base {
    using Base::f;
    void f(int);
};
```

这算是个小彩蛋。

很多人第一次踩到它。

都会怀疑自己是不是记错了“重载”的定义。


### 一句话结论

using 声明。

就是让你在派生类里。

精确地“挑选要公开的基类成员”。


### 小结

你当然可以靠转发函数解决。

也当然可以回去改基类。


using 声明的价值在于。

它让这件事变得更像“接口设计”。

而不是“补样板代码”。


写久了你会发现。

很多 C++ 的语法糖。

本质上都在帮你把团队约定。

变成编译器能检查的合同。
