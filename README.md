# ProblemX - 题库练习系统

![License](https://img.shields.io/badge/license-MIT-blue.svg)
![Qt](https://img.shields.io/badge/Qt-6.8-green.svg)
![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20macOS-lightgrey.svg)

## 项目概述

ProblemX 是一个基于 Qt 6.8 开发的跨平台题库练习系统，旨在帮助学生高效地进行各类科目的题目练习。系统支持多种题型（选择题、判断题、填空题），提供练习模式、错题复习、进度保存等功能，并支持从雨课堂等平台导入题库。

## 功能特点

- **多科目支持**：可自由添加和管理不同学科的题库
- **多题型支持**：支持选择题、判断题、填空题等多种题型
- **练习模式**：随机或顺序练习，支持自定义题目数量
- **错题管理**：自动收集错题，支持错题复习
- **进度保存**：支持中途保存练习进度，随时可以继续上次的练习
- **题库导入**：支持从雨课堂HTML格式导入题库
- **统计分析**：提供练习结果统计和分析
- **用户友好界面**：简洁直观的用户界面，操作便捷

## 安装说明

### 系统要求

- Qt 6.8 或更高版本
- C++17 兼容的编译器
- 支持 Windows、Linux 和 macOS 平台

### 从源码构建

1. 克隆仓库：

```bash
git clone https://github.com/SummerofOrange/ProblemX.git
cd ProblemX
```

2. 使用 Qt Creator 打开 `ProblemX.pro` 文件

3. 配置项目并构建

4. 运行应用程序

### 预编译版本

您也可以从 [Releases](https://github.com/SummerofOrange/ProblemX/releases) 页面下载预编译的二进制文件。

## 使用方法

### 基本操作

1. **启动程序**：运行 ProblemX 应用程序
2. **选择科目**：在开始界面选择要练习的科目
3. **配置练习**：选择题库、题型和题目数量
4. **开始练习**：按照提示完成题目练习
5. **查看结果**：练习完成后查看统计结果
6. **复习错题**：通过错题集功能复习错误的题目

### 题库管理

最开始，你应该导入一个题库，并进行配置。

题库以文件夹的形式，应按照以下目录结构组织：

```
科目/题型/题库.json
```

例如：

```
DataStructure/Choice/U1_Choice.json
```

题型目录包括：
- `Choice`：选择题
- `TrueorFalse`：判断题
- `FillBlank`：填空题

### 题库格式

题库文件采用 JSON 格式，具体格式请参考 [题库格式说明](Readme_ProblemBank.md)。

### 从雨课堂生成可使用的题库

1. 从雨课堂下载 HTML 格式的题库文件
2. 使用内置的转换工具转换为 ProblemX 格式：

```bash
python convert_Yuketang_to_problemx.py -i 雨课堂题库.html -o 输出目录 -name 生成的题库名称
```

详细使用说明请参考 [雨课堂转换工具说明](README_Yuketang_Converter.md)。

## 项目结构

```
├── core/                  # 核心功能模块
│   ├── configmanager.*    # 配置管理
│   ├── practicemanager.*  # 练习管理
│   ├── questionmanager.*  # 题目管理
│   └── wronganswerset.*   # 错题集管理
├── models/                # 数据模型
│   ├── question.*         # 题目模型
│   └── questionbank.*     # 题库模型
├── utils/                 # 工具类
│   ├── bankscanner.*      # 题库扫描
│   └── jsonutils.*        # JSON处理
├── widgets/               # 界面组件
│   ├── startwidget.*      # 开始界面
│   ├── configwidget.*     # 配置界面
│   ├── practicewidget.*   # 练习界面
│   └── reviewwidget.*     # 复习界面
├── Subject/               # 题库目录
├── convert_Yuketang_to_problemx.py  # 雨课堂转换工具
├── main.cpp               # 程序入口
├── mainwindow.*           # 主窗口
└── ProblemX.pro           # Qt项目文件
```

## 贡献指南

欢迎贡献代码、报告问题或提出改进建议！请遵循以下步骤：

1. Fork 本仓库
2. 创建您的特性分支 (`git checkout -b feature/amazing-feature`)
3. 提交您的更改 (`git commit -m 'Add some amazing feature'`)
4. 推送到分支 (`git push origin feature/amazing-feature`)
5. 创建一个 Pull Request

## 许可证

本项目采用 MIT 许可证 - 详情请参阅 [LICENSE](LICENSE) 文件。

## 联系方式

如有任何问题或建议，请通过以下方式联系我们：

- GitHub Issues: [https://github.com/SummerofOrange/ProblemX/issues](https://github.com/SummerofOrange/ProblemX/issues)
- Email: orangesummer.ovo@qq.com

## 更新日志

### **2025/05/31(Beta 0.2 latest)**
  - 修复了练习模块 (`practicewidget.cpp`) 和错题复习模块 (`reviewwidget.cpp`) 中，当题目文本（如代码片段）包含特殊HTML字符（例如 `<`, `>`）时，可能导致的题目显示不完整或渲染异常的问题。通过在 `updateQuestionDisplay` 和 `displayWrongAnswerDetails` 方法中对这些特殊字符进行转义，确保了题目内容的正确渲染。

请参阅 [CHANGELOG](CHANGELOG.md) 文件以获取详细的更新日志。

## 致谢

- [Qt Framework](https://www.qt.io/)
- 所有贡献者和用户