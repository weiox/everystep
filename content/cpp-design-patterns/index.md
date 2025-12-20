---
title: "C++ 设计模式专栏"
description: "面向实际工程的设计模式专栏：聚焦现代 C++ 写法与常见痛点场景。"
---

## 为什么还要谈设计模式？

即使你从来没有系统学过 GoF 23 种设计模式，只要写过几年 C++，你多半已经在不自觉地"用模式"了。区别只在于：你是凭经验写出来的，还是有一套可以交流、可以推导、可以改进的"明规则"。

本专栏站在**现代 C++（C++11 起步）**的视角，系统讲解经典设计模式。我们会覆盖全部 23 种模式，但**按实战痛点而非传统分类**来组织内容，优先讲解工程中最常遇到的场景。

> 与其背定义，不如先记住：它解决了什么具体的痛点。

## 专栏结构

### 创建型模式（Creational Patterns）

关心“对象怎么被创建出来、谁负责哪一部分”的那些模式。

- [x] [**创建型模式总览：从满地 `new` 到对象生产线**](./creational-patterns-overview.md)
    - 串起单例、工厂、Builder、原型的演化故事与对比，建议先读这一篇再下场拆代码
- [x] [**单例模式 (Singleton)**](./singleton-pattern.md)
    - Meyer's Singleton、线程安全、何时该用/不该用
- [x] [**简单工厂模式 (Simple Factory)**](./simple-factory-pattern.md)
    - 从到处 `new` 到集中创建、简单工厂的优缺点
- [x] [**工厂方法模式 (Factory Method)**](./factory-method-pattern.md)
    - 简单工厂 vs 工厂方法、用多态接管“创建哪种对象”
- [x] [**抽象工厂模式 (Abstract Factory)**](./abstract-factory-pattern.md)
    - 一整族产品一起换皮、抽象工厂 vs 工厂方法、配置驱动整套对象
- [x] [**建造者模式 (Builder)**](./builder-pattern.md)
    - 链式调用、复杂对象的分步构造、给“大构造函数”找个装修队长
- [x] [**原型模式 (Prototype)**](./prototype-pattern.md)
    - 按样板房 clone 对象、解决多态复制和原型注册表的问题

### 结构型模式（Structural Patterns）

关心“类与对象怎么拼接在一起”，如何在不改动原有代码的前提下扩展结构。

- [x] [**结构型模式总览：从类拼接到稳定结构**](./structural-patterns-overview.md)
    - 串起适配器、装饰器、代理、组合、外观、桥接、享元这些“改接口、不乱动老代码”的套路，先看整体再下场拆模式
- [x] [**适配器模式 (Adapter)**](./adapter-pattern.md)
    - 接口转换、老代码接新库 / 新协议的小“转换头”、STL 与标准库中的各种适配器
- [x] [**装饰器模式 (Decorator)**](./decorator-pattern.md)
    - 在不改原类的前提下给对象加点料、Decorator vs 继承 / 代理 / 适配器
- [x] [**代理模式 (Proxy)**](./proxy-pattern.md)
    - 智能指针即代理、延迟加载、访问控制
- [ ] **组合模式 (Composite)** (🚧 计划中)
    - 树形结构、文件系统与 UI 控件
- [ ] **外观模式 (Facade)** (🚧 计划中)
    - 简化复杂子系统、API 设计
- [ ] **桥接模式 (Bridge)** (🚧 计划中)
    - 将接口与实现解耦、运行时切换实现
- [ ] **享元模式 (Flyweight)** (🚧 计划中)
    - 共享内部状态、对象池与内存优化

### 行为型模式（Behavioral Patterns）

关心“对象之间如何传递请求、协作完成一件事”。

- [ ] **策略模式 (Strategy)** (🚧 计划中)
    - 用多态或 std::function 替代长 if-else/switch
- [ ] **状态模式 (State)** (🚧 计划中)
    - 有限状态机、与策略模式的区别
- [ ] **观察者模式 (Observer)** (🚧 计划中)
    - 发布-订阅、信号槽机制、弱引用避免内存泄漏
- [ ] **命令模式 (Command)** (🚧 计划中)
    - 撤销/重做、宏命令、任务队列
- [ ] **责任链模式 (Chain of Responsibility)** (🚧 计划中)
    - 中间件、事件冒泡、请求过滤
- [ ] **模板方法模式与 NVI (Template Method & NVI)** (🚧 计划中)
    - 算法骨架、Non-Virtual Interface 惯用法
- [ ] **访问者模式 (Visitor)** (🚧 计划中)
    - 分离数据结构与操作、在不改类的前提下新增行为
- [ ] **迭代器模式 (Iterator)** (🚧 计划中)
    - STL 迭代器、范围 for 背后的思想
- [ ] **中介者 / 备忘录 / 解释器 (Mediator / Memento / Interpreter)** (🚧 计划中)
    - 控制对象交互中枢、快照与回滚、DSL 与脚本解释

### 现代 C++ 惯用法（与模式相关）

这些不是 GoF 经典 23 模式，但在现代 C++ 中经常和设计模式一起出现。

- [ ] **RAII 与资源管理** (🚧 计划中)
    - 智能指针、ScopeGuard、异常安全
- [ ] **依赖注入 (Dependency Injection)** (🚧 计划中)
    - 构造注入、接口注入、测试友好的设计
- [ ] **类型擦除 (Type Erasure)** (🚧 计划中)
    - std::function、std::any、std::unique_ptr<Concept> 的实现思路
- [ ] **CRTP：编译期多态** (🚧 计划中)
    - 静态多态、Mixin 类、性能优化

---
*注：标记为 🚧 的内容将陆续更新。本专栏基于 C++11 及以上标准。*
