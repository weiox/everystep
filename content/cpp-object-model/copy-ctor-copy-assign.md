---
title: '拷贝构造与拷贝赋值：编译器默认做了什么？'
description: '默认拷贝不是魔法。它只是很老实地“逐成员复制”。问题在于：你以为你在拷贝值，编译器可能只是在拷贝地址。'
---

八十年代中后期。

很多人写 C。

系统能跑。

但也很脆。

尤其是“收尾”。

内存要自己要。

也要自己还。

有时候你只是想复制一份数据。

结果你复制走的，是一颗定时炸弹。

后来 C++ 来了。

它做了很多大事。    

但我觉得有一件“小事”特别关键。

它把“复制”这件事。

从一个手法。

变成了一种语言承诺。

而事故。

就藏在你以为很普通的一行里。

```cpp
b = a;
```


### 当年没有“对象语义”时，复制是什么样子

在 C 里，复制很直接。
你要么赋值一个 struct。
要么 `memcpy` 一块内存。

```c
struct Pair {
    int x;
    int y;
};

int main() {
    struct Pair a = {1, 2};
    struct Pair b = a;
}
```

这很好。
因为 `x`、`y` 都是值。

问题出在另一类 struct 上。
里面放的是“地址”。


### 当年大家踩过的坑：你以为你在复制数据，其实你在复制地址

```c
struct Buf {
    char* p;
};

int main() {
    struct Buf a;
    struct Buf b = a;
}
```

这一行会把 `p` 里的地址抄过去。
它不会替你复制 `p` 指向的那块内容。

在 C 里，这事通常还能“靠纪律”兜住。
谁申请，谁释放。
别乱复制。

但纪律这东西。
一忙就断。


### C++ 当年想解决什么：让“收尾”变成类型的责任

在“C with classes”的年代。
大家开始用构造函数做初始化。
用析构函数做清理。

你可以把它理解成：
对象不仅要“有内存”。
还要“能自己收尾”。

这一步很重要。
因为它把很多事故，从凌晨报警变成了作用域结束。


### 但新坑也跟着出现：你写了析构，就等于声明了“所有权”

下面这段代码。
在当年非常常见。

```cpp
#include <cstring>

struct Buf {
    char* p;

    Buf(const char* s) {
        std::size_t n = std::strlen(s);
        p = new char[n + 1];
        std::memcpy(p, s, n + 1);
    }

    ~Buf() {
        delete[] p;
    }
};

int main() {
    Buf a("hi");
    Buf b = a;
}
```

`Buf b = a;` 这一行看起来只是“复制”。
但它会让 `a.p` 和 `b.p` 指向同一块堆内存。
作用域结束时，析构跑两次。

这就是那句老话的出处。

> 你写了析构。
> 你就得想清楚拷贝。


### 关键结论（A）

默认拷贝只会“抄成员”。

它不懂“所有权”。


### 编译器默认做了什么：逐成员复制（很老实）

如果你没写拷贝相关的函数。
编译器会在需要时尝试生成。
生成策略非常朴素：逐成员。

你可以先把它当成这个意思。

```cpp
struct T {
    int x;
    int y;
};

// 伪代码：拷贝构造
T::T(const T& other) : x(other.x), y(other.y) {}

// 伪代码：拷贝赋值
T& T::operator=(const T& other) {
    x = other.x;
    y = other.y;
    return *this;
}
```

这对纯值成员来说很好。
但对指针来说，它只是在复制地址。


### 把“复制”拆成两件事：出生时复制 vs 活着时覆盖

很多人第一次学 C++。
会被同一个 `=` 绕晕。
我当年也是。

```cpp
struct X { int v; };

int main() {
    X a{1};
    X b = a;  // 出生：用 a 初始化 b
    b = a;    // 覆盖：把 a 的内容赋给 b
}
```

第一句叫拷贝构造。
因为 `b` 还没“出生”，它在走构造流程。

第二句叫拷贝赋值。
因为 `b` 已经活着了，这次是覆盖旧状态。


### 这两个函数的“标准长相”

不用背一堆规则。
先把名字和形状对上。

```cpp
struct T {
    T(const T& other);
    T& operator=(const T& other);
};
```

`const T&` 的意思是：从一个不该被你修改的对象里复制。
返回 `T&` 的意思是：允许你写 `c = b = a;`。


### 有时候编译器会说“不行”：默认拷贝生成不出来

最常见的一类原因。
是赋值阶段没法改。

```cpp
struct X {
    const int id;
};

int main() {
    X a{1};
    X b{2};
    // b = a; // 这里会失败：id 不能被重新赋值
}
```

拷贝构造还能做。
因为构造时可以初始化 `id`。
但拷贝赋值做不到。


### 从坑里爬出来：你大概有三种态度

第一种。
让类型变成值语义。
用会自己管理资源的成员。

```cpp
#include <string>

struct User {
    int id;
    std::string name;
};

int main() {
    User a{1, "alice"};
    User b = a;
}
```

这时候默认拷贝就很像你在 C 里复制一份“值”。
因为 `std::string` 会把自己的资源规则处理好。

第二种。
你确实要手动管资源。
那就把拷贝规则写出来。

```cpp
#include <cstring>

struct Buf {
    char* p;

    Buf(const char* s) {
        std::size_t n = std::strlen(s);
        p = new char[n + 1];
        std::memcpy(p, s, n + 1);
    }

    Buf(const Buf& other) {
        std::size_t n = std::strlen(other.p);
        p = new char[n + 1];
        std::memcpy(p, other.p, n + 1);
    }

    ~Buf() { delete[] p; }
};
```

这段只展示一个核心点。
拷贝构造里，你得给新对象分配新资源。

第三种。
你就是不想让它被拷贝。
那就明确禁止。

```cpp
struct NonCopyable {
    NonCopyable() = default;
    NonCopyable(const NonCopyable&) = delete;
    NonCopyable& operator=(const NonCopyable&) = delete;
};
```

这样最好。
事故会在编译期就停下来。


### 最后再落一句（A）

你写下 `b = a;`。

你其实是在跟类型签合同。

合同没写清楚。

编译器就只能按“逐成员抄一遍”来办。
