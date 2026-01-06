---
title: 'future / promise / async：把并发写得更像同步'
description: 'C++11 提供 future/promise/async，把线程的“结果”和“异常”正规化地传回调用方，让并发接口更像函数调用。'
---

很多人第一次写多线程。

都会写出一种接口。

函数启动一个线程。

然后把结果塞进某个全局变量。

或者塞进某个指针参数。

再靠 condition_variable 去等。

能用。

但接口很难看。

也很难传递异常。

C++11 的 future/promise/async。

就是来把这事写得像人话。

让“异步的结果”。

像一个可以拿回来的值。

你可以先把它想成一个很朴素的场景。

你在主线程里处理请求。

但有一段工作很慢。

比如压缩。

比如解析一个大文件。

你不想卡住主线程。

你想把慢活丢出去。

自己继续做别的。

等结果真的需要的时候。

再把它拿回来。

## std::async：最省心的入口

```cpp
#include <future>

int slow() {
    return 42;
}

int main() {
    auto f = std::async(std::launch::async, slow);
    int v = f.get();
}
```

`get()` 会等待。

这点很像同步调用。

你也会看到另一个函数。

`wait()`。

它也会等待。

但它不取结果。

结果还留在 future 里。

区别是。

你可以先把任务丢出去。

继续干别的。

最后再 get。

初学者另一个常见困惑是。

“我不写 std::launch::async 行不行？”

行。

但你要知道。

默认策略可能选择“延迟执行”。

也就是说。

它不一定真的开线程。

它可能等你 `get()`/`wait()` 的时候。

才在当前线程里把函数跑完。

所以如果你的目的就是并行。

你就老老实实写上 `std::launch::async`。

少很多误会。

## future：不只是结果，还有异常

这是 future 最舒服的地方。

异步任务里抛异常。

不会悄悄消失。

它会被捕获。

在你 `get()` 的时候重新抛出。

```cpp
auto f = std::async([]() -> int {
    throw std::runtime_error("bad");
});

try {
    f.get();
} catch (const std::exception& e) {
    // 这里能拿到异常
}
```

这比“线程里崩了但主线程不知道”靠谱太多。

还有一个细节。

`get()` 只能调用一次。

因为它会把结果“取走”。

这跟“收快递”很像。

你签收了。

包裹就不在门口了。

## promise：我来手动交卷

有些场景。

你不是用 async 启动。

你是自己管理线程。

但你仍然想把结果传回去。

promise 就像一个“写答案的人”。

future 像“等成绩的人”。

```cpp
#include <thread>

std::promise<int> p;
std::future<int> f = p.get_future();

std::thread t([pp = std::move(p)]() mutable {
    pp.set_value(42);
});

t.join();
int v = f.get();
```

这里的 `std::move(p)`。

很容易让初学者困惑。

你可以简单记成一句。

promise 不能拷贝。

你要把它交给线程。

就得“转交所有权”。

这跟你把 `unique_ptr` 交出去很像。

不 move。

编译器就会拒绝你。

你也可以用 `set_exception`。

把异常也传回来。

## 小洞见

future/promise/async 最核心的价值。

不是“帮你开线程”。

而是给并发接口一个更好的形状。

结果。

异常。

等待。

都变成了类型的一部分。

你不用再发明一套回调协议。

不用再在代码里到处塞指针输出参数。

你只是在写函数。

只是这个函数的结果。

稍后才到。
