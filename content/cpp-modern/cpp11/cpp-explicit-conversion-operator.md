---
title: '显式转换运算符：让“能不能转”说清楚'
description: 'C++11 允许把类型转换运算符声明为 explicit，解决老派 operator bool 的尴尬，让“能用在 if 里”不等于“到处都能隐式转换”。'
---

下班后。

茶水间的灯还亮着。

我端着一次性纸杯。

就容易想起当年的那些“能跑就行”。

### 那会儿的 C++，还没那么“讲规矩”

那是很多年前。

我们写的 C++。

其实就是 C。

外加一点 class。

你现在觉得理所当然的很多东西。

当年都没有。

没有 `auto`。

没有 lambda。

更没有什么 `explicit operator bool()`。

那时候大家最爱的一句口头禅是。

“能跑就行。”

然后事故也很配合。

说来就来。

### 一个看起来很正常的愿望

当年我们经常写“句柄”类型。

句柄你可以理解成。

“一个小对象，代表一个外部资源”。

比如一个连接。

比如一个文件。

比如一把锁。

你最想要的体验是。

它能像指针一样。

放进 `if` 里判断一下。

```cpp
if (conn) {
    // 连接是好的
}
```

当年这需求太朴素了。

朴素到大家第一反应就是。

那我就写个 `operator bool()` 呗。

### 线上啪一下：我只是少打了三个字母

故事从一个“小项目”开始。

真的是小项目。

一个简单的聊天室。

每个连接有个 `Conn`。

你只想判断它是不是还活着。

```cpp
struct Conn {
    int id;

    Conn(int id) : id(id) {}
    operator bool() const { return id != 0; }
};
```

看起来没毛病。

然后有一天。

线上报警。

“怎么所有人的消息都串了。”

我打开代码。

看到这行。

```cpp
std::vector<int> slot(100);
slot[conn] = user_id;
```

你可能会说。

这不就写错了嘛。

应该是 `slot[conn.id]`。

问题是。

它居然能编译。

而且单机压测的时候。

看起来也“能跑”。

因为连接一多。

所有人都被塞进了 `slot[1]`。

前一个人的数据。

被后一个人覆盖。

聊天室就变成了。

“大家轮流当一个人”。

### 这锅不是 `vector` 的，是“隐式转换”的

这事的关键在于：`operator bool()` 不止能进 `if`。
它还会在你没注意的时候，跑去参加别的计算。

所谓“隐式转换”，就是编译器觉得“这里需要某种类型”，它就自己帮你把类型换过去。
很多时候你还没反应过来，它已经替你做了选择。

在这里，它干了两步：先把 `Conn` 变成 `bool`，再把 `bool` 变成 `int`。
因为在 C++ 里，`true` 可以当成 `1`，`false` 可以当成 `0`。

```cpp
Conn conn{42};
int x = conn; // x 变成 1
```

这就是当年 `operator bool()` 的尴尬。
你本来只想“能不能放进 if”。
它却顺手给你来了个“到处都能当整数用”。

### 于是江湖出现了：safe-bool 这套暗号

老一辈不是没挣扎过。
当年流行过一个东西，叫 safe-bool idiom。

思路很朴实：你既然老把我当 `int`，那我就不变成 `bool` 了。
我去变成一个更“奇怪”的东西。

比如“成员函数指针”。
你可以把它理解成“指向某个类成员函数的指针”。

它能表达“有没有”。
但不太像数字。

```cpp
struct Conn {
    int id;

    Conn(int id) : id(id) {}

    void ok() const {}
    typedef void (Conn::*safe_bool)() const;

    operator safe_bool() const {
        return id != 0 ? &Conn::ok : 0;
    }
};
```

能用吗。

能。

优雅吗。

说实话。

像在给编译器递小纸条。

### C++11：终于有了正经招式

后来 C++11 来了。
它没劝你少写 `operator bool()`，它直接给你一个按钮。

你可以告诉编译器。
“我只允许你在特定场景里把我当 bool。”

```cpp
struct Conn {
    int id;

    explicit operator bool() const noexcept {
        return id != 0;
    }
};
```

这里的 `explicit`，就是“需要我点头”。
你放进 `if`，可以。

```cpp
Conn c{42};
if (c) {
}
```

你想把它当整数。

不行。

```cpp
Conn c{42};
int x = c; // 编译器会拦住你
```

这类 `if (c)` 这种位置。
叫“需要 bool 的语境”。

别被词吓到。

你就记住。
它就是一些固定的地方：条件判断、逻辑运算。

编译器允许你“临时当一下 bool”。
但不允许你“顺手变成别的”。

### 不止 bool：你也可以把“转换成本”写进类型

有时候你确实能转换成别的类型。
比如一个 `UserId`。
你能把它转成字符串。
但你不希望它在拼接、日志、重载里。
随便被编译器拉平。

```cpp
struct UserId {
    int v;

    explicit operator std::string() const {
        return std::to_string(v);
    }
};
```

你真想要 `std::string`。
就明确写出来。

```cpp
UserId id{42};
auto s = static_cast<std::string>(id);
```

读代码的人会更放心。
因为他知道，这次转换是你自己决定的。
不是编译器“热心帮忙”。

### 一句话收尾

把 `explicit` 用起来。

你其实是在把“线上事故”。

换成“编译器红字”。

这笔账。

很划算。
