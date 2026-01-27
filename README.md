# ProblemX - 题库练习系统

![License](https://img.shields.io/badge/license-MIT-blue.svg)
![Qt](https://img.shields.io/badge/Qt-6.8-green.svg)
![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20macOS-lightgrey.svg)

## 项目概述

ProblemX 是一个基于 Qt 6.8 开发的跨平台题库练习系统，旨在帮助学生高效地进行各类科目的题目练习。系统支持多种题型（选择题、多选题、判断题、填空题），提供练习模式、错题复习、进度保存等功能，并支持从 PTA、雨课堂等平台导入题库。

## 功能特点

- **多科目支持**：可自由添加和管理不同学科的题库
- **多题型支持**：支持选择题、多选题、判断题、填空题等多种题型
- **智能题库获取**：内置 PTA 题库抓取工具，一键解析题目、评测状态与图片
- **本地化图片管理**：自动下载题库图片，实现题库完全离线化
- **增强型题库编辑**：全新卡片式编辑器，支持实时预览、Markdown/LaTeX 编辑与评测状态标记
- **Markdown渲染**：支持在题目中使用Markdown格式，包括粗体、斜体、代码块等
- **LaTeX数学公式**：集成KaTeX引擎，支持在题目中渲染LaTeX数学公式
- **练习模式**：随机或顺序练习，支持自定义题目数量
- **错题管理**：自动收集错题，支持错题复习
- **进度保存**：支持中途保存练习进度，随时可以继续上次的练习
- **统计分析**：提供练习结果统计和分析

## 安装说明

### 系统要求

- Qt 6.8 或更高版本
- Qt WebEngine 模块（用于Markdown和LaTeX渲染）
- Qt Network 模块（用于图片下载）
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

你可以参考[Quick Start Guide](QuickStart.md)来快速上手ProblemX。

### 基本操作

1. **启动程序**：运行 ProblemX 应用程序
2. **选择科目**：在开始界面选择要练习的科目
3. **配置练习**：选择题库、题型和题目数量
4. **开始练习**：按照提示完成题目练习
5. **查看结果**：练习完成后查看统计结果
6. **复习错题**：通过错题集功能复习错误的题目
7. **编辑题库**：方便用户自行编辑题库

### 题库管理

**新建科目与题库：**
1. 在“配置程序”界面，点击左下角的【创建科目】，输入名称即可建立新科目。
2. 选中科目后，点击【创建题库】，输入名称并选择模板，即可快速生成题库文件。

**题库结构：**
题库以文件夹形式组织，结构如下：

```
科目/题型/题库.json
科目/题型/asset/ (可选，存放图片)
```

例如：
```
DataStructure/Choice/U1_Choice.json
DataStructure/Choice/asset/img1.png
```

### 从 PTA 平台智能获取题库（推荐）

ProblemX V2.1（Beta） 内置了强大的 PTA 题库抓取工具：

1. 在“配置程序”界面选中科目，点击【自动获取题库】。
2. 在弹出的窗口中，左侧浏览器登录 Pintia 账号并进入题目集详情页。
3. 点击【解析当前页面的...】按钮，系统会自动抓取题目。
4. **评测状态同步**：系统会自动识别题目是否已通过评测，用绿/红/黄颜色标记。
5. **图片本地化**：点击【保存当前题库】，勾选“保存时下载图片”，即可将题目图片保存到本地，实现离线刷题。

### 从雨课堂/学习通生成题库

对于其他平台，可以使用内置的 Python 转换脚本：

- **雨课堂**：`python convert_Yuketang_to_problemx.py ...` ([说明](README_Yuketang_Converter.md))
- **学习通**：`python extract_xuexitong_questions.py ...` ([说明](README_Xuexitong_Converter.md.md))
- **PTA (脚本方式)**：`python convert_PTA_to_problemx.py ...` ([说明](README_PTA_Converter.md))

### 升级旧版题库

如果您持有 v1.0 及以前版本的 JSON 题库，请使用以下命令升级：

```bash
python convert_old_problembank.py <题库根目录>
```

## 项目结构

```
├── core/                  # 核心功能模块
├── models/                # 数据模型
├── utils/                 # 工具类
├── widgets/               # 界面组件
│   ├── bankeditorwidget.* # 增强型题库编辑器
│   ├── ptaimportdialog.*  # PTA 智能导入窗口
│   └── ...
├── Subject/               # 题库目录
├── convert_*.py           # 转换工具脚本
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

### **V2.1（Beta）(latest)**

- **新增 PTA 智能题库获取**：内置浏览器一键解析、评测状态识别、图片本地化下载
- **题库编辑器重构**：卡片式列表 UI、支持嵌入模式与任意文件读写
- **配置体验升级**：题库启用勾选框下沉至左侧列表；抽取数量支持滑动条+输入框联动；新增“乱序题目”开关并持久化

请参阅 [CHANGELOG](CHANGELOG.md) 文件以获取详细的更新日志。

## 致谢

- [Qt Framework](https://www.qt.io/)
- 所有贡献者和用户
