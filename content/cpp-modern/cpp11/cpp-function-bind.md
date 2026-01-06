---
title: 'std::function / std::bind：可调用对象的统一接口'
description: 'C++11 把“能调用的东西”统一起来：function 做类型擦除，bind 做参数绑定；但也要知道它们的成本和陷阱。'
---

C++11 之前。

你想把一个回调传进去。

通常只有两条路。

函数指针。

或者模板。

函数指针很轻。

但它带不走状态。

模板很强。

但它会把类型传染到接口上。

一旦你把回调放进容器。

或者把回调作为成员变量。

模板就不太好用。

C++11 给了 `std::function`。

它的定位很清楚。

把各种可调用对象。

装进一个统一的盒子。

## std::function：可调用对象的“统一接口”

```cpp
#include <functional>

std::function<int(int)> f;

f = [](int x) { return x + 1; };
int y = f(41);
```

这行 `std::function<int(int)>`。

表达了两件事。

参数是什么。

返回什么。

至于你塞进去的是。

函数。

lambda。

bind 结果。

函数对象。

都行。

这就是类型擦除。

## std::bind：把参数先绑一部分

bind 的历史更老。

Boost 时代就有。

C++11 把它收编。

```cpp
using namespace std::placeholders;

auto g = std::bind(std::plus<int>{}, _1, 10);
int r = g(32);
```

这段的意思是。

把加法的第二个参数固定成 10。

留下第一个参数以后再给。

## 你需要知道的成本

`std::function` 可能分配内存。

也可能引入间接调用。

它不是免费的。

但它换来了接口稳定。

换来了可组合。

所以我对它的态度很朴素。

接口层。

用 function。

热路径。

优先用模板或直接传 lambda。

## 小洞见

很多人后来不爱用 bind。

因为 lambda 更直观。

```cpp
auto g = [](int x) { return x + 10; };
```

这比 placeholders 好读太多。

所以。

function 更像“盒子”。

bind 更像“历史遗产”。

你可以知道它。

也可以在需要时用它。

但别为了用而用。

可读性在工程里很值钱。
