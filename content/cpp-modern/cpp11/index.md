---
title: 'C++11 新特性'
description: '现代 C++ 的起点：auto、lambda、移动语义、智能指针等核心特性'
---

C++11 这东西。

我一直觉得它不像“升级”。

更像一次集体还债。

在它之前。

你写 C++，经常要靠一堆祖传写法。

宏、手写类型、各种“我知道我在干嘛”。

在它之后。

语言终于肯给你一些正经工具。

这个子专栏。

就从这些工具讲起。

讲它们怎么来的。

也讲它们在工程里怎么救命。

## 文章列表

### 语言核心（写起来像 C++，而不是 C with classes）

[x] **[= default 和 = delete](./cpp-default-delete)** — 驯服编译器的"小脾气"

[x] **[enum class](./cpp-enum-class)** — 更安全的枚举类型

[x] **[auto 关键字](./cpp-auto)** — 类型推导的艺术

[x] **[decltype 与尾置返回类型](./cpp-decltype-trailing-return)** — “让类型从表达式里自己冒出来”

[x] **[nullptr](./cpp-nullptr)** — 告别 NULL 的歧义

[x] **[统一初始化与 std::initializer_list](./cpp-uniform-initialization)** — 花括号到底在干嘛

[x] **[范围 for 循环](./cpp-range-for)** — 更简洁的遍历方式

[x] **[constexpr 与常量表达式](./cpp-constexpr)** — 把“能算的”提前算掉

[x] **[static_assert](./cpp-static-assert)** — 把断言搬到编译期

[x] **[noexcept](./cpp-noexcept)** — 异常规格说明的现代写法

[x] **[override / final](./cpp-override-final)** — 虚函数覆写别再靠肉眼

[x] **[委托构造与继承构造](./cpp-delegating-inheriting-ctors)** — 构造函数不再写到手抽筋

[x] **[显式转换运算符](./cpp-explicit-conversion-operator)** — 让“能不能转”说清楚

[x] **[alignas / alignof](./cpp-alignas-alignof)** — 对齐这事别再靠猜

[x] **[thread_local](./cpp-thread-local)** — 每个线程一份的“全局变量”

[x] **[原始字符串与 Unicode 字面量](./cpp-raw-string-unicode-literals)** — 写正则和路径别再反斜杠地狱

[x] **[用户自定义字面量](./cpp-user-defined-literals)** — 给单位和领域值一个“类型入口”

### Lambda 与移动语义（现代 C++ 的生产力核心）

[x] **[lambda 表达式](./cpp-lambda)** — 匿名函数与闭包

[x] **[移动语义与右值引用](./cpp-move-semantics)** — 告别不必要的拷贝

[x] **[完美转发与 std::forward](./cpp-perfect-forwarding)** — 把参数“原样”传下去

### 模板与类型系统（库作者的武器库）

[x] **[可变参数模板（variadic templates）](./cpp-variadic-templates)** — 写一次，吃掉任意参数

[x] **[using 类型别名与别名模板](./cpp-using-alias)** — typedef 的升级版

[x] **[type traits（<type_traits>）](./cpp-type-traits)** — 用编译期信息写更聪明的模板

### 并发与内存模型（不是加线程就完事）

[x] **[C++11 内存模型与 atomic](./cpp-memory-model-atomic)** — 你以为的“顺序”，可能不存在

[x] **[std::thread / mutex / condition_variable](./cpp-thread-mutex-condition-variable)** — 线程库的基础件

[x] **[future / promise / async](./cpp-future-promise-async)** — 把并发写得更像同步

### 标准库组件（能少造轮子就少造）

[x] **[std::array](./cpp-std-array)** — 固定长度容器：把“长度”塞进类型里

[x] **[智能指针](./cpp-smart-pointer)** — unique_ptr、shared_ptr、weak_ptr

[x] **[std::chrono](./cpp-chrono)** — 时间与计时

[x] **[unordered 容器](./cpp-unordered-containers)** — unordered_map / unordered_set

[x] **[std::tuple](./cpp-tuple)** — 多返回值与结构化打包（无宏版）

[x] **[std::function / std::bind](./cpp-function-bind)** — 可调用对象的统一接口

[x] **[std::regex](./cpp-regex)** — 正则表达式（以及它的坑）

[x] **[随机数库 <random>](./cpp-random)** — 别再用 rand()
