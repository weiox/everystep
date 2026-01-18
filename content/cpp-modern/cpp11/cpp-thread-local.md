---
title: 'thread_local：每个线程一份的“全局变量”'
description: 'C++11 把线程局部存储变成标准能力：thread_local 让每个线程拥有自己的那份状态，不再手写 pthread_key 或编译器扩展。'
---

有段时间。

我们写程序。

写着写着就会开始用线程。

一个请求一个线程。

或者一个线程池。

那时候的痛点很朴素。

我想要一个“全局变量”。

但我只想让它对当前线程有效。

别的线程别来碰瓷。

### 那些年：没有“每线程一份全局”的日子

先把“线程”说清楚。

线程你可以把它理解成。

同一个进程里。

同时跑着的多条执行路线。

它们共享同一份全局数据。

所以全局变量在它们眼里。

就像一个公共白板。

谁都能写。

也就谁都能把你刚写的擦掉。

早年的工程里。

大家很快会遇到一类“看起来像全局”的状态。

比如错误码。

比如本次请求的 id。

比如一个小小的统计计数。

你不希望它跨线程串味。

但你也不想每个函数都带着它。

因为参数传着传着。

就像背着行李赶路。

走两步就喘。

### 先补一口基础：线程到底共享了什么

刚学 C 的时候。

你看到的“全局变量”。

基本就等于“整个程序都能用”。

到了线程这里。

事情会变得更具体一点。

一个进程里会有很多线程。

它们通常共享同一片地址空间。

也就是。

同一份全局变量。

同一份堆内存（你 `malloc/new` 出来的那堆）。

```cpp
int g = 0;

void f() {
    ++g;
}
```

这行 `++g` 在单线程里很老实。

在多线程里。

它就不是“一次操作”。

它可能会被拆成“读、加一、写回”。

两个线程一掺和。

结果就开始漂。

### 你以为是“偶发 bug”，其实叫数据竞争

并发里有个词。

叫“数据竞争”（data race）。

意思很朴素。

两个线程。

同时访问同一份数据。

至少有一个在写。

还没有任何同步手段。

在 C++ 里。

这会直接变成“未定义行为”。

也就是。

语言标准不保证你会看到什么。

不保证“只是结果错一点”。

也不保证“加个 sleep 就好了”。

它甚至不保证你能复现。

你今天能跑。

明天换个编译选项。

就炸给你看。

你别指望它每次都错。

它会挑你最忙的时候错。

你要是想亲眼看一次。

可以用最土的方式。

两个线程。

各自加一堆。

```cpp
#include <iostream>
#include <thread>

int g = 0;

void work() {
    for (int i = 0; i < 100000; ++i) {
        ++g;
    }
}

int main() {
    std::thread a(work);
    std::thread b(work);
    a.join();
    b.join();
    std::cout << g << "\n";
}
```

你心里会期待 `200000`。

但你跑几次。

很可能会看到别的数字。

它不是“少加了几次”那么简单。

它是在提醒你。

这里的行为不再由语言保证。

### 你可能会想到的“歪招”：volatile

学 C 的时候。

你可能听过一句话。

“多线程就加 volatile”。

这句话很坑人。

`volatile` 主要是跟编译器说。

“别把这次读写优化掉”。

它不负责让两个线程“同步”。

```cpp
#include <iostream>
#include <thread>

volatile int done = 0;

void worker() {
    done = 1;
}

int main() {
    std::thread t(worker);
    while (done == 0) {
        // 你以为这里会很快退出
    }
    t.join();
    std::cout << "ok\n";
}
```

这段代码“看起来应该能跑”。

但你别把它当结论。

并发里要谈正确性。

靠的是锁和原子操作。

不是 `volatile`。

### 线上啪一下：全局变量在多线程下打架

我讲个很小的事故。

一个小项目。

我在日志里想打出“本次请求 id”。

于是我偷懒用了一个全局变量。

```cpp
#include <iostream>
#include <string>
#include <thread>

std::string g_request_id;

void handle(const char* id) {
    g_request_id = id;
    std::cout << "log request=" << g_request_id << "\n";
}

int main() {
    std::thread a(handle, "A");
    std::thread b(handle, "B");
    a.join();
    b.join();
}
```

这段代码有时候会输出。

`A` 的处理函数里。

打印出了 `B`。

原因并不神秘。

两个线程共享同一份 `g_request_id`。

你刚写进去。

我就覆盖掉。

日志就开始胡说八道。

线上排查的时候。

你会觉得世界不可信。

### 老办法：参数、锁、和一串“钥匙”

当年大家当然不是不会解决。

只是解决得很狼狈。

第一种。

把 request id 当参数传下去。

能跑。

代价是每一层都要改签名。

改到最后。

你会怀疑自己是在写业务。

还是在搬运参数。

第二种。

给全局变量加锁。

能保正确。

但它解决的是“同时写”。

解决不了“每个线程要自己的那份”。

你可以更直观地感受一下。

```cpp
#include <mutex>
#include <string>

std::mutex m;
std::string g_request_id;

void handle(const char* id) {
    std::lock_guard<std::mutex> lk(m);
    g_request_id = id;
    // 在锁里打印，确实不会串
}
```

现在它不会互相覆盖。

但代价也很明显。

你把所有线程。

都排成了一条队。

而且它仍然只有“一份” `g_request_id`。

你想同时保留 A 和 B 两个请求的上下文。

它做不到。

这时候有人会说。

那我用 `std::atomic` 呢。

确实。

它能解决“同时写”的正确性。

但它仍然解决不了“每线程一份”。

```cpp
#include <atomic>

std::atomic<int> g{0};

void hit() {
    ++g;
}
```

`g` 还是那一个 `g`。

大家只是更文明地抢它。

而且你会把一个本来很轻的日志点。

变成一个隐形的性能热点。

第三种。

用平台 TLS（线程局部存储）。

比如 pthread 那套 `pthread_key_t`。

它的核心思路是。

你给每个线程发一把“钥匙”。

线程用这把钥匙。

去自己的小柜子里取东西。

```cpp
// 伪代码，意思是“每个线程都有一格自己的存储”
pthread_key_t key;
pthread_key_create(&key, destructor);

pthread_setspecific(key, ptr);
auto* p = pthread_getspecific(key);
```

能用。

但读起来不像 C++。

更像系统 API 使用说明书。

还有一堆 `void*`。

类型靠自觉。

出错靠缘分。

你要是问。

TLS 到底是什么。

你可以先别把它想得很底层。

就把它想成。

每个线程都自带一个“小抽屉”。

抽屉里有很多格子。

key 就是格子的编号。

```cpp
// 仍然是伪代码
// 每个线程：thread.locals[key] = value

set(key, value);
value = get(key);
```

POSIX 给你 pthread 这套。

Windows 也有自己的 TLS API。

大家做的其实是一件事。

只是接口长得不一样。

### 这玩意儿是怎么来的：别人早就踩过坑

其实“每个线程一份状态”并不是 C++ 才想明白。

很多系统和语言。

早就被它折磨过。

一个经典例子是 C 里的 `errno`。

你写过 `fopen` 失败。

再去看 `errno`。

如果 `errno` 是全局变量。

多线程下它会被别人改掉。

所以主流实现里。

`errno` 很早就做成了“线程局部”的。

只是你平时不需要知道它怎么实现。

如果你翻过老代码。

你还可能见过一种写法。

把 “errno” 写成宏。

宏背后其实是函数。

函数返回“当前线程的那份 errno 的地址”。

这也是一种。

把 TLS 藏起来的工程手段。

另一个例子是 Java。

Java 里有个类叫 `ThreadLocal`。

它的思路也很像“钥匙+柜子”。

线程里放一个 map。

用 key 找到自己的那份值。

```cpp
// 还是伪代码，意思是 Java 那套 ThreadLocal 的味道
// Thread -> Map<ThreadLocalKey, Value>

thread.map[key] = value;
value = thread.map[key];
```

你用起来像普通变量。

背后其实是查表。

还有 C11。

它有个关键字 `_Thread_local`。

也是同一件事。

只不过 C11 更偏“C 风格”。

对 C++ 对象的构造析构。

它不替你操心。

在 Python 里。

有 `threading.local()`。

用法也很像“每线程一份属性”。

在 .NET 里。

也有 `ThreadLocal<T>`。

这些东西背后的味道都差不多。

大家都在解决同一类问题。

只是各自把代价放在不同地方。

有的把代价放在“查表”。

有的把代价放在“运行时/加载器支持”。

而 C++11 的 `thread_local`。

选择了让语法最像普通变量。

代价则更多交给实现去兜底。

C++11 做的事情。

更像是把“各家已经在做的 TLS”。

搬进标准里。

给你一个统一的写法。

顺便把 C++ 的对象模型也照顾到。

### 编译器扩展：__thread 能用，但不够体面

在 C++11 之前。

你可能见过 `__thread`。

或者 Windows 上的 `__declspec(thread)`。

它们本质上是。

编译器直接给你一个 TLS 变量。

```cpp
// 旧时代的写法（示意）
__thread int counter;
```

能用。

但它有很多“条件”。

比如跨平台写起来很痛。

比如对带构造/析构的 C++ 对象。

不同编译器、不同链接方式。

规矩不统一。

你写的是工程。

不是赌运气。

### C++11：thread_local 把“柜子”搬进了语言里

C++11 给了一个关键字：`thread_local`。

你可以把它理解成。

“这个变量不是进程一份。”

“而是每个线程一份。”

语法看起来很像 `static`。

但语义换了地方。

```cpp
thread_local int counter = 0;

void hit() {
    ++counter;
}
```

现在每个线程都有自己的 `counter`。

线程 A 递增的是 A 的。

线程 B 递增的是 B 的。

它们不需要互相协调。

因为根本不是同一份数据。

### 它到底像 static 像在哪儿

你可能会注意到。

我一直说它“像 `static`”。

这里的像。

主要是存储期的感觉。

`thread_local` 修饰的变量。

你可以把它理解成。

“生命周期跟线程绑定”。

线程第一次需要它的时候构造。

线程结束的时候析构。

注意这里有个细节。

你不用去背标准条文。

先记住一个实用的直觉。

它会在“该线程第一次用到它”之前完成初始化。

然后一直活到线程结束。

函数里的 `thread_local`。

更像“每线程一份的局部静态变量”。

每个线程第一次走到那行声明。

才会初始化那一份。

而且它不只可以放在全局。

也可以放在函数里。

```cpp
int& per_thread_counter() {
    static thread_local int x = 0;
    return x;
}
```

同一个线程里。

你每次拿到的都是同一个 `x`。

换一个线程。

就是另一份 `x`。

你要是还不放心。

还有个很笨但很有效的验证方法。

把地址打印出来。

```cpp
#include <iostream>
#include <thread>

thread_local int x = 0;

void show() {
    std::cout << &x << "\n";
}

int main() {
    std::thread a(show);
    std::thread b(show);
    a.join();
    b.join();
}
```

你会看到两个不同的地址。

这就是“每个线程一份”。

### 把事故修掉：请求 id 不再串味

把前面的全局变量。

换成 `thread_local`。

```cpp
thread_local std::string request_id;

void handle(const char* id) {
    request_id = id;
    std::cout << "log request=" << request_id << "\n";
}
```

这时候你打印的。

就是“本线程处理的那次请求”。

不用把 id 一路往下传。

也不用为了打个日志上锁。

### 新手最容易踩的坑：线程池会“保留记忆”

这里有个坑。

很像“线上啪一下”的那种坑。

你以为一次请求结束。

线程上下文就清空了。

但如果你用的是线程池。

线程不会退出。

它会拿着上一单的 `thread_local`。

接着处理下一单。

```cpp
thread_local std::string request_id;

void on_request(const char* id) {
    request_id = id;
    // ... 处理请求
    request_id.clear();
}
```

这类 bug 的味道很冲。

日志看起来“偶尔串”。

你查半天。

发现根本不是并发写。

而是“脏数据没清”。

所以更稳的习惯是。

每次处理请求开头就设置。

处理结束就清空。

这不是洁癖。

这是在跟线程池的“长寿命”打交道。

### 另一个常见坑：别把它当“自动传参”

thread_local 很容易让人上头。

因为它太方便了。

方便到你看不见依赖。

```cpp
thread_local int current_user = -1;

int price() {
    // 读者看函数签名，根本不知道它依赖了 current_user
    return current_user == 0 ? 1 : 2;
}
```

这种写法在小项目里很爽。

一旦函数开始复用。

或者你要写单元测试。

你就会发现。

它像一条暗线。

把状态偷偷塞进了代码里。

所以我的经验是。

它适合“线程上下文”。

不适合“业务上下文”。

### 你得知道的现实：它不是免费的

`thread_local` 有成本。

尤其是变量是“带构造/析构”的对象时。

每个线程都会构造一份。

线程退出还要析构一遍。

如果你把一个很重的东西塞进去。

比如一个巨大的缓存。

线程多起来。

内存就会跟着涨。

而且有些线上抖动。

你最后会发现。

根源是“线程创建/退出时的那点初始化和清理”。

所以我更愿意把它用在。

小而纯的状态。

或者明确属于线程上下文的东西。

### 一个常见的好用法：每线程一个小工具

有些东西。

你真的不想每次都 new。

也不想所有线程抢一份。

比如随机数引擎。

```cpp
#include <random>

int rnd() {
    static thread_local std::mt19937 gen{std::random_device{}()};
    return static_cast<int>(gen());
}
```

每个线程初始化一次。

之后一直复用。

互不打扰。

还有一种很土但很实用的用法。

每线程一个小缓冲区。

避免频繁申请释放。

```cpp
#include <array>

std::array<char, 1024>& scratch() {
    static thread_local std::array<char, 1024> buf{};
    return buf;
}
```

你不用上来就理解内存分配器。

你只要知道。

这类“临时空间”。

让每个线程各用各的。

往往比大家抢一块要省心。

### 最后留一句话

thread_local 不是让你继续迷恋全局。

它只是给“线程上下文”一个体面的住处。
