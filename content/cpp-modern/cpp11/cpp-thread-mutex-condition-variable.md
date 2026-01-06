---
title: 'std::thread / mutex / condition_variable：线程库的基础件'
description: 'C++11 把线程同步搬进标准库：thread 创建线程，mutex 保护共享状态，condition_variable 让线程“睡着等条件”。'
---

我很早就不太爱写裸线程。

不是因为线程难。

是因为线程周边那堆东西太碎。

pthread。

Windows API。

一堆平台差异。

你想写一段可移植的并发代码。

得先写一层胶水。

C++11 做了一件很工程的事。

把最基础的线程库标准化。

让你至少能在语言层面。

拥有一套共同的工具。

你可以先把它理解成三件事。

你把活丢到别的线程去跑。

你得保护共享的数据别被乱改。

你还得让线程在“没活干”的时候真的睡着。

不然你就会写出那种 `while(true)` 的傻等。

我们用一个很常见的场景把它串起来。

你有一个后台 worker。

主线程往队列里塞任务。

worker 线程等任务。

有就干。

没就睡。

## std::thread：先把线程开起来

```cpp
#include <thread>

void work() {
    // ...
}

int main() {
    std::thread t(work);
    t.join();
}
```

初学者最容易误会的一点是。

“我启动了线程。”

“那它就会在后台自己好好结束。”

线程确实会结束。

但 `std::thread` 这个对象。

会要求你做一次明确的收尾。

`join()` 很重要。

你不 join。

线程对象析构时会 `std::terminate()`。

这不是标准库凶。

这是在逼你负责。

你要么 join。

要么 detach。

别装作这事不存在。

`detach()` 不是“更高级”。

它更像是。

“我不等你了。”

但你也得保证。

线程里用到的引用。

指针。

对象生命周期。

都不会在主线程结束后突然失效。

初学阶段。

我建议你把 `join()` 当默认。

少走很多弯路。

## mutex：共享状态要么锁，要么别共享

最经典的共享状态。

就是一个容器。

多个线程 push。

一个线程读。

你如果不锁。

那不是“偶尔错”。

那是 data race。

很多人第一次听 data race。

会以为它是“偶尔读到旧值”。

其实更糟。

它属于未定义行为的那一类。

也就是说。

你不能指望它“只是错一点”。

你得先把它消掉。

最直观的反例是。

```cpp
int counter = 0;

void hit() {
    ++counter;
}
```

单线程里很好。

多线程里。

`++counter` 不是一个原子动作。

你很快就会得到一个奇怪的结果。

```cpp
#include <mutex>

std::mutex m;
int counter = 0;

void hit() {
    std::lock_guard<std::mutex> g(m);
    ++counter;
}
```

`lock_guard` 是 RAII。

它的好处很朴素。

你不会忘记 unlock。

也不会被异常打脸。

你可以先把它当成一条很朴素的规矩。

共享变量。

只要能被多个线程同时摸到。

就要么用锁保护。

要么别共享。

## condition_variable：别用 while(true) 去等

并发里另一种很常见的需求。

生产者消费者。

队列为空时。

消费者应该睡。

有任务时。

消费者被叫醒。

C++11 给了 condition_variable。

```cpp
#include <condition_variable>
#include <queue>

std::mutex m;
std::condition_variable cv;
std::queue<int> q;

void producer() {
    {
        std::lock_guard<std::mutex> g(m);
        q.push(1);
    }
    cv.notify_one();
}

int consume() {
    std::unique_lock<std::mutex> lk(m);
    cv.wait(lk, [] { return !q.empty(); });
    int v = q.front();
    q.pop();
    return v;
}
```

这段代码里。

有两个细节。

初学者经常会问。

“为什么要用花括号？”

“为什么 notify 要放在锁外面？”

花括号的目的很简单。

让 `lock_guard` 尽快析构。

也就是尽快释放锁。

然后再去 `notify_one()`。

这样被唤醒的线程。

更有机会马上拿到锁。

少一次无意义的抢锁。

你注意到。

wait 用的是 `unique_lock`。

不是 `lock_guard`。

因为 condition_variable 需要在 wait 时临时释放锁。

醒来后再重新加锁。

## 为什么 wait 要配谓词

因为“虚假唤醒”。

这不是 bug。

这是现实。

你永远应该用 while 条件去保护。

而 `wait(lk, pred)` 就是标准库帮你写好 while。

这样写你就不容易漏。

初学者还有一个常见坑。

把 `cv` 当成“队列本身”。

其实不是。

队列还是队列。

mutex 还是 mutex。

cv 只是一个“叫醒铃”。

真正决定能不能干活的。

永远是那个条件。

也就是 `!q.empty()`。

## 小洞见

线程库这些基础件。

看起来普通。

但它把并发编程的“规矩”写进了 API。

join 或 detach。

锁用 RAII。

wait 要配条件。

这些都是血换来的经验。

C++11 把它们收编进标准库。

让你写并发的时候。

更像写 C++。

不是像写平台手册。
