---
 title: lambda 表达式：匿名函数与闭包
 description: C++11 的 lambda 让“临时写个小函数”变成日常操作，也把回调、算法和并发写得更顺手。
---

lambda 不是突然冒出来的。

它是 C++ 很长一段历史里。

“回调”和“算法”把人逼出来的产物。

九十年代 STL 把算法搬进标准库。

`sort`、`find_if`、`for_each` 这些东西一出来。

大家就开始不断地问一个问题。

比较规则放哪。

过滤条件放哪。

那会儿最省事的做法是函数指针。

```cpp
bool by_length(const std::string& a, const std::string& b) {
    return a.size() < b.size();
}
```

能用。

但它带不走上下文。

你想要一个“阈值”。

想要一个“表”。

想要一个“配置”。

函数指针就开始装死。

于是 C++ 的老派解法登场。

函数对象。

也就是写个类。

把状态塞进去。

再实现 `operator()`。

它很强。

也很啰嗦。

而且只要你写过一堆一次性比较器。

你就会明白。

这不是“工程设计”。

这是“为了写一行算法调用，先铺十行仪式”。

再后来。

社区开始用库来补洞。

Boost 里就出现过好几代“库级 lambda”。

表达力很猛。

代价也很猛。

编译错误像瀑布。

编译时间像冬天。

这些尝试的价值不在于让你天天用。

它们更像是在跟委员会说。

> 需求已经摆在桌上了。
>
> 不要逼大家继续用库硬凑。

到了 C++0x（后来改名 C++11）。

标准委员会要解决的不只是“写起来帅不帅”。

而是两件更朴素的事。

让算法和回调能在现场写。

让状态能安全地跟着走。

于是 lambda 作为语言特性进了标准。

它的语法也很直白。

`[]` 是口袋。

口袋里装捕获。

`()` 是参数。

`{}` 是函数体。

下面我们先从“没有 lambda 的年代”开始。

你会看到。

它到底替你省掉了哪些仪式。

### 没有 lambda 的年代

先看一个最常见的场景。

按长度排序字符串。

C++11 之前你经常这么写。

```cpp
#include <algorithm>
#include <string>
#include <vector>

struct ByLength {
    bool operator()(const std::string& a, const std::string& b) const {
        return a.size() < b.size();
    }
};

int main() {
    std::vector<std::string> v{"aaa", "b", "cc"};
    std::sort(v.begin(), v.end(), ByLength{});
}
```

能用。

也够“正统”。

但它有一个很现实的问题。

这段比较逻辑只在这里用一次。

你还是得给它起名字。

还得给它放个地方。

最后你得到一堆“只用一次的类型”。

代码像抽屉里的塑料袋。

越攒越多。

### lambda 的核心：把临时函数写在原地

C++11 的 lambda 长这样。

```cpp
[]() {
}
```

它看起来像三块。

`[]`。

`()`。

`{}`。

我习惯把它理解成。

“我现在要在这里造一个函数对象”。

最简单的例子。

```cpp
auto f = []() {
    return 42;
};

int x = f();
```

`f` 不是函数指针。

它是一个匿名类型的对象。

它有一个 `operator()`。

所以你可以像调用函数一样调用它。

这就是为什么很多人说。

lambda 的本质是“匿名函数对象”。

### 回到排序：把比较逻辑贴回现场

刚才那段排序。

现在可以写成这样。

```cpp
#include <algorithm>
#include <string>
#include <vector>

int main() {
    std::vector<std::string> v{"aaa", "b", "cc"};

    std::sort(v.begin(), v.end(),
              [](const std::string& a, const std::string& b) {
                  return a.size() < b.size();
              });
}
```

你一眼就知道它在比什么。

你也不用去找 `ByLength` 在哪。

这对“读代码”来说，是实打实的效率。

### 那对方说的“闭包”到底是什么

你会听到另一句话。

lambda 是闭包。

别被术语吓到。

它讲的其实就是一件事。

lambda 可以“带着环境一起走”。

也就是捕获。

### 捕获：把外面的变量带进来

先看一个最朴素的。

我们想筛掉长度小于某个阈值的字符串。

阈值是运行时决定的。

```cpp
#include <algorithm>
#include <string>
#include <vector>

int main() {
    std::vector<std::string> v{"aaa", "b", "cc"};
    std::size_t limit = 2;

    auto it = std::find_if(v.begin(), v.end(),
                           [limit](const std::string& s) {
                               return s.size() >= limit;
                           });

    (void)it;
}
```

这里的 `[limit]` 就是捕获列表。

它表示。

把外面的 `limit` 按值拷贝一份。

拷贝进这个 lambda 对象里。

所以它才叫“闭包”。

它把 `limit` 这份上下文封在里面了。

### 按值捕获 vs 按引用捕获

按值捕获。

安全。

但它不会跟着外界变化。

```cpp
int x = 1;
auto f = [x]() { return x; };

x = 2;
int v = f(); // 还是 1
```

按引用捕获。

灵活。

但你就要开始关心生命周期。

```cpp
int x = 1;
auto f = [&x]() { return x; };

x = 2;
int v = f(); // 变成 2
```

这就是工程里经常踩的坑。

lambda 把引用带走了。

变量却早就死了。

尤其是你把 lambda 塞进回调。

塞进线程。

塞进异步任务。

那就更容易“看着没问题”。

然后在某个晚上炸。

### 让 lambda 修改按值捕获：mutable

C++11 里。

按值捕获默认是只读的。

```cpp
int x = 1;

auto f = [x]() {
    // x++; // ❌ 不允许
    return x;
};
```

如果你确实想改那份“拷贝进来的 x”。

可以加 `mutable`。

```cpp
int x = 1;

auto f = [x]() mutable {
    x++;
    return x;
};

int a = f(); // 2
int b = f(); // 3
```

注意。

你改的是闭包对象内部的那份副本。

不是外面的 `x`。

### 捕获 this：很方便，也很危险

成员函数里写 lambda。

很自然会捕获 `this`。

```cpp
#include <functional>

struct Worker {
    int base = 10;

    std::function<int(int)> make() {
        return [this](int x) {
            return base + x;
        };
    }
};
```

这段代码能跑。

但风险也很明显。

你把 lambda 返回出去了。

lambda 里握着 `this`。

如果 `Worker` 先销毁。

lambda 再被调用。

那就是悬空指针。

很多“回调偶现崩溃”。

背后就是这种故事。

在 C++11 里。

你得自己保证对象活得比回调久。

或者让回调捕获 `shared_ptr`。

别让 `this` 裸奔。

### 返回类型：大多数时候不用写

lambda 的返回类型通常可以自动推导。

```cpp
auto f = [](int x) {
    return x + 1;
};
```

但有一个经典场景会卡住。

分支返回不同类型。

```cpp
auto g = [](bool ok) {
    if (ok) {
        return 1;
    }
    return 0; // 这还好
};
```

如果分支里返回的类型不一致。

你就需要明确写返回类型。

```cpp
auto h = [](bool ok) -> int {
    if (ok) {
        return 1;
    }
    return 0;
};
```

C++11 的规则比较朴素。

它希望你别让编译器猜你到底想要什么。

### 把 lambda 放到哪里

lambda 的类型是匿名的。

所以你最常见的接法是 `auto`。

```cpp
auto pred = [](int x) { return x > 0; };
```

但有时你需要把它存到容器里。

或者做成接口返回。

这时很多人会用 `std::function`。

```cpp
#include <functional>

std::function<int(int)> f = [](int x) { return x + 1; };
```

这能用。

代价也真实。

`std::function` 是类型擦除。

可能有一次堆分配。

也会引入间接调用。

性能敏感路径上。

你要心里有数。

如果你在写模板库。

更常见的做法是。

让调用方把 lambda 作为模板参数传进来。

```cpp
template <class F>
int apply(F f, int x) {
    return f(x);
}

int main() {
    int v = apply([](int x) { return x + 1; }, 41);
    (void)v;
}
```

这样通常可以内联。

也不会产生类型擦除的开销。

### lambda 和线程：最容易踩的一个雷

你把 lambda 丢给线程。

最常见的坑是。

按引用捕获了一个局部变量。

然后线程还没跑完。

局部变量就没了。

```cpp
#include <thread>

std::thread spawn() {
    int x = 42;
    return std::thread([&]() {
        (void)x;
    });
}
```

这段代码的危险点不在语法。

在生命周期。

最稳的办法。

要么按值捕获。

要么把需要的东西搬进线程对象里。

你别让线程去借一个“马上要还的变量”。

### 小结

lambda 不是为了炫技。

它解决的，是“临时小逻辑到处放”的老毛病。

它让使用点和逻辑点贴在一起。

读代码的人少搬家。

写代码的人少起名。

但它也把一个更老的工程问题推到你面前。

生命周期。

你捕获什么。

捕获的是值还是引用。

`this` 能不能安全带出去。

这些问题。

在没有 lambda 的年代也存在。

只是那时你写得更啰嗦。

所以更容易意识到“我在把东西带走”。

到了 lambda。

一切变顺手了。

也就更容易顺手埋雷。

写得爽。

也得写得清醒。
