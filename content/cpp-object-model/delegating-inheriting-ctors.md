---
title: "委托构造与继承构造：减少构造函数样板代码"
description: "C++11 给构造函数补了两把小工具：委托构造让你把初始化逻辑收敛到一个入口；继承构造让派生类少写一排转发。它们不花哨，但很省命。"
---

我第一次在代码里看到“构造函数里再调用构造函数”。

是在一个很老的项目里。

那时候还没有 C++11。

同事想统一初始化逻辑。

他写了一个 `init()`。

然后每个构造函数都先把成员随便填一遍。

再在函数体里 `init()`。

看起来挺像回事。

直到有一天。

我们在一个构造函数里忘了调用 `init()`。

忘得很隐蔽。

因为这个构造函数很少用。

而那天刚好用上了。

线上开始出现“偶现”。

这种 bug 的共同点是。

你复现的时候它就乖。

你不盯着它的时候它就闹。

复盘会上那位前辈只说了一句。

> 你想要一个入口。
>
> 你却给了自己五个入口。

后来 C++11 来了。

标准委员会没有替我们把锅背走。

但它给了两个小工具。

一个叫委托构造。

一个叫继承构造。

它们解决的都不是“不会写代码”。

而是“写多了必然漏”。

而漏一次。

就够你喝一壶。

---

### 1. C++11 之前：大家是怎么“统一构造逻辑”的

先说老办法。

老办法不一定错。

只是它们都带着一点味道。

比如 `init()` 这招。

```cpp
class Cache {
public:
    Cache() {
        cap_ = 1024;
        init();
    }

    explicit Cache(std::size_t cap) {
        cap_ = cap;
        init();
    }

private:
    void init() {
        buf_.reset(new int[cap_]);
    }

private:
    std::size_t cap_ = 0;
    std::unique_ptr<int[]> buf_;
};
```

它最大的问题不是丑。

是它把“对象有效”的定义拆成了两段。

构造函数开始那一刻。

对象已经存在。

但它可能还没初始化完。

你必须记住一条潜规则。

“每个构造函数都要记得 `init()`。”

这条规则写在人的脑子里。

不写在类型系统里。

人会忘。

类型系统不会。

还有一种更常见的老办法。

就是把初始化逻辑复制粘贴。

```cpp
class Cache {
public:
    Cache() : cap_(1024), buf_(new int[1024]) {}
    explicit Cache(std::size_t cap) : cap_(cap), buf_(new int[cap]) {}

private:
    std::size_t cap_;
    std::unique_ptr<int[]> buf_;
};
```

这招跑得起来。

但它在等一个时刻。

等你加第三个构造函数。

加第四个。

然后某个构造函数忘了改。

或者改错了一个常量。

事故就很自然地发生。

> 样板代码不是“无聊”。
>
> 它是“复制粘贴的概率”。

C++11 的委托构造。

本质上就是把“统一入口”这件事。

从口号变成语法。

---

### 2. 委托构造：把初始化逻辑收敛到一个入口

委托构造的写法很直。

一个构造函数可以在初始化列表里。

直接调用同一个类里的另一个构造函数。

```cpp
class Cache {
public:
    Cache() : Cache(1024) {}

    explicit Cache(std::size_t cap)
        : cap_(cap), buf_(new int[cap]) {}

private:
    std::size_t cap_;
    std::unique_ptr<int[]> buf_;
};
```

你看它像“语法糖”。

但它改变的是责任边界。

`Cache()` 不再负责“怎么初始化成员”。

它只负责一件事。

“我默认容量是多少。”

真正的初始化。

只发生在 `Cache(std::size_t)` 这个入口。

这就很像工程里的路口合并。

你把五条小路。

并到一条主路上。

评审时你只盯主路。

测试时你也优先打主路。

主路稳了。

其它路就没那么容易翻车。

再给一个更贴近项目的例子。

很多团队会有一个 `Logger`。

你想要三种打开方式。

默认路径。

指定路径。

以及“把文件名拼到某个目录里”。

```cpp
class Logger {
public:
    Logger() : Logger(defaultPath()) {}

    explicit Logger(std::string path)
        : path_(std::move(path)), fp_(std::fopen(path_.c_str(), "a")) {
        if (!fp_) throw std::runtime_error("open log failed");
    }

    Logger(std::string dir, std::string name)
        : Logger(dir + "/" + name) {}

    ~Logger() {
        if (fp_) std::fclose(fp_);
    }

private:
    static std::string defaultPath() {
        return "./app.log";
    }

private:
    std::string path_;
    std::FILE* fp_ = nullptr;
};
```

这里的重点不是 `fopen`。

而是“失败处理”。

你只想在一个地方写。

你只想在一个地方抛异常。

也只想在一个地方保证 `fp_` 的语义一致。

委托构造给你的就是这个。

一个入口。

一份契约。

### 2.1 一个小坑：委托构造时，初始化列表只能写“委托”

很多人第一次写会这么写。

```cpp
class X {
public:
    X() : X(42), n_(1) {}
    explicit X(int v) : n_(v) {}

private:
    int n_;
};
```

这段是编不过的。

因为一旦你写了委托。

初始化列表里就只能写这一个委托。

不能同时再去初始化别的成员。

这条规则并不难理解。

否则你就会遇到一个尴尬问题。

到底谁对 `n_` 负责。

你。

还是被委托的那个构造函数。

标准选择不让你纠结。

它直接禁止。

所以你要么把 `n_` 的默认值写成成员默认初始化。

要么把它交给被委托的那个构造函数。

```cpp
class X {
public:
    X() : X(42) {}
    explicit X(int v) : n_(v) {}

private:
    int n_ = 1;
};
```

### 2.2 另一个小坑：别递归

委托构造也能写出死循环。

```cpp
struct Bad {
    Bad() : Bad(1) {}
    Bad(int) : Bad() {}
};
```

这不是编译器帮你兜底的那种错误。

这是你写了一条闭环。

它会在运行时把栈打穿。

所以我一直觉得。

委托构造像把双刃剑。

省字。

也更集中。

但你必须让“入口”保持单向。

别让它绕回去。

---

### 3. 继承构造：派生类少写一排“转发”

委托构造解决的是“同一个类里好多构造函数”。

继承构造解决的是“我只是包了一层壳”。

这种壳你一定见过。

比如你有个 `Fd` 管文件描述符。

你又想要一个更语义化的类型。

叫 `SocketFd`。

它本质上还是一个 fd。

但你希望类型名字更清楚。

```cpp
class Fd {
public:
    Fd() = default;
    explicit Fd(int fd) : fd_(fd) {}

    int get() const { return fd_; }

protected:
    int fd_ = -1;
};

class SocketFd : public Fd {
public:
    SocketFd() = default;
    explicit SocketFd(int fd) : Fd(fd) {}
};
```

这段代码里。

`SocketFd(int)` 这一行没干任何新事。

它只是把参数转发给基类。

当基类构造函数多起来。

派生类就开始“复制粘贴转发”。

这时候 `using Base::Base;` 就很香。

```cpp
class SocketFd : public Fd {
public:
    using Fd::Fd;
};
```

它的意思是。

把 `Fd` 的构造函数。

作为 `SocketFd` 的构造函数引入进来。

你少写一排样板代码。

也少了一排未来可能漏改的地方。

### 3.1 继承构造不是“继承了初始化”

这里要说一句容易误会的话。

继承构造继承的是“构造函数签名”。

不是“派生类新增成员的初始化”。

所以如果派生类自己加了成员。

你得保证它有默认初始化。

否则你会把自己逼回老路。

```cpp
class TaggedFd : public Fd {
public:
    using Fd::Fd;

private:
    std::string tag_ = "";
};
```

`tag_` 在这里能活得好好的。

是因为它有成员默认初始化。

你别指望基类构造函数帮你管它。

基类只负责基类子对象。

这也是对象模型里很朴素的一件事。

派生类对象里。

确实藏着一块基类子对象。

但派生类新增的那块内存。

你得自己交代。

### 3.2 另一个常见坑：默认参数不会“顺便继承”

有些老代码喜欢这么写。

```cpp
class Base {
public:
    explicit Base(int x = 0) : x_(x) {}

private:
    int x_;
};
```

你以为派生类 `using Base::Base;` 之后。

也能像 `Base()` 一样默认构造。

实际很可能不行。

因为继承构造不会把默认参数也搬过来。

这个细节挺不友好。

但它是标准有意为之。

默认参数是“调用点的语法糖”。

不是函数签名的一部分。

所以你想保留 `Derived()`。

就老实补一层。

```cpp
class Derived : public Base {
public:
    using Base::Base;
    Derived() : Base() {}
};
```

---

### 4. 什么时候用它们

我不喜欢把它们说成“最佳实践”。

我更愿意把它们当成两条经验。

当你发现构造函数里开始出现复制粘贴。

而复制粘贴只是为了“同一套初始化”。

那就该考虑委托构造。

当你发现派生类存在的唯一价值。

只是换了一个名字。

或者加了一点点语义标签。

那就该考虑继承构造。

最后我想留一句老派的提醒。

> 构造函数写得越多。
>
> 你越该怀疑自己是不是在重复表达同一件事。

C++11 没有让我们更聪明。

它只是承认了一件现实。

人会忘。

人会漏。

所以语言给你两把小刀。

让你少写一点。

也少漏一点。
