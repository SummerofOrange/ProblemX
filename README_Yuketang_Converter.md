# ProblemX 雨课堂题库转换工具使用指南

## 1. 概述

本工具 (`convert_Yuketang_to_problemx.py`) 旨在帮助用户将从**雨课堂**导出的HTML格式题库文件，转换为 **ProblemX** 刷题系统所支持的JSON格式。通过此工具，您可以方便地将雨课堂的题目资源导入到ProblemX中进行练习和管理。

## 2. 相关文件

项目中与转换工具相关的文件包括：

- `convert_Yuketang_to_problemx.py`: **主转换脚本**，执行题库格式转换的核心逻辑。
- `test_html_converter.py`: 用于测试转换脚本功能的脚本。
- `simple_test.py`: 一个简化的测试脚本，可能用于快速验证或调试特定功能。
- `README_Yuketang_Converter.md`: 即本文档，提供详细的使用说明。

## 3. 支持的题型

目前，本转换工具支持以下常见的雨课堂题型：

- **选择题 (Choice)**:
    - 自动识别并提取题干和A、B、C、D四个选项。
    - 能够根据HTML中的特定标记（通常是 `is-checked` 类）自动判断并提取正确答案。
    - 支持处理题目文本中包含复杂HTML格式的情况（例如，通过 `custom_ueditor_cn_body` 类包裹的内容）。
- **填空题 (FillBlank)**:
    - 支持单空和多空填空题。
    - 能够自动识别题目中的填空数量，并提取对应的答案。

## 4. 输入HTML文件格式要求

为了确保转换工具能够正确解析，输入的雨课堂HTML文件应大致遵循以下结构：

```html
<!-- 题目标题/题干 -->
<h4 class="clearfix exam-font">这里是题目的具体内容...</h4>

<!-- 选项列表 (适用于选择题) -->
<ul class="list-unstyled list-unstyled-radio exam-font">
  <li>
    <label class="el-radio is-disabled">
      <span class="radioInput">A</span>
      <span class="radioText">选项A的描述文字</span>
    </label>
  </li>
  <!-- 正确答案的 <label> 标签通常会包含 is-checked 类 -->
  <li>
    <label class="el-radio is-disabled is-checked">
      <span class="radioInput">B</span>
      <span class="radioText">选项B的描述文字 (正确答案)</span>
    </label>
  </li>
  <!-- 其他选项 C, D -->
</ul>

<!-- 填空题的答案区域可能结构不同，工具会尝试解析 -->
```

**注意**：HTML结构可能因雨课堂版本更新而略有差异，工具的解析逻辑基于常见的HTML模式。

## 5. 输出JSON文件格式

转换成功后，输出的JSON文件将严格遵循ProblemX系统的题库标准。具体格式请参考项目根目录下的 `Readme_ProblemBank.md` 文件。

以下为转换后JSON文件内容的示例：

### 5.1 选择题 (Choice) 示例

```json
{
  "data": [
    {
      "type": "Choice",
      "question": "对于一台PC而言，下列各项中（ ）对系统必不可少。",
      "choices": [
        "A.Office软件",
        "B.杀毒软件",
        "C.OS",
        "D.编译软件"
      ],
      "answer": "C"
    }
    // ...更多选择题
  ]
}
```

### 5.2 填空题 (FillBlank) 示例

```json
{
  "data": [
    {
      "type": "FillBlank",
      "question": "OS负责管理和控制计算机系统的______资源。",
      "BlankNum": 1,
      "answer": ["软硬件"]
    },
    {
      "type": "FillBlank",
      "question": "现代OS的基本特性是并发性、共享性、______和异步性。",
      "BlankNum": 1,
      "answer": ["虚拟性"]
    }
    // ...更多填空题
  ]
}
```

## 6. 使用方法

### 6.1 通过命令行运行

推荐使用命令行方式运行转换脚本，具有较高的灵活性。

```bash
python convert_Yuketang_to_problemx.py -i <input_html_path> -o <output_directory_path> -name <output_filename_prefix>
```

**参数详解:**

- `-i` 或 `--input` (必需): 指定输入的雨课堂HTML文件的**完整路径**。
  - *示例*: `C:\Users\YourName\Downloads\Yuketang_Chapter1.html`
- `-o` 或 `--output` (必需): 指定转换后的JSON文件存放的**目录路径**。脚本会自动在此目录下创建 `Choice` 和 `FillBlank` 子目录来分别存放不同类型的题目。
  - *示例*: `D:\ProblemX_Subjects\ComputerBasics`
- `-name` (必需): 指定输出JSON文件的**前缀名**。这有助于组织不同单元或章节的题库。
  - *示例*: 如果设置为 `Unit1`，则选择题文件将命名为 `Unit1_Choice.json`，填空题文件为 `Unit1_FillBlank.json`。

**命令行示例:**

```bash
python convert_Yuketang_to_problemx.py -i Subject/OS/1.html -o Subject/OS_Converted -name U1
```

上述命令将会：
1. 读取位于 `Subject/OS/` 目录下的 `1.html` 文件。
2. 将解析出的选择题保存到 `Subject/OS_Converted/Choice/U1_Choice.json`。
3. 将解析出的填空题保存到 `Subject/OS_Converted/FillBlank/U1_FillBlank.json`。

### 6.2 在Python代码中集成 (高级)

如果您需要在其他Python项目中集成此转换功能，可以导入并使用 `HTMLToProblemXConverter` 类。

```python
from convert_Yuketang_to_problemx import HTMLToProblemXConverter

# 1. 创建转换器实例
converter = HTMLToProblemXConverter()

# 2. 解析HTML文件 (需要您自行提供文件路径)
# html_file_path = "path/to/your/yuketan_exam.html"
# parsed_questions = converter.parse_html_file(html_file_path) # 具体方法名请参照脚本实现

# 3. 保存转换结果 (需要您自行指定输出目录和文件名前缀)
# output_dir = "output_directory"
# filename_prefix = "Chapter1_Converted"
# converter.save_to_json(output_dir, filename_prefix) # 具体方法名请参照脚本实现
```

**注意**: 直接在代码中使用时，您可能需要根据 `convert_Yuketang_to_problemx.py` 脚本中的具体实现来调整文件处理和保存逻辑。

### 6.3 运行测试脚本

项目提供了测试脚本，方便开发者验证转换工具的正确性：

- **完整功能测试**: `test_html_converter.py`
  ```bash
  python test_html_converter.py
  ```
- **简化调试测试**: `simple_test.py` (通常会提供更详细的解析过程输出，便于定位问题)
  ```bash
  python simple_test.py
  ```

## 7. 输出目录结构

成功执行转换后，指定的输出目录 (`-o` 参数指定的路径) 将会形成如下结构：

```
<output_directory_path>/  (例如: Subject/OS_Converted/)
├── Choice/
│   └── <output_filename_prefix>_Choice.json  (例如: U1_Choice.json)
└── FillBlank/
    └── <output_filename_prefix>_FillBlank.json (例如: U1_FillBlank.json)
```

如果未来支持判断题，则可能还会包含 `TrueorFalse` 目录。

## 8. 核心转换特性

- **智能HTML解析**:
    - 能够自动区分选择题和填空题。
    - 采用健壮的解析逻辑，尝试处理雨课堂HTML中可能存在的复杂或不规则结构。
    - 特别优化了对包含 `custom_ueditor_cn_body` 类的富文本内容的提取。
- **选择题处理**:
    - 精确提取题干和A、B、C、D四个标准选项的文本内容。
    - 依赖HTML中的 `is-checked` 类来准确识别正确答案。
    - 输出的选项格式统一为：`"A.选项内容"`, `"B.选项内容"` 等。
- **填空题处理**:
    - 能够识别题目中的填空位置（通常基于 `blank-item-dynamic` 类）。
    - 将HTML中的填空标记统一替换为ProblemX标准的 `______` 占位符。
    - 准确提取标准答案，并能处理一个空有多个可接受答案的情况（例如，`"答案1 / 答案2"`）。
    - 自动计算并记录每个填空题的空格数量 (`BlankNum` 字段)。
- **文本清理与规范化**:
    - 自动移除题目和选项文本中的多余HTML标签。
    - 清理不必要的空白字符（如多余的空格、换行符）。
    -妥善处理常见的HTML实体编码（例如 `&nbsp;` 会被转换为空格）。
    - 确保输出的文本内容干净、可读。
- **错误处理与报告**:
    - 对于无法成功解析的个别题目，脚本会选择跳过，并打印错误信息，以保证其余题目能够继续转换。
    - 在转换结束后，会提供详细的统计信息，包括成功转换的各类题型数量。

## 9. 重要注意事项

1.  **HTML文件编码**: 请确保您从雨课堂下载的HTML文件采用 **UTF-8** 编码，以避免乱码问题。
2.  **文件路径**: 强烈建议在命令行中使用**绝对路径**指定输入和输出文件/目录，以避免因相对路径引发的错误。
3.  **HTML结构依赖**: 转换工具的解析效果高度依赖于雨课堂导出HTML的结构。如果雨课堂的HTML结构发生较大变化，可能需要更新转换脚本的解析逻辑。
4.  **选择题选项完整性**: 为保证正确解析，每道选择题应包含完整的A、B、C、D四个选项。
5.  **正确答案标记**: 选择题的正确答案必须在HTML中通过 `is-checked` 类进行明确标记。

## 10. 错误处理机制

-   **跳过机制**: 当遇到无法解析的题目时，脚本会打印相关错误提示，并跳过该题目，继续处理后续题目。
-   **控制台输出**: 解析过程中的详细信息（包括错误和警告）会直接输出到控制台，方便用户追踪问题。
-   **最终统计**: 无论过程中是否发生错误，脚本执行完毕后都会显示成功转换的题目数量统计。

## 11. 输出统计信息示例

转换任务完成后，您将在控制台看到类似以下的统计信息：

```
=== 转换统计 ===
选择题: 21 道
填空题: 7 道
总计: 28 道

=== 转换成功完成 ===
HTML文件: c:/Users/Admin/Desktop/Qt_project/ProblemX/Subject/OS/1.html
输出目录: c:/Users/Admin/Desktop/Qt_project/ProblemX/Subject/OS_Converted
选择题成功转换: 21 道
填空题成功转换: 7 道
```

## 12. 完整运行示例输出

以下是一个包含22道选择题的HTML文件进行转换时的典型控制台输出：

```
=== HTML题库转换测试 ===
正在解析HTML文件: c:/Users/Admin/Desktop/Qt_project/ProblemX/Subject/OS/1.html
找到 22 道题目
✓ 成功解析第1题 (选择题)
✓ 成功解析第2题 (选择题)
...
✓ 成功解析第22题 (选择题)

=== 转换统计 ===
选择题: 22
填空题: 0
判断题: 0  (若当前版本不支持判断题，则为0)
总计: 22

转换结果已保存到: c:/Users/Admin/Desktop/Qt_project/ProblemX/HTML_Converted
```

## 13. 技术实现细节

-   **主要解析手段**: 脚本主要依赖**正则表达式 (RegEx)** 来匹配和提取HTML中的题目信息。这使得脚本在处理特定结构的HTML时较为高效，但也意味着对HTML结构变化的敏感度较高。
-   **复杂内容处理**: 针对雨课堂中可能存在的嵌套HTML标签或富文本编辑器（如UEditor）生成的内容，脚本进行了一定的优化处理。
-   **特殊字符与格式化**: 自动处理HTML实体编码，并进行基本的文本格式规范化。
-   **输出标准**: 最终输出的JSON格式严格遵守ProblemX题库的数据标准。

---

希望这份更详细的文档能帮助您更好地理解和使用ProblemX雨课堂题库转换工具！