# ProblemX 雨课堂题库转换工具使用指南

**这是一个 Python 脚本，用于从学习通导出的 HTML 文件中提取选择题、判断题和填空题，并将它们保存为 ProblemX 支持的 JSON 题库格式。**

## 功能

* **从 HTML 文件中提取题目信息。**
* **支持三种题型：**
  * **选择题 (Choice)**
  * **判断题 (TrueorFalse)**
  * **填空题 (FillBlank)**
* **将提取的题目以 JSON 格式输出到文件。**
* **支持通过命令行参数指定输入文件、输出文件和题目类型。**

## 依赖

* **Python 3**
* **Beautiful Soup 4**

**在运行脚本之前，请确保已安装 **`Beautiful Soup`：

```
pip install beautifulsoup4
```

## 使用方法

**通过命令行运行此脚本。**

### 基本命令格式

```
python extract_xuexitong_questions.py <input_html_file> --type <question_type> [-o <output_json_file>]
```

### 参数说明

* `file`: **必需**。包含题目的 HTML 文件的路径。
* `--type`: **必需**。要提取的题目类型。可选值为：
  * `Choice` (选择题)
  * `TrueorFalse` (判断题)
  * `FillBlank` (填空题)
* `-o, --output`: **可选**。指定输出的 JSON 文件名。如果未提供，脚本将根据题目类型自动生成一个文件名（例如，`extracted_Choice_questions.json`）。

### 示例

* **提取选择题**：
  ```
  python extract_xuexitong_questions.py choice.html --type Choice -o extracted_choice_questions.json
  ```
* **提取判断题**：
  ```
  python extract_xuexitong_questions.py trueoffalse.html --type TrueorFalse -o extracted_true_false_questions.json
  ```
* **提取填空题**：
  ```
  python extract_xuexitong_questions.py FillBlank.html --type FillBlank -o extracted_fill_blank_questions.json
  ```

## 输出格式

**脚本会将提取的题目保存为一个 JSON 文件，其结构如下：**

```
{
    "data": [
        {
            "type": "...",
            "question": "...",
            // ... 其他字段，取决于题目类型
        }
    ]
}
```
