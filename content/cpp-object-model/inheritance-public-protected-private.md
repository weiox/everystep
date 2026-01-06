---
title: "public / protected / private 继承：不只是访问权限"
description: "从 C with Classes 的复用冲动讲起：三种继承方式到底改变了什么，以及它们如何影响 is-a、接口暴露与可替换性。"
---

### 先别急着谈“访问权限”

很久以前。

大家写 C。

想复用。

只能复制粘贴。

或者写一堆宏。

也有人开始玩“结构体套结构体”。

看起来像继承。

但谁都知道。

这东西全靠自觉。

自觉这种东西。

最不可靠。

### C with Classes 出现以后，新的坑也出现了

后来有了 C with Classes。

你终于可以写 `class`。

可以把数据和函数绑在一起。

也可以让一个类型“长得像另一个类型”。

背景是 80 年代的贝尔实验室。

代码开始变大。

大到“靠所有人自觉”这件事，已经有点像赌运气。

而 Bjarne 想做的也很朴素。

加一点抽象。

但别把 C 的性能和工具链都掀了。

他从 Simula 那边借了很多想法。

但落地时非常克制。

这时候麻烦来了。

“像”到底是给谁看的？

给用户看？

还是只给你自己实现时偷个懒？

### 一段代码，把坑挖出来

你想写一个栈。

你看到一个现成的动态数组，心里想着“正好复用一下”。

手一痒，就这么写了。

```cpp
struct Vector {
    void push_back(int);
    void insert(int pos, int);
    int back() const;
    void pop_back();
};

struct Stack : public Vector {
    void push(int x) { push_back(x); }
    int pop() {
        int v = back();
        pop_back();
        return v;
    }
};
```

代码能过。

但你已经把 `insert` 也“赠送”给了栈的使用者。

更糟的是：这会直接破坏栈最核心的承诺。

```cpp
Stack s;
s.push(1);
s.push(2);
s.insert(0, 999);
int x = s.pop();
```

如果这是一个真正的栈，你会希望 `x` 是 2。

但现在它可能变成 999。

这不是“多了几个函数”。

这是你在对外签合同。

### public 继承：我承认“它就是一种 Base”

`public` 继承最像大家直觉里的“继承”。

你说的是：`Derived` 可以当成 `Base` 用。

于是编译器会帮你做一件很关键的事：允许“把派生类当基类传进去”。

```cpp
void use_vector(const Vector&);

Stack s;
use_vector(s);
```

这句能成立，不是因为编译器心情好。
是因为你用 `public` 继承告诉它：这次“向上转型”是合法的。

向上转型这词听着吓人。
其实就是：我手里拿着 `Stack`，但我现在只把它当 `Vector` 用。

这句承诺很强。

强到你后来会在代码评审里反复被它拷打。

因为从这一刻起，`Vector` 的所有公开接口，都变成了 `Stack` 的公开接口。

你想撤回？

就得升级版本。

就得改用户代码。

### 一个小洞见

很多“继承用错了”的问题。
根源不是语法。
是你根本没意识到自己在签合同。

### private 继承：我只是借你的实现

`private` 继承像是在说。
“我内部用一下你的零件。”
“但别把我当成你。”

你改成这样。

```cpp
struct Stack : private Vector {
    void push(int x) { push_back(x); }
};
```

外部现在看不到 `push_back` 和 `insert` 了。

而且更重要的是。

`Stack` 也不能再被当成 `Vector` 传来传去。

```cpp
void use_vector(const Vector&);

Stack s;
use_vector(s); // 不行
```

这就是“不是访问权限”那一半。
它改变的是类型关系。
你不再承诺“我能当成 Base 用”。
你只是在复用实现。

如果你只记一句。

就记这个。

`public` 像是在说“我是”。

`private` 像是在说“我用过”。

### 那为什么不直接用组合？

很多时候。
你确实应该用组合。

`Stack` 里面放一个 `Vector` 成员。

更直白。

也更少坑。

但 `private` 继承有一个老派理由。

你有时想复用 `Base` 的实现。

还想重写它的虚函数。

或者想用到它的 `protected` 接口。

这时候“成员变量”就不够了。

所以它不是禁术。
只是要更克制。

还有一个更“黑客”的老理由：空基类优化（EBO）。

名字听起来很硬。
意思却很朴素：基类如果是空的，很多实现会把它“挤扁”，让它不占对象大小。

```cpp
struct Tag {};
struct X : private Tag { int v; };
```

`Tag` 更像一个“类型标签”。
很多实现下，`sizeof(X)` 仍然等于 `sizeof(int)`。

### protected 继承：对外不说，对子类说

`protected` 继承处在中间。

它说的是：

对外。

我不是 `Base`。

但对我的子类。

我希望它们还能继续把 `Base` 当作基座。

一个典型画面是。

你在写一套框架。

你不想让用户拿着你的“中间层类型”乱玩。

但你希望框架内部还能在这个层次上继续扩展。

这时候 `protected` 继承就会出现。

它比较少见。

但你在老代码里会遇到。

它最容易让人迷糊的一点是：它不仅“挡住了对外的向上转型”，还会把基类的 `public` 成员，降级成 `protected`。

```cpp
struct Base {
    void f();
};

struct Mid : protected Base {
    void g() { f(); }
};

Mid m;
m.f();
```

`Mid` 自己能在内部调用 `f()`。
但外部不能。

这通常是在说：我允许“继承链往下的同学”继续用这个基座，但我不想把这个基座交到用户手里。

### protected 成员：给子类留门，也容易漏风

`protected` 这个词本身也常被误会。

它不是“更安全的 private”。

它更像“对子类开放”。

```cpp
struct Base {
protected:
    int n = 0;
};

struct Derived : public Base {
    void bump() { ++n; }
};
```

`Derived` 能改 `n`。

外部不能。

听起来挺合理。

但坑也在这里。

一旦你把数据做成 `protected`。

你就把“类不变式”的一半控制权交给了所有子类。

类不变式这词也别怕。
它就是你希望这个类永远成立的规则。
比如“`n` 永远不为负”。

子类一多。

你就很难再改动 `Base` 的内部表示。

所以很多团队更愿意把 `protected` 留给函数。

少把数据放出去。

### 一句话收束

继承方式。

不是给编译器看的装饰。

是你对外写下的合同条款。

它回答的不是“谁能访问”。

它回答的是“谁能把我当成谁”。

### 小结

`public` 继承强调 is-a。

它允许向上转型。

也要求你对替换性负责。

`private` 继承更像实现复用。

它拒绝 is-a。

`protected` 继承把“可见性”主要留给后续的派生层次。

你不必把它们背成三条定义。

你只要每次写下 `: public/protected/private Base` 的时候。

停一秒。

问自己一句。

我到底在跟谁签合同？
