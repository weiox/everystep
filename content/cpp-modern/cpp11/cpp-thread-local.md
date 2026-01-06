---
title: 'thread_local：每个线程一份的“全局变量”'
description: 'C++11 把线程局部存储变成标准能力：thread_local 让每个线程拥有自己的那份状态，不再手写 pthread_key 或编译器扩展。'
---

写并发久了。

你会遇到一种很尴尬的需求。

你需要“全局”。

但你又不希望它真的是全局。

因为它在多线程下会打架。

典型例子是。

统计计数。

缓存。

错误码。

还有某些老库里那种“每线程上下文”。

C++11 之前。

你当然能做。

你可以用 pthread 的 TLS。

可以用 Windows 的 TLS API。

也可以用编译器扩展 `__thread`。

但你很难写出一份跨平台、读起来像人话的代码。

C++11 给了 `thread_local`。

这东西的气质很像 `static`。

但它的作用域是线程。

不是进程。

## 最直观的用法

```cpp
thread_local int counter = 0;

void hit() {
    ++counter;
}
```

每个线程都会有自己的 `counter`。

线程 A 递增的是 A 的那份。

线程 B 递增的是 B 的那份。

它们互不干扰。

也不需要锁。

你不再需要写一套键值表。

不再需要把 void* 塞来塞去。

## 一个更像工程的场景

日志里经常需要“线程 id”。

或者“本线程的请求 id”。

你不想每个函数都把它当参数传一遍。

传到最后。

参数表比业务逻辑还长。

thread_local 能让你把这类状态。

放在“每线程一份”的位置。

```cpp
thread_local std::string request_id;

void set_request_id(std::string id) {
    request_id = std::move(id);
}

const std::string& get_request_id() {
    return request_id;
}
```

你当然要克制。

因为任何“隐藏状态”都有代价。

但当你明确知道这是线程上下文。

thread_local 比全局变量靠谱太多。

## 你需要知道的一点现实

thread_local 不是免费的。

尤其是涉及动态初始化和析构时。

每个线程会各走一遍。

线程退出时也要做清理。

所以我习惯把 thread_local 用在两类地方。

一种是小而纯的状态。

比如计数器。

一种是确实属于线程上下文的东西。

比如请求 id。

然后我会尽量让它“看得见”。

别把它藏成全局魔法。

C++11 的 thread_local。

说到底就是。

把一件并发编程里很常见、很实用的需求。

从平台 API 里拉出来。

变成语言的基本能力。

你写起来更像 C++。

不是像系统编程说明书。
