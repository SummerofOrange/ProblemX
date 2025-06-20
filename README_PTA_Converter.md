# ProblemX PTA 题库转换工具使用指南

## 1. 概述

本工具 (`convert_PTA_to_problemx.py`) 旨在帮助用户将从 **PTA (Programming Teaching Assistant)** 平台导出的HTML格式题库文件，转换为 **ProblemX** 刷题系统所支持的JSON格式。通过此工具，您可以方便地将PTA的题目资源导入到ProblemX中进行练习和管理。

## 2. 支持的题型

目前，本转换工具支持以下常见的PTA题型：

- **选择题 (Choice)**
- **判断题 (TrueorFalse)**
- **填空题 (FillBlank)**

## 3. 输入HTML文件格式要求

为了确保转换工具能够正确解析，输入的PTA HTML文件应大致遵循以下结构：

- 每个题目包含在一个 `div.pc-x[id]` 容器中。
- 题干位于 `div.rendered-markdown p`。
- 选项和答案的结构根据题型有所不同，工具会根据特定选择器进行解析。

**注意**：HTML结构可能因PTA版本更新而略有差异，工具的解析逻辑基于常见的HTML模式。

## 4. 输出JSON文件格式

转换成功后，输出的JSON文件将严格遵循ProblemX系统的题库标准。具体格式请参考项目根目录下的 `Readme_ProblemBank.md` 文件。

## 5. 使用方法

### 通过命令行运行

推荐使用命令行方式运行转换脚本，具有较高的灵活性。

```bash
python convert_PTA_to_problemx.py <input_html_path> -type <question_type> -o <output_json_path>
```

**参数详解:**

- `input_file` (必需): 指定输入的PTA HTML文件的**完整路径**。
- `-type` (必需): 指定要提取的题目类型。可选值为 `TrueorFalse`, `Choice`, `FillBlank`。
- `-o` 或 `--output` (可选): 指定输出的JSON文件名。默认为 `extracted_questions.json`。

**命令行示例:**

```bash
# 提取选择题
python convert_PTA_to_problemx.py pta_export.html -type Choice -o ChoiceQuestions.json

# 提取判断题
python convert_PTA_to_problemx.py pta_export.html -type TrueorFalse -o TrueFalseQuestions.json

# 提取填空题
python convert_PTA_to_problemx.py pta_export.html -type FillBlank -o FillBlankQuestions.json
```

## 6. 重要注意事项

1.  **HTML文件编码**: 请确保您从PTA导出的HTML文件采用 **UTF-8** 编码，以避免乱码问题。
2.  **文件路径**: 强烈建议在命令行中使用**绝对路径**指定输入和输出文件/目录，以避免因相对路径引发的错误。
3.  **HTML结构依赖**: 转换工具的解析效果高度依赖于PTA导出HTML的结构。如果PTA的HTML结构发生较大变化，可能需要更新转换脚本的解析逻辑。