// span_test_cases.cpp
// 编译命令：clang++ -std=c++20 span_test_cases.cpp -o span_test
// 运行命令：./span_test

#include <array>
#include <iomanip>
#include <iostream>
#include <span>
#include <vector>

// 通用检阅函数，使用 std::span 接收不同类型的数组
void inspect(std::span<const int> troops) {
  std::cout << "报告将军！一支由 " << troops.size()
            << " 名士兵组成的方阵前来报到！" << " 他们分别是：";
  for (int soldier : troops) {
    std::cout << soldier << " ";
  }
  std::cout << std::endl;
}

// 处理 RGB 颜色的函数，展示静态 span 的编译期检查
void process_rgb_color(std::span<const float, 3> color) {
  std::cout << "处理颜色: R=" << color[0] << ", G=" << color[1]
            << ", B=" << color[2] << std::endl;
}

// 展示 span 的切片功能
void demo_slice_operations(std::span<int> data) {
  std::cout << "原始数据: ";
  for (int val : data) {
    std::cout << val << " ";
  }
  std::cout << std::endl;

  // 使用 subspan 获取中间部分
  auto middle = data.subspan(1, 3);
  std::cout << "中间切片 (subspan(1, 3)): ";
  for (int val : middle) {
    std::cout << val << " ";
  }
  std::cout << std::endl;

  // 使用 first 获取前半部分
  auto front = data.first(2);
  std::cout << "前部切片 (first(2)): ";
  for (int val : front) {
    std::cout << val << " ";
  }
  std::cout << std::endl;

  // 使用 last 获取后半部分
  auto back = data.last(2);
  std::cout << "后部切片 (last(2)): ";
  for (int val : back) {
    std::cout << val << " ";
  }
  std::cout << std::endl;
}

// 展示生命周期问题（警示用例）
// std::span<int> dangerous_function() {
//   std::vector<int> local_data = {9, 9, 9}; // 局部变量
//   return std::span<int>(local_data); // 危险！返回了局部变量的视图
// }

int main() {
  std::cout << "========== 统一接口测试 ==========" << std::endl;
  // 1. 准备不同类型的数组
  std::vector<int> elite_troops = {1, 2, 3};
  std::array<int, 4> guard_troops = {4, 5, 6, 7};
  int veteran_troops[] = {8, 9};

  // 统一接口测试
  std::cout << "【vector 检阅】" << std::endl;
  inspect(elite_troops);

  std::cout << "【array 检阅】" << std::endl;
  inspect(guard_troops);

  std::cout << "【C数组 检阅】" << std::endl;
  inspect(veteran_troops);

  std::cout << "\n========== 切片操作测试 ==========" << std::endl;
  std::vector<int> squad = {1, 2, 3, 4, 5};
  demo_slice_operations(squad);

  std::cout << "\n========== 静态 span 测试 ==========" << std::endl;
  // 创建符合要求的 RGB 颜色
  std::array<float, 3> red = {1.0f, 0.0f, 0.0f};
  process_rgb_color(red);

  float blue[] = {0.0f, 0.0f, 1.0f};
  process_rgb_color(blue);

  // 下面这行如果取消注释会导致编译错误，因为尺寸不匹配
  // std::array<float, 4> color_with_alpha = {0.0f, 1.0f, 0.0f, 1.0f};
  // process_rgb_color(color_with_alpha); // 编译失败

  std::cout << "\n========== 生命周期警示 ==========" << std::endl;
  std::cout << "以下是危险操作的演示（实际代码中应避免）：" << std::endl;

  // 安全的使用方式：确保 span 生命周期不超过数据源
  {
    std::vector<int> safe_data = {7, 7, 7};
    std::span<int> safe_view(safe_data);
    std::cout << "安全的 span 使用: " << safe_view[0] << std::endl;
  } // safe_data 和 safe_view 都在这里销毁，很安全

  // 为了避免程序崩溃，这里只演示危险模式而不实际调用
  std::cout << "危险的返回局部变量的 span（已注释掉，防止崩溃）" << std::endl;
  // auto ghost_view = dangerous_function(); // 返回了悬挂的 span
  // std::cout << ghost_view[0] << std::endl; // 如果执行会导致未定义行为

  std::cout << "\n========== 测试完成 ==========" << std::endl;
  return 0;
}