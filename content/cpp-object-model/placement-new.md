---
title: 'placement new：在指定内存上构造对象'
description: '它不分配内存。它只是把构造函数“按”在你给的地址上。'
---

八十年代初。

贝尔实验室里有一拨人。

他们在用 C 写系统。

代码能跑。

但维护很痛。

痛点不新鲜。

就是“忘了收尾”。

忘了 `destroy`。

忘了 `free`。

忘了某个 `init`。

然后线上开始讲鬼故事。

有人想把这事变得更可靠。

不是靠喊口号。

是靠语言把路堵住。

有个人后来经常被提到。
叫 Bjarne Stroustrup。

他当时做的事不神秘。
就是在 C 上加一点“规矩”。


### 当年的背景：从“手写 init/destroy”走出来

那时候的 C 风格很直。
分配一块内存，然后你自己把它“弄成可用的样子”。

比如这样。

```cpp
#include <cstdlib>

struct File {
    int fd;
};

int main() {
    File* f = static_cast<File*>(std::malloc(sizeof(File)));
    std::free(f);
}
```

现实里的代码往往还会有 `init(f)` / `destroy(f)`。
这段代码能写，也能忘。

忘一次，代价可能不大。
忘十次，你就开始写事故复盘。


### 发明者在想什么：对象得“出生就完整”

于是 C++ 的想法就很朴素。
把“初始化”和“清理”变成类型的职责。

这就是构造函数和析构函数。
你可以把它们当成“开机”和“关机”。

所以 `new T(...)` 不是 `malloc` 的别名。
它做的是一套连招：先拿内存，再调用构造函数。


### 你现在最可能困惑的点：内存和对象不是一回事

你在 C 里习惯了“有地址就能用”。
但在 C++ 里，类型更在乎的是：构造函数跑没跑。

没跑。
这块内存就只是字节。

跑了。
对象才算真正“活过来”。


### 但新问题也来了：我想先拿内存，晚点再开机

写容器的人先遇到这个问题。
写内存池的人也会遇到。

他们的需求很具体。
先搞到一块连续的原始内存，然后按需在里面构造对象。

这不叫“抠门”。
这是在做性能和控制。


### 先复现一个坑：把字节当对象用

如果你刚从 C 过来。
很可能会写出这样的代码。

```cpp
#include <cstdlib>

struct Widget {
    Widget(int v) : x(v) {}
    int x;
};

int main() {
    void* mem = std::malloc(sizeof(Widget));
    Widget* p = static_cast<Widget*>(mem);
    return p->x;
}
```

它能编过。
但它没有给你“对象已经出生”的保证。

更直白一点。
`static_cast` 只是“换个类型看这块内存”。
它不会替你调用构造函数。

所以这里的 `p->x`。
不是在读一个 `Widget` 的成员。
更像是在读一块随机字节。

你可能今天读到 0。
明天读到 123456。
也可能一切都正常到让你放松警惕。

这种“不知道会发生什么”的状态。
在 C++ 里有个名字。
叫未定义行为。


### 从坑里爬出来：给“构造”单独一个按钮

后来就出现了一个很小、但很关键的能力。
在你指定的地址上，调用构造函数。

这个能力就叫 placement new。
`placement` 的意思很直白：放置。


### 先记一句话（A）

placement new 只负责构造。


### 它长什么样：在指定地址上“按下开机键”

写法是这样。

```cpp
#include <new>

T* p = new (mem) T(args);
```

括号里的 `mem` 是你给的地址。
这句不会去申请堆内存，它只会调用构造函数。


### 这块内存从哪里来：你有三条路

很多初学者卡在这个问题上。
`mem` 到底是谁给的？

答案是：谁都可以。
但要满足三个条件。

够大。
够对齐。
还活着。

第三个最容易被忽略。
你把内存 `free` 了。
它就不再是你的了。

第一条路。
栈上的缓冲区。

```cpp
#include <new>

struct Widget {
    explicit Widget(int v) : x(v) {}
    int x;
};

int main() {
    alignas(Widget) unsigned char buf[sizeof(Widget)];
    Widget* p = new (buf) Widget(7);
    p->~Widget();
}
```

`buf` 就是那块内存。
`alignas(Widget)` 是在提醒编译器：这块内存要“站得稳”。

还有一点很容易忘。
`buf` 在栈上。
离开函数，它就没了。
所以别把 `p` 作为返回值带出去。

第二条路。
你熟悉的 `malloc`。

```cpp
#include <cstdlib>
#include <new>

struct Widget {
    explicit Widget(int v) : x(v) {}
    int x;
};

int main() {
    void* mem = std::malloc(sizeof(Widget));
    Widget* p = new (mem) Widget(7);
    p->~Widget();
    std::free(mem);
}
```

这条路能走。
但你要自己负责异常路径和释放顺序。

还有个小细节。
`malloc` 的对齐在大多数场景够用。
但如果你的类型对齐要求更高。
你就得更小心。

第三条路。
用 `::operator new`。

```cpp
#include <new>

struct Widget {
    explicit Widget(int v) : x(v) {}
    int x;
};

int main() {
    void* mem = ::operator new(sizeof(Widget));
    Widget* p = new (mem) Widget(7);
    p->~Widget();
    ::operator delete(mem);
}
```

它看起来像 `malloc`。
但它是 C++ 的那套分配机制。

顺手提醒一句。
谁分配的，就用谁的方式释放。

很多人第一次看到这里。
会开始混乱：`new`、`::operator new`、`delete`、`::operator delete` 到底谁是谁。

你可以先记一个不严谨但很管用的对应关系。
`new T(...)` 这句，通常等价于“分配 + 构造”。

```cpp
#include <new>

// 伪代码：帮助你建立直觉
void* mem = ::operator new(sizeof(T));
T* p = new (mem) T(args);
```

也就是说。
你平时写的 `new`，背后大概率也用到了 placement new。

反过来。
`delete p` 通常等价于“析构 + 释放”。

```cpp
// 伪代码：帮助你建立直觉
p->~T();
::operator delete(p);
```

所以 placement new 的重点就很清楚了。
它把“分配那一步”拿掉了。
只留下“构造”。

### 把刚才那段代码改对：一行就够

```cpp
#include <cstdlib>
#include <new>

struct Widget {
    Widget(int v) : x(v) {}
    int x;
};

int main() {
    void* mem = std::malloc(sizeof(Widget));
    Widget* p = new (mem) Widget(7);
    int r = p->x;
    p->~Widget();
    std::free(mem);
    return r;
}
```

`new (mem) Widget(7)` 做的事很单纯。
它让对象在这块内存上“出生”。


### 关键结论（A）

有内存。

不等于有对象。


### 另一个坑：它不帮你收尾

这里很容易误会。
你用 placement new 构造出来的对象，不能随手 `delete p`。

因为 `delete` 默认会做两件事。
先析构，再释放那块“由 new 分配”的内存。

而你的内存不是它分配的。
所以你要自己把“关机”和“归还内存”写清楚。

```cpp
p->~Widget();
```

这行叫显式析构。
它只调用析构函数，不会 `free`。

还有一个读起来很别扭、但很关键的点。
析构完之后，那块内存又变回“字节”。

你如果还想用它。
你得在同一块内存上，再构造一次。


### 一个小洞见（给读过容器的人）

当你看到 `reserve` 的时候。
它常常只做“拿地”，不做“盖房”。

真正的对象。
要等到你 push 进去的时候才一个个出生。


### 最后再落一句（A）

分配是拿地。

构造是点火。

你把这两件事拆开。

就会更少踩坑。
