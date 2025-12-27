---
title: "C++11 新特性"
description: "现代 C++ 的起点：auto、lambda、移动语义、智能指针等核心特性"
---

C++11 是现代 C++ 的分水岭，带来了大量革命性的新特性。这个子专栏将深入讲解 C++11 的核心内容。

## 文章列表

### 语言核心（写起来像 C++，而不是 C with classes）

- [x] **[= default 和 = delete](./cpp-default-delete)** — 驯服编译器的"小脾气"
- [x] **[enum class](./cpp-enum-class)** — 更安全的枚举类型
- [x] **[auto 关键字](./cpp-auto)** — 类型推导的艺术
- [ ] **decltype 与尾置返回类型** — “让类型从表达式里自己冒出来”
- [x] **[nullptr](./cpp-nullptr)** — 告别 NULL 的歧义
- [x] **[统一初始化与 std::initializer_list](./cpp-uniform-initialization)** — 花括号到底在干嘛
- [x] **[范围 for 循环](./cpp-range-for)** — 更简洁的遍历方式
- [ ] **constexpr 与常量表达式** — 把“能算的”提前算掉
- [ ] **static_assert** — 把断言搬到编译期
- [ ] **noexcept** — 异常规格说明的现代写法
- [ ] **override / final** — 虚函数覆写别再靠肉眼
- [ ] **委托构造与继承构造** — 构造函数不再写到手抽筋
- [ ] **显式转换运算符** — 让“能不能转”说清楚
- [ ] **alignas / alignof** — 对齐这事别再靠猜
- [ ] **thread_local** — 每个线程一份的“全局变量”
- [ ] **原始字符串与 Unicode 字面量** — 写正则和路径别再反斜杠地狱
- [ ] **用户自定义字面量** — 给单位和领域值一个“类型入口”

### Lambda 与移动语义（现代 C++ 的生产力核心）

- [x] **[lambda 表达式](./cpp-lambda)** — 匿名函数与闭包
- [ ] **移动语义与右值引用** — 告别不必要的拷贝
- [ ] **完美转发与 std::forward** — 把参数“原样”传下去

### 模板与类型系统（库作者的武器库）

- [ ] **可变参数模板（variadic templates）** — 写一次，吃掉任意参数
- [ ] **using 类型别名与别名模板** — typedef 的升级版
- [ ] **type traits（<type_traits>）** — 用编译期信息写更聪明的模板

### 并发与内存模型（不是加线程就完事）

- [ ] **C++11 内存模型与 atomic** — 你以为的“顺序”，可能不存在
- [ ] **std::thread / mutex / condition_variable** — 线程库的基础件
- [ ] **future / promise / async** — 把并发写得更像同步

### 标准库组件（能少造轮子就少造）

- [x] **[std::array](./cpp-std-array)** — 固定长度容器：把“长度”塞进类型里
- [ ] **智能指针** — unique_ptr、shared_ptr、weak_ptr
- [ ] **std::chrono** — 时间与计时
- [ ] **unordered 容器** — unordered_map / unordered_set
- [ ] **std::tuple** — 多返回值与结构化打包（无宏版）
- [ ] **std::function / std::bind** — 可调用对象的统一接口
- [ ] **std::regex** — 正则表达式（以及它的坑）
- [ ] **随机数库 <random>** — 别再用 rand()
