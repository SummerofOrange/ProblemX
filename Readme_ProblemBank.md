# ProblemX 题库配置指南

本指南详细说明了 ProblemX 刷题系统所支持的题库目录结构和JSON文件格式规范。请按照以下说明配置您的题库，以确保程序能够正确加载和解析题目。

## 1. 题库目录结构

ProblemX 程序通过扫描指定的科目文件夹来加载题库。标准的目录结构如下：

```
<科目名称>/<题型名称>/<题库文件名>.json
```

**说明:**

-   `<科目名称>`: 代表具体的学科，例如 `DataStructure`, `C++`, `OS` 等。文件夹名称可以自定义。
-   `<题型名称>`: 代表该科目下的题目类型，ProblemX目前支持以下固定的题型文件夹名称：
    -   `Choice`: 用于存放选择题。
    -   `FillBlank`: 用于存放填空题。
    -   `TrueorFalse`: 用于存放判断题。
-   `<题库文件名>.json`: 具体的题库文件，文件名可以自定义，但必须是以 `.json` 结尾的JSON文件。

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

1.  **顶层结构**: JSON文件必须是一个**对象 (Object)**。
2.  **数据容器**: 该顶层对象内必须包含一个名为 `"data"` 的**数组 (Array)**。这个数组的每个元素代表一道独立的题目。

### 2.1 题目 (Item) 通用字段

数组 `"data"` 中的每一个题目对象都应包含以下基本字段：

-   `"type"` (String): **必需**。表示题目的类型。其值必须与该题库所在的 `<题型名称>` 文件夹名称**完全一致** (区分大小写)。
    -   例如，在 `Choice` 文件夹下的题库，题目类型应为 `"Choice"`。
-   `"question"` (String): **必需**。表示题目的题干内容。可以使用 `\n` 来表示换行。
-   `"image"` (String, 可选): 如果题目包含图片，此字段表示图片的**相对路径**。路径的基准点是**当前JSON题库文件所在的目录**。
    -   例如，如果题库文件是 `Subject/DataStructure/Choice/Chapter1_Choice.json`，图片位于 `Subject/DataStructure/Choice/Asset/chapter1/image1.png`，则 `"image"` 字段应填入 `"Asset/chapter1/image1.png"`。

### 2.2 选择题 (Choice) 格式

当 `"type": "Choice"` 时，题目对象还需要包含以下字段：

-   `"choices"` (Array of Strings): **必需**。一个包含四个字符串元素的数组，**严格按照A、B、C、D的顺序**排列，每个字符串代表一个选项的描述。
    -   选项文本可以包含换行符 `\n`。
-   `"answer"` (String): **必需**。表示该选择题的正确答案，其值为选项的字母标识（例如：`"A"`, `"B"`, `"C"`, `"D"`）。

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

### 2.3 判断题 (TrueorFalse) 格式

当 `"type": "TrueorFalse"` 时，题目对象还需要包含以下字段：

-   `"answer"` (String): **必需**。表示该判断题的正确答案。其值必须是：
    -   `"T"` (代表正确/True)
    -   `"F"` (代表错误/False)

**判断题示例:**

```json
{
    "type": "TrueorFalse",
    "question": "数据项是数据的最小单位。",
    "answer": "T"
}
```

### 2.4 填空题 (FillBlank) 格式

当 `"type": "FillBlank"` 时，题目对象还需要包含以下字段：

-   `"BlankNum"` (Integer): **必需**。表示该填空题包含的**填空数量**。
-   `"answer"` (Array of Strings): **必需**。一个字符串数组，其元素数量必须与 `"BlankNum"` 的值**一致**。数组中的每个字符串**按顺序**对应题目中的每一个空的正确答案。

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
        }
        // ...更多题目
    ]
}
```

---

请严格按照本指南配置您的题库，以确保ProblemX能够顺利加载和运行。如有疑问，请参考项目中的示例题库或联系开发者。

