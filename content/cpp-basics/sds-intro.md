---
title: "揭秘 Redis 字符串：C++ 程序员的面试加分项 SDS"
description: "聊腻了 std::string 和 char*？不如看一眼 Redis 这种工业级系统是怎么重新发明字符串的：SDS 如何在 C 的基础上补完长度、安全和性能这几笔历史欠账。"
order: 90
---
 
在我做面试官的这些年里，只要聊到 C++，字符串几乎是逃不过去的一关。  
一开始大家还在认真比较 `std::string` 和 `char*`，讨论谁更安全、谁更高效，聊到差不多的时候，我一般会顺手把话题往旁边一拐：

“如果不盯着教科书看，换个角度想想——那些真正在线上扛着流量跑的系统，是怎么对待字符串的？”

这时候，我就会把 Redis 拉出来当主角。这个天天和字符串打交道的家伙，很早就嫌 C 那套 `char*` 太别扭，于是干脆在 C 之上又造了一层自己的字符串抽象：SDS（Simple Dynamic String）。

这一篇，我们就当是把这个故事好好讲一遍：  
先回到 C 语言还年轻的年代，看看当时为什么会选现在这套字符串方案；  
再顺着时间往前走，到 Redis 出场的那个阶段，看看 SDS 是怎么一步步把这些“历史欠账”补回来。

### C 语言字符串：从时代选择到“美丽的错误”

要理解 SDS 为什么会长成现在这个样子，得先对它的“前辈”——C 字符串——多一点耐心。

把时间往回拨到七八十年代：内存贵得要命，CPU 频率抬不太起来，编译器也远没有今天这么鬼灵精。那时候设计 C 的人心里打的算盘很简单：

“我得有一种最省事的办法，在一块连续内存里塞一串字符，
  机器好实现，移植到各种平台也不折腾。”

最后落地的方案，就是你现在熟得不能再熟的那一幕：

一条连续的字符数组，加上末尾一个 `\0` 作为“终点标记”。

- 对当时的编译器来说，这太友好了：只要知道起始地址，顺着走到 `\0` 为止。
- 对那会儿的程序规模来说，也够用：字符串不长，调用频率也有限。

问题在于，语言活得比当年的机器久太多。几十年下来，C 被用在了操作系统、数据库、网络服务里，字符串也从“顺手一用的小工具”，变成“系统的血液”。这时候，当年的那点简化，就开始反过来“要债”了。

可以先记住几件麻烦事：

1. **问长度，要从头走到尾**  
   `strlen()` 想知道多长，只能从第一个字符开始，一直数到 `\0`。  
   在配置文件里扫一遍没什么；但如果在高频路径上对长字符串反复 `strlen`，这就是实打实的 O(N) 成本。

2. **缓冲区溢出，几乎写在 C 的 DNA 里**  
   像 `strcpy`、`strcat` 这种函数，并不知道目标缓冲区有多大。  
   你多给一点空间，它就乖乖地拷贝；你少给一点，它一样照抄不误，顺带把旁边的内存一起改了。早年的许多安全漏洞，追根究底都能扯回到这里。

3. **对通用二进制很不友好**  
   把 `\0` 当“结束信号”的后果是：一旦数据中间自然地包含了 `\0`，  
   剩下的内容在 C 字符串的视角里就统统“消失”了。  
   这让它更适合表示人类文本，而不是“任意字节序列”。

4. **一有修改，就容易牵动一大片**  
   在复杂系统里，字符串经常需要拼接、截断、替换。  
   如果底层只是“紧挨着的字符 + 终止符”，那一旦长度变化，很容易就演变成：重新分配一块新内存，把旧数据全量拷贝过去，再释放旧的。

在小程序、小脚本时代，这些都还不至于变成灾难。  
但对 Redis 这样的服务来说，**字符串既是主要的数据结构，又处在最热的路径上**，这些“历史选择”就必须被一一对账。

### SDS 闪亮登场：从 “char*” 到“带脑子的字符串”

在这样的背景下，Redis 作者 antirez 设计了 SDS——Simple Dynamic String。

它没有离开 C 的世界：底层依然是结构体加上一块连续内存，只是多加了一点“脑子”：

- **Simple（简单）**：没有黑魔法，结构体 + `malloc` / `free`，任何 C 程序员都看得懂；
- **Dynamic（动态）**：长度可变、容量有策略地预留，可以抵抗频繁修改带来的抖动；
- **String（字符串）**：对外依然长得像“字符串”，还能和传统 C API 比较平滑地打交道。

它做的事情，用一句话概括就是：

> 在原始字符数组前面，加上一个“小脑袋”，  
> 把“当前长度”和“剩余空间”这些关键信息都记在里面。

这个“小脑袋”在代码里大概是这样：

```cpp
// 伪代码：SDS 的核心结构
struct sdshdr {
    // ✅ 已用长度：记录 buf 中已占用的字节数
    unsigned int len;

    // ✅ 剩余空间：记录 buf 中还剩多少空闲字节
    unsigned int free;

    // ➡️ 真实数据：字节数组，真正存储字符串内容
    char buf[];
};
```

`len` 和 `free` 这两个字段，就是 SDS 的“账本”。

有了它们，很多原本需要遍历才能知道的信息，现在只要抬手读一个整数就够了。

从工程视角看，这一步其实是：**把“字符串的真实使用方式”压成了一个更贴近业务的内存布局**。后面所有的性能、安全特性，几乎都是这两个字段“顺势而来”的副产品。

### SDS vs C 字符串：一场很有工程味的对照实验

有了 `len` 和 `free` 这两个“小账本”，SDS 在几个关键问题上，对传统 C 字符串做了非常务实的改造。

#### 1. 问长度？O(1) 张口就来

- C 字符串：`strlen(s)` -> O(N)，每次都要从头走到尾。
- SDS：`sdslen(s)` -> 直接返回 `s->len`，O(1)。

在本地小工具里，这点差异没什么感觉；  
但在 Redis 这种每秒处理数十万请求的服务里，把“高频 O(N)”换成“O(1) 读字段”，就是扎扎实实的 CPU 节省。

#### 2. 缓冲区溢出：用协议消灭一类 Bug

当你需要拼接字符串时，SDS 的 API 不再“傻乎乎地照搬”，而是先问一嘴：`free` 空间还够不够？

- **空间足够**：直接把新内容接在尾巴上。
- **空间不足**：提前扩容，换个大一点的缓冲区，再把旧数据搬过去。

```cpp
// 伪代码：sdsacat 函数逻辑
SDS* sdsacat(SDS* s, const char* t) {
    size_t t_len = strlen(t);
    // 1. 问管家：free 空间还够吗？
    if (s->free < t_len) {
        // 2. 不够？管家去申请更大的房子
        s = sdsMakeRoomFor(s, t_len);
    }
    // 3. 够了！把新数据搬进来
    memcpy(s->buf + s->len, t, t_len);
    // 4. 更新小本本
    s->len += t_len;
    s->free -= t_len;
    return s;
}
```

你可以把这理解成：**把“写之前先量尺子”这条工程共识，固化进了数据结构的协议里**。  
只要大家都走 SDS 的 API，就很难再写出“随手一 strcat 把缓冲区挤爆”的老 Bug。

#### 3. 二进制安全：真正的“万物皆字节”

SDS 判断字符串结束的唯一标准就是 `len`，它完全不依赖 `\0`。

这意味着：

- `buf` 里可以放心放任何字节，包括中间带一堆 `\0` 的二进制；
- 你可以用同一套 API 去操作 JSON 文本、协议报文、压缩后的二进制数据。

这个特性，直接把 Redis 从“简单的 key-value 文本存储”，推向了“通用二进制数据管道”的位置。

#### 4. 空间换时间：为高频修改做预案

为了避免每次修改字符串都去“麻烦”操作系统这位大忙人，SDS 采用了两种非常现实的策略：

- **空间预分配**：扩容时，不是“刚好够用”，而是“稍微多一点”：
  - 如果修改后 `len` < 1MB，直接给你 `2 * len` 的容量；
  - 如果修改后 `len` ≥ 1MB，再多送 1MB。

- **惰性空间释放**：截短字符串时，多出来的空间不会立刻还给系统，而是记录在 `free` 里，留给后续可能的增长。

从抽象角度看，这就是一句话：**用一点可控的“浪费”，换掉大量重复的“分配-拷贝-释放”抖动**。  
这类策略在 Redis 里随处可见：只要是高频路径，就尽量用时间换空间、用顺序访问换随机访问。

### 用 C++ “山寨”一个 SDS：把概念落到手上

光说不练容易变成“概念背诵”。为了把 SDS 的设计变成你手上的肌肉记忆，我们可以用 C++ 山寨一个极简版的 SDS，当成一套可运行的“心智模型”。

#### Step 1：搭个蓝图——`SimpleSDS` 类的骨架

先不用急着对齐 Redis 源码，先搭一个最小可用模型。`SimpleSDS` 类里，就三样东西：

- 记录当前内容长度的“小本本”；
- 记录当前容量的“地契”；
- 存放真实数据的“仓库”。

```cpp
#include <iostream>
#include <string>
#include <cstring> // for memcpy, strlen

// 蓝图：一个简化版的 SDS
class SimpleSDS {
private:
    unsigned int len_;      // 记录当前长度
    unsigned int capacity_; // 记录房子总大小 (len + free)
    char* buf_;             // 存放真实数据

public:
    // 构造函数：如何“出生”
    SimpleSDS(const char* init_str = "");

    // 析构函数：如何“善后”
    ~SimpleSDS();

    // 核心功能：拼接字符串
    void append(const char* t);

    // 辅助功能
    size_t length() const { return len_; }
    const char* c_str() const { return buf_; }
};
```

`len_` 和 `capacity_` 就好比 SDS 里的 `len` 和 `len + free`，`buf_` 则是那块真正装东西的缓冲区。

#### Step 2：构造与析构——对象的一生

一个对象的旅程，从构造函数开始。它负责根据你给的初始字符串，为 `SimpleSDS` 申请一块不大不小的“宅基地”，把数据安顿好；  
有生就有死，析构函数负责在对象销毁时，把申请的内存还给系统。

```cpp
// 构造函数：初始化我们的“智能字符串”
SimpleSDS::SimpleSDS(const char* init_str) {
    len_ = strlen(init_str);
    capacity_ = len_; // 刚出生时，容量不多不少，正好等于长度
    buf_ = new char[capacity_ + 1]; // +1 给末尾的 '\0' 留位置
    memcpy(buf_, init_str, len_);
    buf_[len_] = '\0'; // 兼容 C 风格函数，让 printf 也能用
}

// 析构函数：释放内存
SimpleSDS::~SimpleSDS() {
    delete[] buf_;
}
```

注意这里我们依然保留了末尾的 `\0`，主要是为了方便和现有 C / C++ 库联动。真正的“权威长度”还是 `len_`。

#### Step 3：实现 `append`——把“预分配”写进代码

接下来轮到 `append` 登场了。它负责一件事：在尽量少打扰操作系统的前提下，让字符串“长胖”。

我们先看它的外形：就是一个接收 C 字符串的成员函数。

```cpp
void SimpleSDS::append(const char* t) {
    size_t t_len = strlen(t);
    // ...
}
```

第一步只是算一算这次要追加多少字节。真正有意思的是接下来这段“算账”的逻辑。

```cpp
    // 1. 检查剩余空间 (capacity - len 就是 free)
    if (capacity_ - len_ < t_len) {
        // 2. 不够？扩容（这里采用双倍扩容策略）
        capacity_ = (len_ + t_len) * 2;
        char* new_buf = new char[capacity_ + 1];

        // 搬家：把旧数据复制到新房子
        memcpy(new_buf, buf_, len_);
        delete[] buf_;
        buf_ = new_buf;
    }
```

这部分就是把 **“空间预分配”** 写进代码本身：与其每次刚好分配够本次使用的空间，不如趁机多留一截，给后面的追加操作预热。

最后，再把本次要追加的内容，安稳地接在尾部：

```cpp
    // 3. 把新数据接到尾部
    memcpy(buf_ + len_, t, t_len);
    len_ += t_len;
    buf_[len_] = '\0';
}
```

如果把这一小段逻辑抽象出来，其实就是一句话：

> 在“能用”和“好用”之间，再往前走半步，  
>  主动花一点可控的空间，把将来一大堆重复的小开销提前买断。

#### Step 4：组装测试——让模型跑起来

最后，我们把所有零件组装起来，写一个 `main` 函数跑一跑。

```cpp
// 为了让代码块能独立运行，我们将类的定义和实现放在一起
#include <iostream>
#include <string>
#include <cstring>

class SimpleSDS {
private:
    unsigned int len_;
    unsigned int capacity_;
    char* buf_;

public:
    SimpleSDS(const char* init_str = "") {
        len_ = strlen(init_str);
        capacity_ = len_;
        buf_ = new char[capacity_ + 1];
        memcpy(buf_, init_str, len_);
        buf_[len_] = '\0';
        std::cout << "构造: \"" << buf_ << "\", len: " << len_ << ", cap: " << capacity_ << std::endl;
    }

    ~SimpleSDS() {
        delete[] buf_;
    }

    void append(const char* t) {
        size_t t_len = strlen(t);
        if (capacity_ - len_ < t_len) {
            capacity_ = (len_ + t_len) * 2;
            char* new_buf = new char[capacity_ + 1];
            memcpy(new_buf, buf_, len_);
            delete[] buf_;
            buf_ = new_buf;
            std::cout << "扩容! 新容量: " << capacity_ << std::endl;
        }
        memcpy(buf_ + len_, t, t_len);
        len_ += t_len;
        buf_[len_] = '\0';
        std::cout << "拼接后: \"" << buf_ << "\", len: " << len_ << ", cap: " << capacity_ << std::endl;
    }

    size_t length() const { return len_; }
    const char* c_str() const { return buf_; }
};

int main() {
    SimpleSDS s("Hello");
    s.append(", World!");
    s.append(" This is a long string to trigger reallocation.");
    return 0;
}
```

运行结果大致类似：

```text
构造: "Hello", len: 5, cap: 5
扩容! 新容量: 26
拼接后: "Hello, World!", len: 13, cap: 26
扩容! 新容量: 104
拼接后: "Hello, World! This is a long string to trigger reallocation.", len: 52, cap: 104
```

这段小实验并不是为了“复刻 Redis 源码”，而是帮你在脑子里刻下一幅图：**一个字符串，在“长度、容量、数据”这三维上，是怎么协同工作的**。

### 总结：从 SDS 身上，C++ 程序员能学到什么？

把视角从 Redis 源码里抽出来，SDS 其实在教我们几件事情：

1. **先看清“真实使用方式”，再设计数据结构**  
   C 字符串的“线性 + `\0`”非常简单，但和今天的使用场景已经错位；  
   SDS 则是反过来：先承认“会频繁修改、会存二进制、会跑在高并发服务器里”，再倒推内存布局。

2. **用一点额外的元数据，换掉一大片隐性成本**  
   多存两个字段，看起来“浪费了 8 个字节”，  
   换来的却是 O(1) 长度查询、内建防溢出机制、策略性扩容——这类 trade-off 在工程里几乎处处可见。

3. **敢于在标准库之外，写一个“更贴你场景”的轮子**  
   在 C++ 世界里，`std::string` 很优秀，但它不可能为每个具体业务场景都做到极致；  
   理解 SDS 的设计之后，你会更敢在自己的项目里，为日志、协议缓冲区、热点字符串路径，量身定做一个“小型 SDS”。

所以下次面试再聊字符串，别只停在“`std::string` vs `char*`”的层面了。  
顺着 Redis 的 SDS 讲一讲“为什么要这样设计”“背后优化了哪些实际代价”，  
既能让新人听得懂，也足够让老手点头。

