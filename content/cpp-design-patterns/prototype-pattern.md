---
title: "原型模式 (Prototype)：按样板房复制对象"
description: "从拷贝构造、对象切片和配置样板讲起，聊聊原型模式如何通过 clone 在多态层次上复制对象，以及它和工厂 / Builder 的分工。"
order: 60
---

如果你在 C++ 项目里待得够久，大概率经历过这么几件事：

- 一开始，代码里满地 `new`，哪里需要对象就地起炉灶；
- 后来嫌乱，开始搞“简单工厂 / 工厂方法 / 抽象工厂”，
  把 `new` 收拢到几个地方统一管理；
- 再后来，你发现有些对象配置巨复杂，好不容易配好了，
  结果产品一句“再来一份，稍微改两下”，
  你又得从头把那一坨参数重新敲一遍。

工厂家族主要解决的是第一类问题：

> “我要**新搞一个对象**，从无到有，怎么搞得干净点？”

原型模式关心的是第二类：

> “我这已经有一个**配好了的对象**，
>  现在就想照着它**再复制一份**，
>  能不能别每次都从零开始重新装修？”

在不少老项目里，你能看到这样的经典画面：

- 系统启动时，先在某个初始化阶段，把一堆“模板对象”配好；
- 业务代码需要新实例时，不是重新配置，而是**先复制模板，再改几处关键参数**；
- 刚开始大家拿 `memcpy` 或手写 `copy` 硬抄；
- 慢慢地，某个老同事受不了，提了一句：
  “要不，咱们干脆在基类上约个 `clone()`，
   让**对象自己**负责怎么复制自己算了？”

这背后，就是原型模式（Prototype）最朴素的出发点：

- 在基类上约定一个 `clone()` 接口；
- 每个派生类自己决定“我该怎么复制我自己”；
- 上层只需要握着一个多态指针，
  在**不知道确切类型**的前提下，
  就能复制出一份“一模一样的新对象”。

GoF 那本《Design Patterns》里就专门单独拎出了 Prototype，
但真正把它玩明白、玩深入的，其实是 C++ 这一拨人：

- 一边要面对“对象切片、拷贝构造、多态”这些底层细节；
- 一边还要在 GUI 库、游戏引擎、工作流引擎里，
  给策划 / 产品 / 业务提供“**按样板房复制对象**”的体验。

下面我们就从 C++ 里非常典型、
也非常容易踩坑的“对象切片”讲起，
顺便看一眼：为什么一大摊老 C++ 代码里，都有个 `virtual clone()`。

### 1. 对象切片与多态拷贝：拷贝构造并不总够用

先看一段非常“教科书”的 C++ 代码：

```cpp
struct Shape {
    virtual void draw() const = 0;
    virtual ~Shape() = default;
};

struct Circle : Shape {
    double radius;

    void draw() const override {
        // ... 画圆 ...
    }
};

struct Rectangle : Shape {
    double w, h;

    void draw() const override {
        // ... 画矩形 ...
    }
};
```

假设我们有一个 `std::vector<std::unique_ptr<Shape>>` 存放各种图形：

```cpp
std::vector<std::unique_ptr<Shape>> shapes;
shapes.push_back(std::make_unique<Circle>(Circle{10.0}));
shapes.push_back(std::make_unique<Rectangle>(Rectangle{3.0, 4.0}));
```

某一天，你想“复制一份 `shapes` 出来，再单独改动”：

```cpp
std::vector<std::unique_ptr<Shape>> shapesCopy;
for (const auto& s : shapes) {
    // ??? 怎么在不知道具体类型的前提下复制一份？
}
```

很多人第一次卡在这里，就是下意识地想：

- “拷贝构造这么香，直接 `*s` 拷一份不就完了？”

结果一运行，多态全失灵：

- 你以为自己复制的是一个 `Circle`，
  实际上只拷了 `Shape` 那一截；
- 派生类的信息被“削掉”了，
  这就是经典的**对象切片（object slicing）**；
- 想靠一堆 `dynamic_cast` 把所有派生类一个个识别出来，
  不但丑，还很快会变成“谁加一个派生类，谁就得来改这一坨 if / switch”。

这就是“**在多态层次上复制对象**”这道老难题：

- 普通拷贝构造解决的是“**我知道你是谁**”时的复制；
- 而我们现在手里只有一个 `Shape*` / `Shape&`，
  只知道“你是个形状”，不知道你是圆还是矩形。

原型模式给出的答案其实很朴素：

> 既然你想在**不知道确切类型**的情况下复制对象，
> 那就让“对象自己”来承担复制的责任。

你别猜我是谁，让我自己告诉你：

- 对于圆来说，“复制自己”就是再来一个半径一样的圆；
- 对于矩形来说，则是再来一块宽高一样的矩形；
- 这些事，交给派生类最合适不过。

### 2. 一个经典的 C++ 原型写法：virtual clone()

在 C++ 圈里，Prototype 最接地气的一种写法就是：

> 在基类上约定一个 `virtual clone()`。

比如刚才的 `Shape`：

```cpp
struct Shape {
    virtual void draw() const = 0;
    virtual std::unique_ptr<Shape> clone() const = 0;
    virtual ~Shape() = default;
};
```

每个派生类各自实现“如何复制自己”：

```cpp
struct Circle : Shape {
    double radius;

    void draw() const override {
        // ... 画圆 ...
    }

    std::unique_ptr<Shape> clone() const override {
        return std::make_unique<Circle>(*this);
    }
};

struct Rectangle : Shape {
    double w, h;

    void draw() const override {
        // ... 画矩形 ...
    }

    std::unique_ptr<Shape> clone() const override {
        return std::make_unique<Rectangle>(*this);
    }
};
```

上层代码看起来就舒服多了：

```cpp
std::vector<std::unique_ptr<Shape>> shapes;
// ... 填充 shapes ...

std::vector<std::unique_ptr<Shape>> shapesCopy;
for (const auto& s : shapes) {
    shapesCopy.push_back(s->clone());
}
```

这就是地地道道的“原型模式”：

- `Shape` 作为**原型接口**，只负责说清楚：“我能被克隆”；
- `Circle` / `Rectangle` 作为具体原型，各自决定“怎么拷贝出一个自己”；
- 调用方只管拿着一个 `Shape&` 或 `Shape*`，
  调 `clone()` 就能得到一份同款新对象。

如果你去翻一些老一点的 C++ GUI 库、游戏引擎、流程引擎的源码，
经常能看到类似签名：

- 早年：`virtual Shape* clone() const = 0;`，
- C++11 之后，逐步演进成：`virtual std::unique_ptr<Shape> clone() const = 0;`。

这些 `clone()`，背后做的都是一件事：

> “拿一个已经配置好的样板对象，
>  在多态层次上**复制出一份一模一样的新对象**。”

### 3. 原型注册表：按名字拿样板，按样板克隆

现实项目里，原型往往不止一个，而是一整柜“样板房”：

- 游戏里有 `"orc_warrior"`、`"orc_mage"`、`"elf_archer"`；
- UI 系统里有 `"primary_button"`、`"danger_button"`、`"ghost_button"`；
- 计费系统里有 `"basic_plan"`、`"pro_plan"`、`"enterprise_plan"`。

每个样板的配置都不太一样，
但本质上都是“以后可以反复复制的模板”。

这个时候，单有 `clone()` 还不够顺手，
我们通常会再加一层**原型注册表（Prototype Registry）**：

```cpp
struct PrototypeRegistry {
    void registerPrototype(std::string name, std::unique_ptr<Shape> proto) {
        prototypes_[std::move(name)] = std::move(proto);
    }

    std::unique_ptr<Shape> create(const std::string& name) const {
        auto it = prototypes_.find(name);
        if (it == prototypes_.end()) return nullptr;
        return it->second->clone();
    }

private:
    std::map<std::string, std::unique_ptr<Shape>> prototypes_;
};
```

用起来是这个味道：

```cpp
PrototypeRegistry registry;
registry.registerPrototype("big_circle",
    std::make_unique<Circle>(Circle{10.0}));
registry.registerPrototype("small_rect",
    std::make_unique<Rectangle>(Rectangle{1.0, 2.0}));

auto s1 = registry.create("big_circle");
auto s2 = registry.create("big_circle");
auto s3 = registry.create("small_rect");
```

每次 `create("big_circle")`：

- 你拿到的都是一份**全新的 `Circle` 副本**；
- 而不是对同一个 `Circle` 反复改来改去，
  最后谁也说不清这货现在到底是什么配置。

教科书里把这套组合叫“原型注册表”，
其实就是两层事：

- 注册阶段：先准备好一批“样板对象”；
- 运行时：按名字查找样板，调用 `clone()` 复制；
- 调用方既不用关心具体类型，
  也不用重复那一大坨初始化细节。

如果你做过游戏或者重配置的业务系统，
大概遇到过类似的句子：

> 策划：“这个怪不错，再复制 200 只，血量稍微调一调就行。”

听上去像是“改个数字”的小需求，
背后这种“按样板大规模复制再微调”的模式，
用 Prototype + Registry 来抽象，是非常自然的一步。

### 4. 原型 vs 工厂 / Builder：它到底解决的是什么？

说到这里，免不了要和工厂 / Builder 再对一下拳：

> “这不还是在**创建对象**吗？
>  跟工厂 / Builder 比，原型究竟特别在哪？”

还是用房子的比喻更直观：

- **工厂 / 抽象工厂**：
  - 你在售楼处选“哪一个户型 / 哪一套家居套餐”；
  - 开发商按一套既定规则**重新造一套**给你；
- **Builder**：
  - 你和装修队长一起，从毛坯房开始一步步选材选方案；
  - 关注的是“整个装修过程是否清晰、可控”；
- **Prototype 原型**：
  - 你先逛一圈样板间，
  - 看中其中一套之后说：“就按这套给我**复制一套**，
    地板颜色稍微浅一点就行。”

落到代码里，原型模式更适合这种情况：

- 对象的**配置非常复杂**，而且这一套配置已经比较稳定了；
- 以后你会经常“按某个模板再来一份”，
  只在少数字段上做小差异；
- 复制出来的对象可能是多态层次里的任何一个派生类。

这时候当然也可以用工厂，
但体验很容易变成：

- 你得重新把所有配置参数排一遍队；
- 稍微改个默认值，就要在两三处地方一起改；
- 如果模板对象本身是**运行时算出来的**，
  工厂函数也很难写死在代码里。

原型模式则更像是顺势而为：

- 先让系统里自然长出一批“配置好的对象”；
- 在基类上约一个 `clone()`；
- 真正用的时候，**先 `clone()` 一份，再在上面做小修改**。

一句话归纳这俩的分工：

> **工厂擅长“从规则出发创建新对象”，
>  原型擅长“从现有对象出发复制新对象”。**

很多成熟系统里，这两者是同时存在、各司其职的：

- 工厂负责**第一次造出样板对象**；
- 原型负责**后续按样板批量复制**；
- Builder 则在“第一次造样板”的过程中，
  把那一坨复杂初始化写得更可读一些。

### 5. 在 C++ 里用好 Prototype，需要注意什么？

Prototype 在书上看着挺简单，
真落在 C++ 代码里，还是有几个坑容易翻车：

- **对象切片**
  - 在多态层次上用值拷贝代替 `clone()`，
    基本等于主动把派生类那一截给切掉；
  - 一旦你在容器里存的是值（比如 `std::vector<Shape>`），
    多态基本就算废了。

- **深拷贝 vs 浅拷贝**
  - `clone()` 里常见写法是 `std::make_unique<Derived>(*this)`，
    等价于调用派生类的拷贝构造；
  - 如果类里有裸指针或共享资源，
    就要认真想一想：
    “我是真的要再来一份资源，
     还是只是多一个引用？”
  - 很多历史遗留 bug，
    都是“以为自己做了深拷贝，
     实际上大家还在抢同一块资源”。

- **资源所有权**
  - 早年 C++ 里，`clone()` 常见返回值是 `Base*`，
    谁 `clone()` 谁 `delete`，全靠自觉；
  - 现代 C++ 里，更推荐让 `clone()` 返回 `std::unique_ptr<Base>`：
    - 一眼就能看出“这个对象的所有权交给调用方”；
    - 也减少了“漏写 delete”这类纯体力型事故。

于是又回到那个常见的问题：

> 既然有了拷贝构造，为什么还需要 `clone()`？

原因其实很直接：

- 拷贝构造只能在“**我知道你确切类型**”的时候用；
- 多态场景下，很多时候你只有一个 `Base&` 或 `Base*`；
- 这个时候，**`virtual clone()` 是极少数不用 RTTI / `dynamic_cast`，
  就能优雅解决问题的办法之一。**

你也可以对照一下自己项目里的代码：

- 有没有那种“先 `new` 一个对象、配好一大堆参数，
  再在不同地方复制来复制去”的写法？
- 有没有一串 `switch(type) { case Circle: ... case Rect: ... }`，
  里面偷偷在做“多态对象的拷贝”？

如果有，这些地方大概率都可以抽象成“原型 + 注册表”的组合：

- 一方面减轻调用方的心智负担，
- 另一方面也让新增一个派生类这件事，
  更接近“只改局部、不动大局”。

---

说到这里，创建型这条线上的几个主力角色，
基本都露过一面了：

- 简单工厂：从满地 `new` 到集中开工厂；
- 工厂方法：当简单工厂胖到没人敢改时的自然进化；
- 抽象工厂：一整套家居套餐一起换皮；
- 建造者：给复杂对象找一个靠谱的“装修队长”；
- 原型：给配置复杂的对象准备几套“样板房”，
  以后按样板复制一份、再轻微改造。

以后你再写 C++，
每次准备“创建一个对象”的时候，
可以在心里默念几句：

> 这次我是要“选户型 / 套餐”？
>
> 还是在“装一套复杂的房子”？
>
> 还是已经有了一套样板房，
>  只想按它的样子再来一份？

能把这几个问题想清楚，
往往比背下一整张 UML 图都管用得多。
