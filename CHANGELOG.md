# 更新日志

## Beta 0.3 (latest)

  - **新增Markdown渲染支持**：集成Qt WebEngine模块，支持在题目中使用Markdown格式，包括粗体、斜体、代码块、列表等格式
  - **新增LaTeX数学公式支持**：集成KaTeX引擎，支持在题目中渲染LaTeX数学公式，提升数学类题目的显示效果
  - **优化题目显示**：通过`MarkdownRenderer`类提供更好的题目内容渲染体验
  - **改进依赖管理**：添加Qt WebEngine模块依赖，确保Markdown和LaTeX功能正常运行

## Beta 0.2(2025/05/31)

  - 修复了练习模块 (`practicewidget.cpp`) 和错题复习模块 (`reviewwidget.cpp`) 中，当题目文本（如代码片段）包含特殊HTML字符（例如 `<`, `>`）时，可能导致的题目显示不完整或渲染异常的问题。通过在 `updateQuestionDisplay` 和 `displayWrongAnswerDetails` 方法中对这些特殊字符进行转义，确保了题目内容的正确渲染。