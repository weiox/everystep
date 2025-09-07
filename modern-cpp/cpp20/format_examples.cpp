/**
 * std::format 功能验证示例
 * 
 * 编译命令:
 * clang++ -std=c++20 format_examples.cpp -o format_examples
 * 
 * 运行命令:
 * ./format_examples
 * 
 * 注意：需要支持 C++20 的编译器。如果编译失败，可能需要更新编译器或使用更多标志:
 * clang++ -std=c++20 -stdlib=libc++ format_examples.cpp -o format_examples
 */

#include <iostream>
#include <string>
#include <format>
#include <vector>
#include <numbers> // C++20 提供了数学常数，比如 PI

// 自定义类型示例
struct Point {
    int x, y;
};

// 为 Point 特化 std::formatter
template <>
struct std::formatter<Point> {
    // parse 函数：解析格式字符串
    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    // format 函数：执行格式化
    auto format(const Point& p, std::format_context& ctx) const {
        return std::format_to(ctx.out(), "({}, {})", p.x, p.y);
    }
};

// 商品结构体，用于对齐示例
struct Item {
    std::string name;
    double price;
};

int main() {
    std::cout << "===== std::format 功能验证 =====\n\n";

    // 1. 基本用法示例
    std::cout << "【基本用法】\n";
    std::string student_name = "小明";
    int score = 95;
    std::string message = std::format("{}同学, 你的期末成绩是 {} 分!", student_name, score);
    std::cout << message << "\n\n";

    // 2. 类型安全示例
    std::cout << "【类型安全】\n";
    std::cout << "以下代码在编译时会失败 (已注释):\n";
    std::cout << "std::format(\"{:d}\", \"我是一个字符串\");\n\n";
    // 如果取消下面的注释，将在编译时失败
    // std::format("{:d}", "我是一个字符串");

    // 3. 对齐与填充示例
    std::cout << "【对齐与填充】\n";
    std::vector<Item> shopping_list = {{"苹果", 5.0}, {"香蕉", 2.5}, {"草莓蛋糕", 25.8}};
    std::cout << std::format("{:<12}{:>10}\n", "商品", "价格");
    std::cout << "----------------------\n";
    for (const auto& item : shopping_list) {
        std::cout << std::format("{:<12}{:>10.2f}\n", item.name, item.price);
    }
    std::cout << "\n";

    // 4. 数字格式化示例
    std::cout << "【数字格式化】\n";
    std::cout << std::format("圆周率 PI ≈ {:.4f}\n", std::numbers::pi);
    int number = 42;
    std::cout << std::format("十进制: {}\n", number);
    std::cout << std::format("二进制: {:b}\n", number);
    std::cout << std::format("带前缀的十六进制: {:#x}\n", number);
    std::cout << "\n";

    // 5. 自定义类型格式化示例
    std::cout << "【自定义类型格式化】\n";
    Point p = {10, 20};
    std::string s = std::format("坐标点是: {}", p);
    std::cout << s << "\n\n";

    // 6. 填充字符示例
    std::cout << "【填充字符】\n";
    std::cout << std::format("{:*^20}\n", "居中");
    std::cout << std::format("{:->20}\n", "右对齐");
    std::cout << std::format("{:-<20}\n", "左对齐");
    std::cout << "\n";

    // 7. 位置参数示例
    std::cout << "【位置参数】\n";
    std::cout << std::format("重复使用参数: {0}, 再次是 {0}, 第二个参数是 {1}\n", "第一个", "第二个");
    std::cout << "\n";

    // 8. 比较与printf和iostream
    std::cout << "【对比】\n";
    const char* name = "Alex";
    int level = 99;
    
    std::cout << "printf 风格: ";
    printf("玩家: %s, 等级: %d\n", name, level);
    
    std::cout << "iostream 风格: ";
    std::cout << "玩家: " << name << ", 等级: " << level << std::endl;
    
    std::cout << "std::format 风格: ";
    std::cout << std::format("玩家: {}, 等级: {}\n", name, level);
    
    return 0;
} 