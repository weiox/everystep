---
title: "用纯 C++ 手写一个迷你信号/槽系统"
description: "从回调到观察者模式，一步步手写一个 MVP 版的信号/槽系统，搞清楚 Qt 这套机制到底在做什么。"
---

在用 Qt 写界面的那些年，我最常敲的一行代码，大概就是这句：

```cpp
connect(button, &QPushButton::clicked,
        this,   &MainWindow::onButtonClicked);
```

这行谁都会写，IDE 还能帮你补全。可要真问一句：

- `clicked` 到底是个啥？
- 为什么可以连到各种“长得像函数”的东西上？
- Qt 在 `connect` 里到底记了些什么？

很多人（包括当年的我）心里只有一个模糊印象：

> “大概是 MOC 搞了点宏魔法，反正能用就行了。”

有一次我在老项目里排 Bug：一个按钮点一下，后台日志里同一条消息刷三遍。最后排查半天，发现同一个按钮在三个地方都被 `connect` 了同一个槽。

那天之后，我下了个决心：

> 别老拿“黑魔法”当挡箭牌，咱就用最朴素的 C with class 思路，一步步把这套东西**自己写一遍**。

这篇文章就是把这条路从头走一遍：

- 先从最土的 C 风格函数指针开始，只支持一个回调；
- 再演化成“一个按钮可以通知一群听众”；
- 然后解决“回调里还想带点自己的私货”的问题；
- 最后，把这些东西收拾成一个 `Signal` 类，和 Qt 的 `connect(...)` 一行一行对上号。

读完之后，你再看那句：

```cpp
connect(button, &QPushButton::clicked,
        this,   &MainWindow::onButtonClicked);
```

脑子里就不再是“魔法”，而是很清楚的一条链路：谁在保存谁，什么时候被谁调用。

### 一个小预览：终局长什么样

先别急着钻细节，先看一眼我们最终要造出的那套东西，在你脑子里立个“终点站”。

它的大致流程可以画成这样一张小图：

```text
[ Button::click() ]       // 事件发生
        │
        ▼
 [ Signal::emit() ]       // 依次调用所有槽
        │
        ├── slot1()
        ├── slot2()
        └── slot3()
```

对应到代码层面，大概就是下面这个味道：

```cpp
class Signal {
public:
    using Slot = std::function<void()>;

    void connect(Slot slot) { slots_.push_back(std::move(slot)); }
    void emit() { for (auto& slot : slots_) slot(); }
    void operator()() { emit(); }

private:
    std::vector<Slot> slots_;
};

class Button {
public:
    Signal clicked;     // 对外暴露一个信号
    void click() { clicked(); }  // 事件发生时发信号
};

class Window {
public:
    void onButtonClicked() { /* ... */ }
};

int main() {
    Button btn;
    Window w;

    // 把成员函数包装成一个“槽”，连到 clicked 上
    btn.clicked.connect([&w] { w.onButtonClicked(); });

    btn.click();  // 触发：从 Button::click -> Signal::emit -> Window::onButtonClicked
}
```

后面几节要做的事情，就是把这个终局一步步“拆开重建”：

- 先用最原始的 C 回调，搭出一个只有一个回调的按钮；
- 再撑成“可以广播给一串回调”；
- 然后学会给回调“带点私货”；
- 最后抽象成 `Signal` 类，再和 Qt 的 `connect(...)` 一一对上号。

### 1. 我们先不谈 Qt：最土的 C 风格回调

**本节先搭一个“最小版”：一个按钮，只能挂一个回调。**

先把 Qt 忘掉，当年写 C 程序，如果想在“按钮被点一下”的时候顺手干点别的事，大家一般怎么玩？

最常见的就是：**把一个函数指针塞给按钮**。

```cpp
#include <iostream>

void on_clicked() {
    std::cout << "button clicked\n";
}

class Button {
public:
    void setCallback(void (*cb)()) { callback_ = cb; }

    void click() {
        if (callback_) {
            callback_();
        }
    }

private:
    void (*callback_)() = nullptr;
};

int main() {
    Button btn;
    btn.setCallback(&on_clicked);

    btn.click(); // 模拟用户点了一下按钮
}
```

如果你是 C with class 水平，这段代码应该完全没有压力：

- `setCallback` 接受一个函数指针，就像 C 里传回调一样；
- `click()` 里如果 `callback_` 不为空，就把它调一下。

这里，其实已经有了“信号/槽”的影子：

- `click()` 相当于“发信号”；
- `setCallback` 里传进来的函数，就是“槽”。

问题也很明显：

- **一个按钮只能有一个回调**，后面的人再调 `setCallback`，前面的就被覆盖了；
- 回调的签名被写死成 `void()`，你要带点上下文信息，只能靠全局变量、单例这类土办法。

这就是“信号/槽”故事的出发点：

> 一个对象想在自己身上发生某个事件时，通知**不止一个人**，
> 通知的时候，还想带点上下文信息。

到目前为止，我们手里有的是：

- 一个按钮；
- 它只能挂一个 `void()` 回调；
- 点一下，就只会叫这一位听众。

下一步，就轮到“**一个回调不够用**”这个需求登场了。

接下来，我们一点一点把这个需求揉成代码。

### 2. 一个回调不够用：先撑成“一串回调”

**这一节要解决的事很简单：一个按钮只能有一个回调，太寒酸了。**
我们至少得让它能通知一群人。

最直接的想法就是：

> 函数指针从“一个”变成“一个数组”。

用 C++ 标准库的话，就是一个 `std::vector`：

```cpp
#include <vector>

class Button {
public:
    void addCallback(void (*cb)()) {
        callbacks_.push_back(cb);
    }

    void click() {
        for (auto cb : callbacks_) {
            if (cb) {
                cb();
            }
        }
    }

private:
    std::vector<void (*)()> callbacks_;
};
```

现在按钮就从“只能通知一个人”，升级成了“广播站”：谁对这个按钮感兴趣，就往 `callbacks_` 里塞一个函数指针，点一下就**挨个叫一遍**。

如果你翻设计模式的书，这套东西有个正式名字：**观察者模式（Observer）**。

- 按钮是“被观察者”（Subject）；
- 回调是“观察者”（Observer）；
- 事件发生时，Subject 把所有 Observer 叫一遍。

到这一步，其实已经很接近 Qt 的信号/槽了，只不过还比较“原始”：

- 形态上，我们已经有了“一个事件，对一串回调”的雏形；
- 但能力上，还只能塞 `void (*)()` 这种**不带任何上下文**的裸函数指针。

下一节，我们就来解决“**回调也想带点私事**”这个现实需求。

### 3. 回调也想带点“私事”：从函数指针到函数对象

**这一节要解决的问题是：回调想带点自己的“小心事”。**

上一节我们解决了“一个按钮可以通知很多人”，但每个回调长得仍然是这样：

```cpp
void some_callback();
```

它什么都不知道：

- 不知道是哪个窗口上的按钮点的；
- 也访问不到某个窗口里的成员变量。

而你在 Qt 代码里，真正想挂上去的，其实更像是这样一对东西：

- “**哪个对象**”：`this`（假设它是一个 `Window*`）；
- “**调哪个成员函数**”：`&Window::onButtonClicked`。

也就是说，我们真的想保存的是：

```text
( Window* this, &Window::onButtonClicked )
```

以后按钮被点一下，就去把这对东西拿出来，用它来调用那个窗口的成员函数。

如果还停留在纯 C 思路，你大概会这么干：

- 把 `this` 塞进一个 `void* userdata`；
- 写一个形如 `void func(void* userdata)` 的回调函数；
- 每次触发时手动把 `userdata` 传给它。

代码大概是这个味道：

```cpp
struct Callback {
    void* userdata;               // 想带点什么上下文，就塞在这里
    void (*func)(void* userdata); // 真正干活的函数，记得把 userdata 传进去
};

void on_button_clicked_c_style(void* userdata) {
    auto* w = static_cast<Window*>(userdata);
    w->onButtonClicked();
}

// somewhere:
Callback cb;
cb.userdata = &window;              // 这里其实就是把 this 塞进去了
cb.func     = &on_button_clicked_c_style;

// 触发时这么调：
cb.func(cb.userdata);
```

你可以把这对 `(func, userdata)` 看成是一个 C 风格的“小盒子”，
里面装的其实就是我们刚才说的那对东西：`(Window* this, &Window::onButtonClicked)`。

这种写法“能跑”，但有两个典型的槽点：

- 所有上下文都被压成了 `void*`，类型信息全丢了；
- 一旦哪里少传 / 传错了 `userdata`，编译器也帮不上忙，全靠人记忆。

后来 C++ 社区给了一个更干净的思路：

> 不要只传“函数指针 + void*”，
> 传一个**能调用的对象**——这个对象里，既能记住“要调哪个函数”，
> 也能记住“我要带的那点私货（this 指针、额外状态）”。

所谓“能调用的对象”，其实就是：

- 一个普通的 C++ 类；
- 只不过多写了一个 `operator()`，所以你可以像调函数一样调它。

我们先不用 `std::function` 这个词，先手写一个针对 `Window` 的“函数对象”看看：

```cpp
class WindowSlot {
public:
    explicit WindowSlot(Window* w)
        : w_(w) {}

    void operator()() const {
        w_->onButtonClicked();
    }

private:
    Window* w_;
};

int main() {
    Window w;
    WindowSlot slot(&w);  // 这里把 this 指针塞进去了

    slot();               // 实际等价于：slot.operator()();
}
```

`WindowSlot` 只是一个非常朴素的类：

- 构造函数里记住了“我要服务哪一个 `Window`”；
- `operator()` 里干的事就是调用它的 `onButtonClicked()`。

关键是最后那一行：

- 写 `slot();` 的时候，看起来像在“调用一个函数”；
- 实际上编译器会把它翻译成 `slot.operator()();`，
  所以**这个对象本身就成了一个“带着自己上下文的回调”**。

到这里，你可以在脑子里对一下：

- C 版本是 `(void (*func)(void*), void* userdata)` 这对东西；
- C++ 版本是 `WindowSlot` 这个小类（里面自己记着 `Window*`，`operator()` 里负责真正的调用）。

从“能不能被 `()` 调”的角度看，它们本质上是一回事。

问题在于，如果我们继续沿用上一节的按钮实现：

```cpp
class Button {
public:
    void addCallback(void (*cb)());
    // ...
};

WindowSlot slot(&w);
btn.addCallback(slot); // 这里当然是编译不过的
```

因为 `addCallback` 的参数类型还是 `void (*)()`，它只认“裸函数指针”，
并不认识这种“带状态的可调用对象”。

这时候就轮到 `std::function` 上场了：

> `std::function<void()>` 可以把
> - 普通函数、
> - 成员函数配上对象（通过小包装）、
> - lambda、
> - 自己写的函数对象（比如上面的 `WindowSlot`）
>
> 统统装进同一个“盒子”里，用统一的语法 `slot();` 调用。

你可以把 `std::function<void()>` 想象成一个“万能回调盒子”：

- 只要某个东西能写成 `东西();` 的形式调起来，
- 就可以塞进这个盒子里存着，
- 以后我们只管对着这个盒子调用 `slot();`，不用管里面到底是函数指针、lambda，还是哪个小类的 `operator()`。

有了这个“盒子”，我们就可以把 `Button` 稍微改造一下：

```cpp
#include <functional>
#include <vector>

class Button {
public:
    using Slot = std::function<void()>;

    void addSlot(Slot slot) {
        slots_.push_back(std::move(slot));
    }

    void click() {
        for (auto& slot : slots_) {
            slot();
        }
    }

private:
    std::vector<Slot> slots_;
};
```

现在你就可以这么用：

```cpp
class Window {
public:
    void onButtonClicked() {
        std::cout << "Window::onButtonClicked\n";
    }
};

int main() {
    Button btn;
    Window w;

    // 1. 普通函数
    btn.addSlot([] { std::cout << "free slot\n"; });

    // 2. 成员函数 + lambda 打个包
    btn.addSlot([&w] { w.onButtonClicked(); });

    btn.click();
}
```

你可以把 `Slot` 理解成一个“万能回调盒子”：

- 这个盒子里可以塞各种“长得像函数的东西”；
- 我们只要记住一个动作：`slot();`。

这一节走完，我们已经从“只能塞函数指针”，进化到了“可以塞任意可调用体，还能优雅地带上状态”。

这时再回头看 Qt 的那句 `&MainWindow::onButtonClicked`，你就可以在脑子里想象：

> Qt 在内部其实也在干类似的打包动作，
> 只是我们这里用 lambda 手写了一遍。

### 4. 把它独立出来：写一个最小可用版 `Signal`

上面这些代码，其实已经把“信号/槽”的核心都凑齐了，只是还散落在 `Button` 里面。

下一步，我们做一件很 C with class 的事：

> 把“维护一串回调 + 依次调用”的逻辑，
> 从 `Button` 里**抽出来**，包装成一个小类。

```cpp
class Signal {
public:
    using Slot = std::function<void()>;

    // 连接一个槽
    void connect(Slot slot) {
        slots_.push_back(std::move(slot));
    }

    // 发射信号
    void emit() {
        for (auto& slot : slots_) {
            slot();
        }
    }

    // 小语法糖：可以写成 signal();
    void operator()() { emit(); }

private:
    std::vector<Slot> slots_;
};
```

这个 `Signal` 就是我们这篇文章要到达的 MVP：

- 里面有一串 `Slot`；
- `connect` 往这串里塞东西；
- `emit` / `operator()` 把它们一个个调一遍。

你可以先把 Qt 忘掉，把 `Signal` 当成一个“高级一点的函数数组”：

- 能接各种形式的回调（自由函数、lambda、带状态的函数对象）；
- 以后想加点日志、统计、线程切换，也只要在 `emit` 这一处动手脚。

### 5. 再塞回按钮里：`btn.clicked.connect(...)`

现在我们有了一个独立的 `Signal` 类型，可以回过头再给按钮“装配件”：

```cpp
class Button {
public:
    Signal clicked;  // 对外暴露一个信号

    void click() {
        // 这里可以有别的逻辑，比如更新 UI 状态
        clicked.emit();
        // 或者直接：clicked();
    }
};

class Window {
public:
    void onButtonClicked() {
        std::cout << "Window::onButtonClicked\n";
    }
};

int main() {
    Button btn;
    Window w;

    // 把成员函数通过 lambda 包一层，接到信号上
    btn.clicked.connect([&w] { w.onButtonClicked(); });

    btn.click();  // 间接触发 onButtonClicked
}
```

如果你习惯用 Qt 的语气来读代码，可以在脑子里这么翻译：

- `btn.clicked`：相当于 Qt 里 `&Button::clicked` 这个信号；
- `.connect(...)`：就是 `QObject::connect` 做的事，只不过我们这里是成员函数形式；
- `lambda` 里那句 `w.onButtonClicked();`，就是你熟悉的槽。

到这里，你已经用标准 C++ + 标准库，拼出了一个**最小可用版**的信号/槽系统：

- 支持多个槽；
- 槽可以是自由函数、lambda、带上下文的成员函数；
- 调用关系一眼就能顺下来。

接下来，就可以把这套东西和 Qt 那句 `connect(...)` 正式对上号了。

### 6. 和 Qt 的 `connect(...)` 一行一行对上号

先看我们自己的写法：

```cpp
btn.clicked.connect([&w] { w.onButtonClicked(); });
```

再把 Qt 那句写在旁边：

```cpp
connect(button, &QPushButton::clicked,
        &w,      &Window::onButtonClicked);
```

如果你已经跟着前面的演化过程走了一遍，现在就可以很平静地把它拆开看：

- `button`：谁是发信号的人？就是我们的 `btn`；
- `&QPushButton::clicked`：发的是什么信号？对应我们类里那个 `Signal clicked;` 成员；
- `&w`：谁来听？对应我们刚才 lambda 里捕获的对象；
- `&Window::onButtonClicked`：听到信号后要调哪个成员函数？

#### 6.1 直白版心智模型：把 Qt 当成“帮你做 connect 的助手”

先抛开成员函数指针这些语法糖，想象有这么一个极简版的 `connect`：

```cpp
// 直白版：已经帮你把槽打包成 std::function 了
bool connect(Button& sender,
             Signal& signal,
             std::function<void()> slot) {
    signal.connect(std::move(slot));
    return true;
}

// 调用方式大概是：
connect(btn, btn.clicked, [&w] { w.onButtonClicked(); });
```

你可以把真正的 Qt `connect(...)`，先粗暴想象成就是在做这件事：

- 第一个参数：谁在发信号（`sender`）；
- 第二个参数：发的是什么信号（`signal`）；
- 第 3 / 4 个参数：Qt 帮你把“对象 + 成员函数”打包成一个 `std::function` 风格的槽，塞进那条回调链里。

这样看，`connect(button, &QPushButton::clicked, &w, &Window::onButtonClicked)`
和我们手写的 `btn.clicked.connect(...)`，就成了两种**语法不同、骨架相同**的写法而已。

#### 6.2 进阶版：成员指针视角（可以先跳过）

如果你对“成员指针语法”比较熟，可以进一步用一个更贴近 C++ 的伪实现来想象 Qt 内部做的事：（这一小段可以先跳过，不影响理解整体机制）

```cpp
bool connect(Button* sender, Signal Button::*signal,
             Window* receiver, void (Window::*method)())
{
    // 1. 从 sender 里拿到那个 Signal
    Signal& sig = sender->*signal; // 成员指针语法

    // 2. 把 (receiver + method) 打包成一个 Slot
    sig.connect([receiver, method] {
        (receiver->*method)();
    });

    return true;
}
```

如果你能读懂这段伪代码，其实就已经把 Qt 信号/槽的**心脏**看清楚了：

> Qt 在 `connect(...)` 里，就是想办法把“对象 + 成员函数”打包成一个 `Slot`，
> 然后塞进某个 `Signal` 维护的那条回调链里。

真正的 Qt 当然要复杂得多，它要考虑：

- 不同线程之间怎么发信号（排队还是直接调）；
- 槽对应的对象析构了怎么办（自动断开）；
- 参数类型怎么检查、怎么做反射调用；
- QML、Designer 这些工具怎么利用这些信息。

但这些“工程加强项”，都建在你刚才写出来的这套小骨架上。

### 7. 回头看一眼：信号/槽其实没那么玄

现在，我们可以用一句话把整件事收个尾：

> **信号就是一个带着“回调列表”的对象；
> 槽就是被放进列表里的各种可调用体；
> `connect` 就是往列表里塞东西；
> `emit`（或者 `signal()`）就是把列表从头到尾扫一遍。**

这篇文章我们刻意只做了一个 MVP：

- 单线程、本进程内；
- 只演示 `void()` 这种不带参数的信号；
- 不管自动断连、不管调试工具、不管元对象系统。

但这几十行代码有一个很重要的作用：

> 把“信号/槽”这三个字，从一个看起来很高大上的概念，
> 变回“就是一条回调链”这么朴素的画面。

等你哪天又遇到“按钮连了三次槽，日志刷三遍”这种小 Bug，把这篇文章翻出来，顺着那条 `std::vector<Slot>` 想一想，大概就会会心一笑：

> Qt 也没有那么多妖魔鬼怪，
> 只是在我们刚才这套小轮子的基础上，
> 又很认真的加了一圈安全带和护栏而已。

