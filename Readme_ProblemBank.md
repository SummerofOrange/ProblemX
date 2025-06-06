# ProblemX 题库配置指南

本指南详细说明了 ProblemX 刷题系统所支持的题库目录结构和JSON文件格式规范。请按照以下说明配置您的题库，以确保程序能够正确加载和解析题目。

## 1. 题库目录结构

ProblemX 程序通过扫描指定的科目文件夹来加载题库。标准的目录结构如下：

```
<科目名称>/<题型名称>/<题库文件名>.json
```

**说明:**

- `<科目名称>`: 代表具体的学科，例如 `DataStructure`, `C++`, `OS` 等。文件夹名称可以自定义。
- `<题型名称>`: 代表该科目下的题目类型，ProblemX目前支持以下固定的题型文件夹名称：
  - `Choice`: 用于存放选择题。
  - `MultiChoice`: 用于存放多选题。
  - `FillBlank`: 用于存放填空题。
  - `TrueorFalse`: 用于存放判断题。
- `<题库文件名>.json`: 具体的题库文件，文件名可以自定义，但必须是以 `.json` 结尾的JSON文件。

**示例:**

假设您的ProblemX项目位于 `C:\Users\Admin\Desktop\Qt_project\ProblemX`，那么一个数据结构科目的选择题题库文件路径可能如下：

```
C:\Users\Admin\Desktop\Qt_project\ProblemX\Subject\DataStructure\Choice\Chapter1_Introduction.json
```

### 1.1 图片资源 (Asset) 目录 (可选)

如果题目中包含图片，建议在相应的 `<题型名称>` 目录下创建一个名为 `Asset` (或其他自定义名称，但需与JSON文件中 `image` 字段路径对应) 的子目录，用于存放图片资源。

**示例目录结构 (以数据结构科目为例):**

```
C:\USERS\ADMIN\DESKTOP\QT_PROJECT\PROBLEMX\SUBJECT\DATASTRUCTURE
├── Choice
│   ├── Chapter1_Choice.json
│   ├── Chapter2_Choice.json
│   └── Asset
│       ├── chapter1
│       │   └── image1.png
│       └── chapter2
│           ├── figure_A.jpg
│           └── figure_B.png
├── FillBlank
│   ├── Chapter1_FillBlank.json
│   └── Asset
│       └── chapter1
│           └── diagram1.png
└── TrueorFalse
    └── Chapter1_TrueorFalse.json
```

## 2. 题库JSON文件格式规范

每个 `.json` 题库文件都必须遵循以下格式：

1. **顶层结构**: JSON文件必须是一个**对象 (Object)**。
2. **数据容器**: 该顶层对象内必须包含一个名为 `"data"` 的**数组 (Array)**。这个数组的每个元素代表一道独立的题目。

### 2.1 题目 (Item) 通用字段

数组 `"data"` 中的每一个题目对象都应包含以下基本字段：

- `"type"` (String): **必需**。表示题目的类型。其值必须与该题库所在的 `<题型名称>` 文件夹名称**完全一致** (区分大小写)。
  - 例如，在 `Choice` 文件夹下的题库，题目类型应为 `"Choice"`。
- `"question"` (String): **必需**。表示题目的题干内容。可以使用 `\n` 来表示换行。
  - **Markdown支持**: 题目内容支持Markdown格式，包括：
    - **粗体文本**: `**粗体**`
    - *斜体文本*: `*斜体*`
    - `代码`: `` `代码` ``
    - 代码块: `` ```语言\n代码内容\n``` ``
    - 列表: `- 项目` 或 `1. 项目`
    - 支持更多markdown语法，包括表格等
  - **LaTeX数学公式支持**: 支持使用KaTeX引擎渲染数学公式：
    - 行内公式: `$公式内容$`
    - 块级公式: `$$公式内容$$`
    - 示例: `$E = mc^2$` 或 `$$\\int_{-\\infty}^{\\infty} e^{-x^2} dx = \\sqrt{\\pi}$$`
- `"image"` (String, 可选): 如果题目包含图片，此字段表示图片的**相对路径**。路径的基准点是**当前JSON题库文件所在的目录**。
  - 例如，如果题库文件是 `Subject/DataStructure/Choice/Chapter1_Choice.json`，图片位于 `Subject/DataStructure/Choice/Asset/chapter1/image1.png`，则 `"image"` 字段应填入 `"Asset/chapter1/image1.png"`。

### 2.2 选择题 (Choice) 格式

当 `"type": "Choice"` 时，题目对象还需要包含以下字段：

- `"choices"` (Array of Strings): **必需**。一个包含四个字符串元素的数组，**严格按照A、B、C、D的顺序**排列，每个字符串代表一个选项的描述。
  - 选项文本可以包含换行符 `\n`。
- `"answer"` (String): **必需**。表示该选择题的正确答案，其值为选项的字母标识（例如：`"A"`, `"B"`, `"C"`, `"D"`）。

**选择题示例:**

```json
{
    "type": "Choice",
    "question": "下面代码段的时间复杂度是（）。\nfor (i=0; i<n; i++)\n    for (j=0; j<m; j++)\n        a[i][j]=0;",
    "choices": [
        "A. O(1)",
        "B. O(m*n)",
        "C. O(m^2)",
        "D. O(n^2)"
    ],
    "answer": "B",
    "image": "Asset/complexity/figure1.png" // 可选的图片路径
}
```

**包含Markdown和LaTeX的选择题示例:**

```json
{
    "type": "Choice",
    "question": "已知函数 $f(x) = x^2 + 2x + 1$，求 $f'(x)$ 的值。\n\n**提示**: 使用求导公式 $(x^n)' = nx^{n-1}$\n\n```python\ndef derivative(x):\n    return 2*x + 2\n```",
    "choices": [
        "A. $2x + 1$",
        "B. $2x + 2$",
        "C. $x^2 + 2$",
        "D. $2x^2 + 2x$"
    ],
    "answer": "B"
}
```

### 2.3 多选题 (MultipleChoice) 格式

当 `"type": "MultipleChoice"` 时，题目对象还需要包含以下字段：

- `"choices"` (Array of Strings): **必需**。一个包含四个字符串元素的数组，**严格按照A、B、C、D的顺序**排列，每个字符串代表一个选项的描述。
  - 选项文本可以包含换行符 `\n`。
- `"answer"` (String): **必需**。表示该多选题的正确答案，其值为选项的字母标识组合，**多个答案之间用逗号分隔**（例如：`"A,B,D"`、`"A,C"`、`"B,C,D"`）。

**多选题示例:**

```json
{
    "type": "MultipleChoice",
    "question": "下列关于数据结构的说法正确的是（）。",
    "choices": [
        "A. 栈是一种先进先出的数据结构",
        "B. 队列是一种先进先出的数据结构",
        "C. 链表可以随机访问元素",
        "D. 数组支持动态扩容"
    ],
    "answer": "B"
}
```

**包含多个正确答案的多选题示例:**

```json
{
    "type": "MultipleChoice",
    "question": "以下哪些是面向对象编程的特征？",
    "choices": [
        "A. 封装性",
        "B. 继承性",
        "C. 结构化",
        "D. 多态性"
    ],
    "answer": "A,B,D"
}
```

### 2.4 判断题 (TrueorFalse) 格式

当 `"type": "TrueorFalse"` 时，题目对象还需要包含以下字段：

- `"answer"` (String): **必需**。表示该判断题的正确答案。其值必须是：
  - `"T"` (代表正确/True)
  - `"F"` (代表错误/False)

**判断题示例:**

```json
{
    "type": "TrueorFalse",
    "question": "数据项是数据的最小单位。",
    "answer": "T"
}
```

### 2.5 填空题 (FillBlank) 格式

当 `"type": "FillBlank"` 时，题目对象还需要包含以下字段：

- `"BlankNum"` (Integer): **必需**。表示该填空题包含的**填空数量**。
- `"answer"` (Array of Strings): **必需**。一个字符串数组，其元素数量必须与 `"BlankNum"` 的值**一致**。数组中的每个字符串**按顺序**对应题目中的每一个空的正确答案。

**填空题示例:**

```json
{
    "type": "FillBlank",
    "question": "基本术语\n(   ) 是对特定问题求解步骤的一种描述，它是指令的有限序列，其中每一条指令表示一个或多个操作。\n计算机的五大基本组成部分是：控制器、(   )、存储器、输入设备和 (   )。",
    "BlankNum": 3,
    "answer": [
        "算法",
        "运算器",
        "输出设备"
    ]
}
```

## 3. 完整题库JSON文件示例

以下是一个包含多种题型的完整题库JSON文件 (`example_bank.json`) 的片段：

```json
{
    "data": [
        {
            "type": "Choice",
            "question": "下列运算符中，（ ）运算符不能重载。",
            "choices": [
                "A. &&",
                "B. []",
                "C. ::",
                "D. <<"
            ],
            "answer": "C"
        },
        {
            "type": "TrueorFalse",
            "question": "在C++中，构造函数可以被继承。",
            "answer": "F"
        },
        {
            "type": "FillBlank",
            "question": "面向对象程序设计的三大特征是封装性、继承性和 (   )。",
            "BlankNum": 1,
            "answer": [
                "多态性"
            ]
        },
        {
            "type": "FillBlank",
            "question": "# 编程算法填空题\n\n## 快速排序算法\n\n```python\ndef quicksort(arr, low, high):\n    if low < high:\n        # 分区操作\n        pi = partition(arr, low, high)\n        \n        # 递归排序左右子数组\n        quicksort(arr, low, ________)  # 填空1\n        quicksort(________, pi + 1, high)  # 填空2\n\ndef partition(arr, low, high):\n    pivot = arr[high]  # 选择最后一个元素作为基准\n    i = low - 1\n    \n    for j in range(low, high):\n        if arr[j] <= pivot:\n            i += 1\n            arr[i], arr[j] = arr[j], arr[i]\n    \n    arr[i + 1], arr[high] = arr[high], arr[i + 1]\n    return ________  # 填空3\n```\n\n## 时间复杂度分析\n\n- **最好情况**：$O(n \\log n)$\n- **平均情况**：$O(n \\log n)$\n- **最坏情况**：$O(n^2)$\n\n### 空间复杂度\n\n递归调用栈的深度为 $O(\\log n)$（平均情况）到 $O(n)$（最坏情况）。\n\n**填空说明**：\n1. 填空1：递归排序左子数组的结束位置\n2. 填空2：递归排序右子数组时传入的数组参数\n3. 填空3：分区函数应该返回的值",
            "BlankNum": 3,
            "answer": [
                "pi - 1",
                "arr",
                "i + 1"
            ]
        },
        {
            "type": "FillBlank",
            "question": "# 线性代数矩阵运算\n\n## 矩阵乘法\n\n给定两个矩阵：\n\n$$A = \\begin{pmatrix}\n1 & 2 \\\\\n3 & 4\n\\end{pmatrix}, \\quad B = \\begin{pmatrix}\n5 & 6 \\\\\n7 & 8\n\\end{pmatrix}$$\n\n计算矩阵乘积 $AB$：\n\n$$AB = \\begin{pmatrix}\n1 \\cdot 5 + 2 \\cdot 7 & 1 \\cdot 6 + 2 \\cdot 8 \\\\\n3 \\cdot 5 + 4 \\cdot 7 & 3 \\cdot 6 + 4 \\cdot 8\n\\end{pmatrix} = \\begin{pmatrix}\n(1) & (2) \\\\\n(3) & (4)\n\\end{pmatrix}$$\n\n## 行列式计算\n\n对于 2×2 矩阵 $A$，其行列式为：\n\n$$\\det(A) = \\begin{vmatrix}\n1 & 2 \\\\\n3 & 4\n\\end{vmatrix} = 1 \\cdot 4 - 2 \\cdot 3 = (5)$$\n\n> **提示**：矩阵乘法的计算规则是行乘列",
            "BlankNum": 5,
            "answer": [
                "19",
                "22",
                "43",
                "50",
                "-2"
            ]
        },
        {
            "type": "Choice",
            "question": "# Markdown语法测试\n\n## 文本格式\n\n这是一个包含**粗体**、*斜体*、***粗斜体***和~~删除线~~的测试题目。\n\n### 列表测试\n\n无序列表：\n- 第一项\n- 第二项\n  - 嵌套项\n- 第三项\n\n有序列表：\n1. 第一步\n2. 第二步\n3. 第三步\n\n### 代码测试\n\n行内代码：`console.log('Hello World')`\n\n代码块：\n```javascript\nfunction fibonacci(n) {\n    if (n <= 1) return n;\n    return fibonacci(n-1) + fibonacci(n-2);\n}\n```\n\n### 引用和链接\n\n> 这是一个引用块\n> 可以包含多行内容\n\n链接测试：[GitHub](https://github.com)\n\n---\n\n下面是数学公式：$f(x) = x^2 + 2x + 1$",
            "choices": [
                "A. 这道题测试**粗体**格式",
                "B. 这道题测试*斜体*格式",
                "C. 这道题测试`代码`格式",
                "D. 这道题测试所有格式"
            ],
            "answer": "D"
        },
        {
            "type": "Choice",
            "question": "# 表格和复杂数学公式测试\n\n## 数据结构比较\n\n| 数据结构 | 插入时间复杂度 | 查找时间复杂度 | 删除时间复杂度 |\n|---------|---------------|---------------|---------------|\n| 数组 | $O(n)$ | $O(1)$ | $O(n)$ |\n| 链表 | $O(1)$ | $O(n)$ | $O(1)$ |\n| 哈希表 | $O(1)$ | $O(1)$ | $O(1)$ |\n| 二叉搜索树 | $O(\\log n)$ | $O(\\log n)$ | $O(\\log n)$ |\n\n## 复杂数学公式\n\n积分公式：\n$$\\int_{-\\infty}^{\\infty} e^{-x^2} dx = \\sqrt{\\pi}$$\n\n矩阵运算：\n$$\\begin{pmatrix}\na & b \\\\\nc & d\n\\end{pmatrix}\n\\begin{pmatrix}\nx \\\\\ny\n\\end{pmatrix} = \n\\begin{pmatrix}\nax + by \\\\\ncx + dy\n\\end{pmatrix}$$\n\n求和公式：\n$$\\sum_{k=1}^{n} k^2 = \\frac{n(n+1)(2n+1)}{6}$$",
            "choices": [
                "A. 哈希表的平均查找时间复杂度是 $O(1)$",
                "B. 二叉搜索树的最坏查找时间复杂度是 $O(n)$",
                "C. 数组的插入时间复杂度总是 $O(n)$",
                "D. 以上都正确"
            ],
            "answer": "D"
        }
        // ...更多题目
    ]
}
```

---

## 4. Markdown和LaTeX功能使用注意事项

### 4.1 Markdown格式注意事项

- 在JSON字符串中使用Markdown时，需要注意转义字符的使用
- 换行符使用 `\n` 表示
- 代码块中的反引号需要适当转义
- 建议在编写复杂Markdown内容前先在Markdown编辑器中测试

### 4.2 LaTeX数学公式注意事项

- 使用 `$...$` 表示行内公式，`$$...$$` 表示块级公式
- 在JSON字符串中，反斜杠需要双重转义，例如 `\\frac{1}{2}` 表示 `\frac{1}{2}`
- 支持大部分KaTeX语法，包括分数、积分、求和、矩阵等
- 建议参考 [KaTeX支持的函数列表](https://katex.org/docs/supported.html)

### 4.3 自动适配大小功能

- **智能高度调整**：题目和选项渲染器会根据内容长度自动调整显示高度
- **窗口适配**：自动适配功能确保内容不会超出主窗口的显示范围
- **最佳体验**：长题目和复杂公式能够完整显示，短题目则紧凑显示，提供最佳的视觉体验
- **性能优化**：使用异步JavaScript计算内容高度，不影响界面响应性能

### 4.4 兼容性说明

- Markdown和LaTeX功能需要Qt WebEngine模块支持
- 旧版本的题库文件完全兼容，无需修改
- 新功能为可选功能，不使用时不影响题目正常显示
- 自动适配功能默认启用，可通过代码配置关闭

---

请严格按照本指南配置您的题库，以确保ProblemX能够顺利加载和运行。如有疑问，请参考项目中的示例题库或联系开发者。
