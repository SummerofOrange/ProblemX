# 更新日志

## Beta 0.2(2025/05/31)
  - 修复了练习模块 (`practicewidget.cpp`) 和错题复习模块 (`reviewwidget.cpp`) 中，当题目文本（如代码片段）包含特殊HTML字符（例如 `<`, `>`）时，可能导致的题目显示不完整或渲染异常的问题。通过在 `updateQuestionDisplay` 和 `displayWrongAnswerDetails` 方法中对这些特殊字符进行转义，确保了题目内容的正确渲染。