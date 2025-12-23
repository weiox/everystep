---
title: std::format：类型安全的格式化
description: C++20 把 fmtlib 那套“像 printf 一样顺手、但像模板一样安全”的格式化能力收进了标准库。
---

`printf` 这套东西。

来自 C 的年代。

那时候机器慢。

内存小。

接口越薄越好。

可变参数一上。

大家都觉得很优雅。

然后代价就被悄悄塞进了“约定”。

格式串要和参数对齐。

你不对齐。

它也不一定立刻骂你。

它可能当场崩。

也可能装作没事。

```cpp
#include <cstdio>

void demo(int x) {
    std::printf("x=%s\n", x);
}
```

这段代码。

问题不在 `std::printf`。

问题在 `%s`。

它期待的是一个指针。

你却递了一个整数。

这就是 C 时代的气质。

“我相信你。”

“你别骗我。”

顺便说一句。

你会看到 `std::printf`，也会看到 `printf`。

这不是两种函数。

这是两种头文件。

你 `#include <cstdio>`。

标准保证 `std::printf` 在 `std` 里。

至于全局的 `::printf`。

很多实现也会给。

但你别把“很多”当成“必然”。

### 后来：iostream 讲道理，但也磨人

到了 C++。

大家开始想。

能不能别再靠人脑对齐格式串。

于是有了 `iostream`。

类型安全。

也更 C++。

但它有个副作用。

你写着写着。

就像在拧水管。

```cpp
#include <sstream>
#include <string>

std::string make_msg(int id, std::string_view name) {
    std::ostringstream oss;
    oss << "id=" << id << ", name=" << name;
    return oss.str();
}
```

这段代码当然没错。

但它不“像一句话”。

它更像是。

“先买个桶”。

“再一勺一勺往里倒”。

你最烦的不是啰嗦。

是阅读的时候。

格式被拆碎了。

意图反而不显眼。

### Python 把火点起来：fmtlib 先跑了一步

再后来。

很多团队开始用 `fmt`（fmtlib）。

原因很朴素。

它像 `printf` 一样直观。

又像模板一样讲理。

你写的是一条格式。

不是一串操作。

```cpp
// fmt::format("id={}, name={}", 7, "alice")
```

它太好用了。

好用到标准委员会也装不下去了。

于是 C++20 把这套模型收进来。

起名叫 `std::format`。

你可以把它理解成。

“标准库终于承认：字符串格式化不是小事。”

### std::format：把意图写成一句话

最常见的场景。

是拼日志。

拼错误信息。

拼一个你希望人能读懂的句子。

```cpp
#include <format>

auto msg = std::format("id={}, name={}", 7, "alice");
```

这里的 `{}`。

就是“把参数放进来”。

它返回 `std::string`。

你得到的是一个结果。

不是一个过程。

### 你迟早会踩的一个细节：花括号怎么输出

你写 JSON。

写模板。

写配置。

花括号本身就会出现在文本里。

这时候要写双份。

```cpp
#include <format>

auto s = std::format("{{\"ok\": {}}}", true);
```

输出就是。

`{"ok": true}`。

这不魔法。

只是约定。

### 一段真实的业务味道：日志对齐

你写过排障日志。

就知道对齐有多值钱。

人眼扫日志。

最怕列不齐。

```cpp
#include <format>

auto line = std::format("[{:<5}] {:>8}", "INFO", 42);
```

`{:<5}` 是左对齐。

`{:>8}` 是右对齐。

你看一眼就懂。

不需要记“上一次设置了 std::setw 还是 std::left”。

### 数字那点事：十六进制和精度

写协议。

写哈希。

写调试输出。

十六进制很常见。

```cpp
#include <format>

auto hex = std::format("0x{:08x}", 48879);
```

你得到 `0x0000beef`。

宽度。

补零。

全在一处。

浮点也一样。

```cpp
#include <format>

auto pi = std::format("{:.3f}", 3.1415926);
```

输出 `3.142`。

### 它为什么“更安全”：错误更早暴露

`printf` 的错误。

很多是运行时才炸。

`std::format` 的思路是。

能在编译期查的。

尽量别拖到线上。

当你的格式串是字面量。

并且实现支持编译期检查时。

一些不匹配会更早被拦下。

你可以把它理解成。

“把深夜事故，提前到白天编译错误”。

这其实很划算。

因为编译器骂你。

总比用户骂你强。

### 但现实总是现实：格式串有时来自运行时

配置文件给了你一个模板。

用户自己定义了一个展示格式。

你就没法指望编译期检查。

这时别硬上 `std::format` 的字面量那套。

用 `std::vformat`。

```cpp
#include <format>
#include <string>

std::string format_runtime(std::string_view fmt, int x, int y) {
    return std::vformat(fmt, std::make_format_args(x, y));
}
```

这时候出错。

会抛 `std::format_error`。

你要接受。

这是运行时世界的规矩。

### format_to：别老盯着 string，输出也可以是“流向”

有些路径。

你真的在乎性能。

比如你在拼一大段日志。

你不想反复创建临时字符串。

你更希望。

把结果直接写进容器。

```cpp
#include <format>
#include <iterator>
#include <string>

std::string out;
std::format_to(std::back_inserter(out), "[{}] {}", 200, "OK");
```

它看起来像算法。

本质也确实像。

“把格式化后的字符流，写到一个输出迭代器”。

### 自定义类型：别再手搓 to_string 了

工程里一定有自己的类型。

比如坐标。

比如订单号。

比如你们那个叫 `FooContext` 的老结构体。

你肯定不想每次输出都手写拼接。

你想要的是。

它能自然地进入 `{}`。

```cpp
#include <format>

struct Point {
    int x;
    int y;
};
```

你为它提供一个 `std::formatter`。

```cpp
template <>
struct std::formatter<Point> {
    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const Point& p, std::format_context& ctx) const {
        return std::format_to(ctx.out(), "({}, {})", p.x, p.y);
    }
};
```

然后它就可以像内置类型一样出现。

```cpp
#include <format>

auto s = std::format("p={}", Point{1, 2});
```

我喜欢这件事。

因为它不是“为了炫技”。

它是在告诉你。

格式化也是接口。

也是约定。

你把约定写进类型系统。

后面的人就少猜一点。

### 最后说句实话：std::format 不是到处都能用

标准是 C++20。

但实现跟上需要时间。

你在某些环境里。

可能会遇到“头文件有了，功能没全”。

这不丢人。

这是现实。

如果你要兼容老平台。

`fmt` 依然是很好的选择。

而且迁移心智成本很低。

因为它们长得很像。

### 小结

`std::format` 没有发明新需求。

它只是承认了一个老事实。

字符串格式化是生产力。

也是事故源。

你要的不是“能拼出来”。

你要的是。

代码读起来像一句话。

错误尽量早点暴露。

那种半夜被日志嘲笑的日子。

能少一点。
