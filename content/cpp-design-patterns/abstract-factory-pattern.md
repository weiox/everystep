---
title: "抽象工厂模式 (Abstract Factory)：一整套家居套餐一起换皮"
description: "从 UI 主题皮肤和数据库驱动讲起，聊聊抽象工厂如何一次性切换一整族产品，以及它和工厂方法的分工。"
order: 40
---

如果说工厂方法是"把简单工厂长胖的那摊事，按场景拆成一排小工厂"，
那抽象工厂大概就是：

> 不光要决定“造哪种椅子”，
> 还要顺手把同一风格的桌子、沙发、床头柜一起配齐。
>
> 一句话：**我不是只卖一把椅子，而是卖一整套家居套餐。**

很多团队第一次真正用到抽象工厂，
往往不是在 C++ 教程里，而是在 UI 主题、数据库驱动、跨平台 SDK 这种场景里：

- 换一套 UI 皮肤，不只是换 Button，还要成套换 TextBox / Menu / Dialog；
- 换一个数据库驱动，不只是换 Connection，还要成套换 Statement / ResultSet；
- 换一个平台 SDK，不只是换一个 API 调用，而是一整套相关对象都要一起变。

有一次我们给线上产品换 UI 主题，结果半夜一看页面：

- 顶栏是新皮肤，侧边栏还是旧皮肤；
- 有的按钮已经用上了新样式，有的还顽强地停留在上个版本；

最后排查了半天，发现是有几处控件忘记跟着一起切换实现，
导致“同一页里混搭了两套家居”。
那次之后，大家才真的开始认真讨论：

> 换主题这件事，能不能别再靠人肉记忆，
> 而是有个机制保证“一整套东西要么一起换，要么都不换”？

这个时候，如果你还在用工厂方法“一个产品线一个工厂”，
就会发现：

- A 主题的 Button 从 `ThemeAButtonFactory` 出来；
- A 主题的 TextBox 从 `ThemeATextBoxFactory` 出来；
- A 主题的 Menu 从 `ThemeAMenuFactory` 出来；

看起来很 OO，但**想整体切一套主题的时候，你得自己把这几家工厂一一配对好**，
不小心还会混搭出“Button 用了 A 主题、TextBox 用了 B 主题”的奇怪效果。

抽象工厂就是在这个节点出场的：

- 它不再只关心“单一产品线 + 多种实现”；
- 而是关心“**一整族相关产品**该如何统一换皮”；
- 于是把“成套创建一整族产品”的接口，
  抽象成一个更大的“工厂家族入口”。

下面我们就沿着这个故事，
顺着简单工厂 → 工厂方法那条线，看看抽象工厂是怎么顺势长出来的。

先提前用一句话把这条线串在一起：

> **工厂方法帮你选一把椅子，
>  抽象工厂帮你一次性换一整套家。**

### 1. 先从一段真实代码开始：主题是怎么“拆散”的？

先别急着看模式结构图，先看一段很多人项目里都出现过的 UI 代码。

假设一开始，你只是想根据配置切换 Light / Dark 两套主题：

```cpp
void renderDashboard(const Config& cfg) {
    if (cfg.theme == "dark") {
        auto button = std::make_unique<DarkButton>();
        auto textBox = std::make_unique<DarkTextBox>();
        auto menu = std::make_unique<DarkMenu>();
        // ... 画一整页
    } else {
        auto button = std::make_unique<LightButton>();
        auto textBox = std::make_unique<LightTextBox>();
        auto menu = std::make_unique<LightMenu>();
        // ... 画一整页
    }
}
```

刚写完的时候，这段代码甚至还挺“清爽”：

- **一个 if 把两套实现分开**；
- **每个控件都手动 new 对应主题版本**。

### 2. 需求膨胀：从 if-else 到一排小工厂

后来需求一点点加：

- 仪表盘之外，又多了设置页 / 报表页 / 弹窗对话框；
- 每个页面都要照葫芦画瓢，自己 `if (cfg.theme == ...)` 一遍；
- 有人嫌烦，把 Button 提取成了工厂方法，把 TextBox 也提取成了工厂方法……

于是项目里就开始出现这一类东西：

- `ButtonFactory`，里面有 `createButton()`；
- `TextBoxFactory`，里面有 `createTextBox()`；
- `MenuFactory`，里面有 `createMenu()`。

这些工厂本身都没问题，各自都可以用工厂方法那一套思路来写。
真正的问题出在**调用方**：

```cpp
void renderDashboard(ButtonFactory&  btnFactory,
                     TextBoxFactory& txtFactory,
                     MenuFactory&    menuFactory) {
    auto button  = btnFactory.createButton();
    auto textBox = txtFactory.createTextBox();
    auto menu    = menuFactory.createMenu();
    // ...
}
```

只要有一个地方把 `DarkButtonFactory` 和 `LightTextBoxFactory` 混用，
你就会得到那种“按理说是 Dark 主题，结果某个输入框还是 Light 样式”的奇怪页面。

比如：你在改 Dark 主题的时候，把仪表盘页的三个工厂都换成了 Dark 版本，
但设置页只改了 Button/Menu 的工厂，忘了改 TextBox 的工厂，代码大概会长这样：

```cpp
void renderSettings(ButtonFactory&  btnFactory,
                    TextBoxFactory& txtFactory,
                    MenuFactory&    menuFactory);

void runDarkTheme() {
    DarkButtonFactory   btnFactory;
    LightTextBoxFactory txtFactory; // 这里不小心用了 Light 版本
    DarkMenuFactory     menuFactory;

    renderSettings(btnFactory, txtFactory, menuFactory);
}
```

这个时候：

- 页面整体背景、菜单都是 Dark 主题；
- 只有输入框还是 Light 主题，看起来就像“被忘记换皮”的残留控件。

这就是很多团队遇到的真实场景：

- **每条产品线（Button / TextBox / Menu）各自有一套工厂方法**；
- **但“这次到底要 Light 这套，还是 Dark 这套”，是散落在代码各处手动凑出来的**。

如果从实现角度来想一想，接下来要登场的 `ThemeFactory`，
其实就是**把刚才那三个分散的工厂（`ButtonFactory` / `TextBoxFactory` / `MenuFactory`）打包进了一个更大的工厂接口**：

- `ThemeFactory::createButton()` 可以理解为“内部帮你挑对了那一套里的 ButtonFactory，再去造一个 Button”；
- `ThemeFactory::createTextBox()` / `ThemeFactory::createMenu()` 也分别对应原来那两条产品线的工厂方法；

只是我们在示例代码里，为了简化展示，直接在 `LightThemeFactory` / `DarkThemeFactory` 里面 `make_unique<LightButton>()` / `make_unique<DarkButton>()`，
把“里层再套一层小工厂”的细节省略掉了。

如果你愿意把这个“打包三个小工厂”的结构摊开来写，代码可能更像下面这样：

```cpp
// 三条产品线各自的工厂接口（可以用工厂方法那一套实现）
struct ButtonFactory {
    virtual std::unique_ptr<Button> createButton() = 0;
    virtual ~ButtonFactory() = default;
};

struct TextBoxFactory {
    virtual std::unique_ptr<TextBox> createTextBox() = 0;
    virtual ~TextBoxFactory() = default;
};

struct MenuFactory {
    virtual std::unique_ptr<Menu> createMenu() = 0;
    virtual ~MenuFactory() = default;
};

// 抽象工厂接口表面上还是直接提供 createXxx
struct ThemeFactory {
    virtual std::unique_ptr<Button>  createButton()  = 0;
    virtual std::unique_ptr<TextBox> createTextBox() = 0;
    virtual std::unique_ptr<Menu>    createMenu()    = 0;
    virtual ~ThemeFactory() = default;
};

// 但具体实现里，其实是把那三条产品线的工厂聚在了一起
struct DarkThemeFactory : ThemeFactory {
    DarkButtonFactory  btnFactory_;   // Button 这条线专门的工厂
    DarkTextBoxFactory txtFactory_;   // TextBox 这条线专门的工厂
    DarkMenuFactory    menuFactory_;  // Menu 这条线专门的工厂

    std::unique_ptr<Button> createButton() override {
        return btnFactory_.createButton();
    }

    std::unique_ptr<TextBox> createTextBox() override {
        return txtFactory_.createTextBox();
    }

    std::unique_ptr<Menu> createMenu() override {
        return menuFactory_.createMenu();
    }
};
```

从调用方的角度看：

- 你只看到一个 `ThemeFactory&`，可以 `createButton()` / `createTextBox()` / `createMenu()`；
- 至于它内部是直接 `make_unique<DarkButton>()`，
  还是再包了一层 `DarkButtonFactory` 来帮忙创建，
  对业务代码来说完全透明。

这就是前面那句话的具体落地版本：

- 抽象工厂 `ThemeFactory` 是一个“更大的工厂入口”；
- 它可以在内部把多个“按产品线划分的工厂”组合在一起，对外统一发货。

所以，从工厂方法的视角看：

- 它一次解决的是“同一个抽象接口，选哪一个实现”的问题；
- 但当你要“**整套一起选**”时，它就显得有点力不从心。

抽象工厂，就是在这个节点顺势长出来的：  
既然总是要成套换 Button / TextBox / Menu，那就**把这整套产品族的创建入口，收拢到一个统一的抽象里**。

### 3. 典型结构：一整个产品族的“皮肤工厂”

先用 UI 主题的例子来画个典型结构图，脑补一下那种“整个界面一起换皮肤”的感觉：

```text
    Client → ThemeFactory (abstract)
                     ↑
          +----------+-----------+
          |                      |
   LightThemeFactory      DarkThemeFactory

    ↓            ↓             ↓
createButton  createTextBox  createMenu
```

如果落到 C++ 代码里，大致长这样：

```cpp
struct Button {
    virtual void draw() = 0;
    virtual ~Button() = default;
};

struct TextBox {
    virtual void draw() = 0;
    virtual ~TextBox() = default;
};

struct Menu {
    virtual void draw() = 0;
    virtual ~Menu() = default;
};

struct ThemeFactory {
    virtual std::unique_ptr<Button> createButton() = 0;
    virtual std::unique_ptr<TextBox> createTextBox() = 0;
    virtual std::unique_ptr<Menu> createMenu() = 0;
    virtual ~ThemeFactory() = default;
};
```

这里做了两件小事：

- 把“同一主题下的一整套控件”抽象成 `Button` / `TextBox` / `Menu` 三个接口；
- 再抽出一个 `ThemeFactory`，约定好：
  “你要什么控件，我就按当前主题风格给你造什么控件出来”。

具体主题的工厂，就各自负责“这一整套控件”的创建逻辑：

```cpp
struct LightThemeFactory : ThemeFactory {
    std::unique_ptr<Button> createButton() override {
        return std::make_unique<LightButton>();
    }

    std::unique_ptr<TextBox> createTextBox() override {
        return std::make_unique<LightTextBox>();
    }

    std::unique_ptr<Menu> createMenu() override {
        return std::make_unique<LightMenu>();
    }
};

struct DarkThemeFactory : ThemeFactory {
    std::unique_ptr<Button> createButton() override {
        return std::make_unique<DarkButton>();
    }

    std::unique_ptr<TextBox> createTextBox() override {
        return std::make_unique<DarkTextBox>();
    }

    std::unique_ptr<Menu> createMenu() override {
        return std::make_unique<DarkMenu>();
    }
};
```

这里没什么玄学，就是把“Light 这一套控件”和 “Dark 这一套控件”
各自塞进一个对应的工厂里：

- 你要 Button，就从对应主题的工厂要 Button；
- 你要 TextBox / Menu，也是同理；

于是“这套 UI 用 Light 还是 Dark”，
就不再是散落在代码各处的 if-else，而是收拢成了：

> 这次到底选 `LightThemeFactory`，还是选 `DarkThemeFactory`？

业务代码这边，继续沿用“只认识抽象，不认识细节”的套路，只握着一个 `ThemeFactory&`：

```cpp
struct Window {
    Window(ThemeFactory& factory)
        : button_(factory.createButton()),
          textBox_(factory.createTextBox()),
          menu_(factory.createMenu()) {}

    void draw() {
        button_->draw();
        textBox_->draw();
        menu_->draw();
    }

private:
    std::unique_ptr<Button> button_;
    std::unique_ptr<TextBox> textBox_;
    std::unique_ptr<Menu> menu_;
};
```

老项目里你可能见过一大堆类似 `window.setButtonTheme(...)`、`window.setMenuTheme(...)` 之类的调用，
手动把一堆控件调成同一主题。

抽象工厂的味道就是：

- `Window` 根本不直接碰具体控件类型；
- 只在构造的时候要一个 `ThemeFactory&`，
  剩下“这一整套控件长什么样”，全交给工厂去操心。

你可以在程序启动时，根据配置/平台/用户设置，选出一个合适的 `ThemeFactory` 实例：

```cpp
std::unique_ptr<ThemeFactory> makeFactory(const Config& cfg) {
    if (cfg.theme == "dark") {
        return std::make_unique<DarkThemeFactory>();
    } else {
        return std::make_unique<LightThemeFactory>();
    }
}
```

后面的业务代码完全不用关心“现在是 Light 还是 Dark”，
也不需要分别注入 Button/TextBox/Menu 的工厂：

- 只要拿到一个 `ThemeFactory&`，
- 它就能帮你造出一整套同主题的控件。

从这个角度看：

> 抽象工厂就是“**一整族相关产品的一站式工厂**”，
> 把成套出现的对象创建打包到了同一个抽象里。

### 4. 和工厂方法的关系：谁在谁上面？

很多同学第一次看抽象工厂，
都会有点晕：

> 这不就是“几个工厂方法放在一个接口上”吗？
> 那它到底和工厂方法有什么本质区别？

比较实在的看法是：

- **工厂方法**关心的是“**单一产品线 + 多个实现**”，
  - 比如都是 `Button`，只是 Light / Dark 主题不同；
  - 比如都是 `Connection`，只是 MySQL / PostgreSQL 实现不同；
- **抽象工厂**关心的是“**一组相关产品族**”，
  - 比如同一主题下的 Button / TextBox / Menu 一整套；
  - 比如同一数据库驱动下的 Connection / Statement / ResultSet 一整套。

很多书上会说：

> 抽象工厂是“工厂方法的工厂”。

翻译成更接地气的话，就是：

- 抽象工厂的每一个方法，
  本质上就是一个“工厂方法”；
- 只不过抽象工厂把**一整族相关的工厂方法**打了个包，
  放在同一个抽象接口下面统一管理。

用刚才的 `ThemeFactory` 举例：

- `createButton()` 是一个工厂方法；
- `createTextBox()` 是另一个工厂方法；
- `createMenu()` 也是一个工厂方法；

这三者合在一起，
就构成了这个“UI 主题”这条产品族的抽象工厂。

换句话说：

> 当你发现自己在不同地方到处传 `ButtonFactory`、`TextBoxFactory`、`MenuFactory`，
> 只是为了保证它们配成一套主题时，
> 你基本就已经站在抽象工厂模式的门口了。

### 5. 什么时候用抽象工厂，什么时候停在工厂方法？

很多人会问：

> 我是不是应该“一上来就用抽象工厂”？
> 还是先用工厂方法，等需要的时候再升级？

结合简单工厂 / 工厂方法前两篇，我们可以拉一条“升级曲线”：

- **阶段 1：到处乱 `new`**
  - 任何地方想要对象就 `new`，构造参数四处散落；
- **阶段 2：简单工厂**
  - 把 `new` 收拢到一个 `SimpleFactory::create(type)` 里；
- **阶段 3：工厂方法**
  - 当简单工厂开始按“环境/渠道/客户”写大块 if，
    把这些场景拆成一堆小工厂子类（每个场景一个工厂）；
- **阶段 4：抽象工厂**
  - 当你发现“换一次场景 / 主题 / 驱动”，
    不只是换一个类，而是一整套相关类要一起换时，
    就该考虑抽象工厂出场了。

更直接一点的判断标准可以是：

- **如果你主要是在同一个抽象接口下，替换不同实现**（比如 `Task` 在不同环境的实现），
  - 工厂方法足够；
- **如果你经常要把一整套相关对象打包一起换**（比如一整套 UI 皮肤、一整套 DB 相关类），
  - 抽象工厂会更合适。

回到一线项目里看，其实抽象工厂总是在几类“老场景”里反复登场：

- **UI 主题 / 皮肤那一挂**：很多团队都是先从「到处 if (theme == ...)」写起，某次熬到半夜上线 Dark 模式，发现同一页里 Button 是黑的、Dialog 还是白的，这时候才痛定思痛，把一整套控件按主题打包成族——你一眼选 `DarkThemeFactory`，后面 Button / TextBox / Menu 就全跟着换皮了。
- **数据库 / 后端接入这一挂**：早年大家都是在代码里到处 `if (driver == "mysql") ... else if (driver == "pg") ...`，后来一波从 MySQL 迁到 PostgreSQL、再上云厂商托管，才发现真正要换的不是一个 `Connection`，而是一整套 Connection / Statement / ResultSet / Transaction 的组合，这时抽象工厂就很自然地变成“这一家数据库家族”的总入口。
- **跨平台 / 多品牌这一挂**：做过桌面 GUI 或多租户 SaaS 的同学应该都有体会：Windows / Linux / macOS 各有各的控件族，大客户 A / 品牌 B 各有各的 UI / 文案 / 风格要求。一开始你可能靠配置 + if-else 顶着写，等到第三个品牌、第四个平台一上，大家通常会妥协：不如认个命，把“这一整套平台/品牌家族的组件”封成一个抽象工厂，换环境就换工厂，实现自己去长。

在 C++ 项目里，一个比较常见的节奏是：

- 先用工厂方法把“单个抽象类型”的实现切干净；
- 等到“配套出现的一整组对象”越来越多，
  再把它们收拢到一个抽象工厂接口下面。

你也可以顺手对照一下自己项目里的代码：

- 你们现在切数据库驱动 / UI 主题，是改一堆配置 + if-else，
  还是只换一处“选哪个工厂实现”的入口？
- 有没有遇到过那种“半套新皮肤 + 半套旧皮肤”的诡异页面，
  排查下来才发现是某几个控件忘了跟着一起换？

### 6. 小结：从“一把椅子”到“一整套家居”

回过头看简单工厂 → 工厂方法 → 抽象工厂这条线，
其实就是从：

- “哪儿缺对象就地 `new` 一把椅子”；

走到：

- “所有椅子都从同一个简单工厂领”；

再走到：

- “不同场景，各自有一个更懂自己的椅子工厂”（工厂方法）；

最后自然会问出那句：

- “我能不能直接买一整套同风格的家居套餐？”（抽象工厂）。

作为一个写 C++ 写久了的人，我更愿意这样记抽象工厂：

- **它不是为了显得高级，而是当“产品族”这个概念在你项目里变成日常用语之后，顺水推舟长出来的抽象。**
- 当你发现自己总在手动搭配“Button 工厂 / TextBox 工厂 / Menu 工厂”这几件东西时，
  就差不多可以考虑，是不是该给它们头上再盖一层“ThemeFactory”了。

真正有用的，不是你能背出多少模式的结构图，
而是下次再看到“成套出现的一整组对象”时，
脑子里会闪一下：

> 这次我是在换“一把椅子”，
> 还是已经在换“一整套家居套餐”？
>
> 如果只是前者，工厂方法就够了；
> 如果是后者，那就是抽象工厂上场的时候了。

当你开始成套换对象，而不是只换一个类时，
抽象工厂这个名字就不再显得“教科书”，
而只是给你已经在做的那件事，贴了一个顺水推舟的标签。

如果你前面还没看简单工厂 / 工厂方法那两篇，
可以回去对照一下：你们现在的工厂，停在了哪一个阶段？

再往后走，等把“创建这一块”这三兄弟讲完，
我们还有建造者（Builder）之类的角色，会从“分步构建复杂对象”的角度继续补全这条线。
