---
title: 'std::chrono：时间与计时'
description: 'C++11 用 chrono 把“秒/毫秒/纳秒”和“系统时钟/稳定时钟”都装进类型系统，避免 timeout=500 这种单位事故。'
---

时间这东西。

在工程里最容易变成谜语。

你写 `timeout = 500`。

500 什么。

毫秒。

秒。

还是“随便写个数”。

老代码里。

时间经常用 int。

然后靠注释。

注释一旦过期。

bug 就开始长出来。

C++11 的 `<chrono>`。

就是把单位拉进类型系统。

让你不靠注释。

靠类型。

## duration：先把单位讲清楚

```cpp
#include <chrono>

std::chrono::milliseconds t(500);
```

这行的意思很明确。

500 毫秒。

你想传给需要秒的接口。

编译器会逼你显式转换。

```cpp
auto s = std::chrono::duration_cast<std::chrono::seconds>(t);
```

它让“单位变更”变得可见。

## clock：时间不是一种时间

chrono 里最常见的三个时钟。

`system_clock`：系统时间，会跳。

`steady_clock`：单调递增，适合计时。

`high_resolution_clock`：实现相关。

你要测耗时。

通常用 steady_clock。

```cpp
auto beg = std::chrono::steady_clock::now();
work();
auto end = std::chrono::steady_clock::now();

auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - beg);
```

用 system_clock 去计时。

遇到系统时间回拨。

你会得到负耗时。

这不是笑话。

线上真的会发生。

## 小洞见

chrono 把“时间”拆成两层。

一层是单位。

一层是时钟来源。

你写对了。

你就少掉两类事故。

单位事故。

和时钟事故。

而这两类事故。

都特别喜欢在生产环境出现。

因为本地机器通常很正常。

线上机器通常不讲道理。
