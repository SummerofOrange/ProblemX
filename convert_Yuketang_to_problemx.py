#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
HTML题库转换为ProblemX格式的转换工具

支持将网页下载的HTML格式题库转换为ProblemX支持的JSON格式
支持选择题和填空题的转换
"""

import json
import re
import os
import argparse # 导入 argparse 模块
from html.parser import HTMLParser
from typing import List, Dict, Any

class HTMLQuestionParser:
    def __init__(self):
        self.questions = []
    
    def parse(self, html_content):
        """解析HTML内容，提取题目信息"""
        # 解析选择题
        self.parse_choice_questions(html_content)
        # 解析填空题
        self.parse_fillblank_questions(html_content)
        
        return self.questions
    
    def parse_choice_questions(self, html_content):
        """解析选择题"""
        # 使用正则表达式提取选择题题目块
        # 查找包含"单选题"的题目
        choice_pattern = (
            r'<div[^>]*class\s*=\s*"[^"]*?\bsubject-item\b[^"]*?"[^>]*>\s*'
            r'<div[^>]*class\s*=\s*"[^"]*?\bresult_item\b[^"]*?"[^>]*>\s*'
            r'<div[^>]*class\s*=\s*"[^"]*?\bitem-type\b[^"]*?"[^>]*>\s*\d+\.单选题(?:.(?!<\/div>))*?.<\/div>\s*'
            r'.*?<div[^>]*class\s*=\s*"[^"]*?\bitem-body\b[^"]*?"[^>]*>\s*'
            r'.*?<h4[^>]*class\s*=\s*"[^"]*?\bexam-font\b[^"]*?"[^>]*>(.*?)<\/h4>\s*'
            r'.*?<ul[^>]*class\s*=\s*"[^"]*?\blist-unstyled-radio\b[^"]*?"[^>]*>(.*?)<\/ul>\s*'
            r'.*?<\/div>\s*'  # Closes item-body
            r'.*?<div[^>]*class\s*=\s*"[^"]*?\bitem-footer\b[^"]*?"[^>]*>(?:.(?!<\/div>))*?.<\/div>\s*'
            r'.*?<\/div>\s*'  # Closes result_item
            r'.*?<\/div>'  # Closes subject-item
        )
        question_matches = re.findall(choice_pattern, html_content, re.DOTALL)
        print(f"DEBUG: HTMLQuestionParser - Found {len(question_matches)} choice question matches.") # Added debug print
        
        for question_html, choices_html in question_matches:
            question_text = self.extract_question_text(question_html)
            if not question_text:
                continue
                
            choices, correct_answer = self.extract_choices_and_answer(choices_html)
            
            if choices and correct_answer:
                question_data = {
                    'type': 'Choice',
                    'question': question_text,
                    'choices': choices,
                    'answer': correct_answer
                }
                self.questions.append(question_data)
    
    def parse_fillblank_questions(self, html_content):
        """解析填空题"""
        # 提取填空题块：从题目类型到下一个题目或结束
        fillblank_pattern = r'<div class="subject-item"><div[^>]*class="result_item"[^>]*><div[^>]*class="item-type"[^>]*>\s*\d+\.填空题.*?</div>.*?<div[^>]*class="item-body"[^>]*>(.*?)</div>.*?正确答案：.*?<ul[^>]*class="list-unstyled problem-options"[^>]*>(.*?)</ul>.*?</div>.*?</div>.*?</div>'
        fillblank_matches = re.findall(fillblank_pattern, html_content, re.DOTALL)
        
        for question_body, answer_section in fillblank_matches:
            question_text = self.extract_fillblank_question_text(question_body)
            if not question_text:
                continue
                
            answers = self.extract_fillblank_answers(answer_section)
            
            if answers:
                question_data = {
                    'type': 'FillBlank',
                    'question': question_text,
                    'answer': answers
                }
                self.questions.append(question_data)
    
    def extract_question_text(self, question_html):
        """提取题目文本"""
        # 检查是否包含custom_ueditor_cn_body
        if 'custom_ueditor_cn_body' in question_html:
            # 提取div内的内容
            div_pattern = r'<div[^>]*class="custom_ueditor_cn_body"[^>]*>(.*?)</div>'
            div_match = re.search(div_pattern, question_html, re.DOTALL)
            if div_match:
                content = div_match.group(1)
                # 移除HTML标签，保留文本
                text = re.sub(r'<[^>]+>', '', content)
                text = text.replace('&nbsp;', ' ')
                return self.clean_text(text)
        
        # 直接提取h4标签内的文本
        text = re.sub(r'<[^>]+>', '', question_html)
        text = text.replace('&nbsp;', ' ')
        return self.clean_text(text)
    
    def extract_choices_and_answer(self, choices_html):
        """提取选项和正确答案"""
        choices = []
        correct_answer = None
        
        # 查找所有选项
        choice_pattern = r'<label[^>]*>.*?<span[^>]*class="el-radio__label"[^>]*>.*?<span[^>]*class="radioInput"[^>]*>\s*([A-D]).*?<\/span>.*?<span[^>]*class="radioText"[^>]*>(.*?)<\/span>.*?<\/label>'
        choice_matches = re.findall(choice_pattern, choices_html, re.DOTALL)
        
        print(f"DEBUG: extract_choices_and_answer - choices_html: {choices_html[:500]}...") # Added debug print
        for letter, choice_text in choice_matches:
            # 处理选项文本
            if 'custom_ueditor_cn_body' in choice_text:
                # 提取div内的内容
                div_pattern = r'<div[^>]*class="custom_ueditor_cn_body"[^>]*>(.*?)</div>'
                div_match = re.search(div_pattern, choice_text, re.DOTALL)
                if div_match:
                    content = div_match.group(1)
                    text = re.sub(r'<[^>]+>', '', content)
                else:
                    text = re.sub(r'<[^>]+>', '', choice_text)
            else:
                text = re.sub(r'<[^>]+>', '', choice_text)
            
            text = text.replace('&nbsp;', ' ')
            clean_text = self.clean_text(text)
            choices.append(f"{letter}.{clean_text}")
        print(f"DEBUG: extract_choices_and_answer - Extracted choices: {choices}") # Added debug print
        
        # 查找正确答案（包含is-checked类的选项）
        correct_pattern = r'<label[^>]*class="[^"]*is-checked[^"]*"[^>]*>.*?<span[^>]*class="el-radio__label"[^>]*>.*?<span[^>]*class="radioInput"[^>]*>\s*([A-D]).*?<\/span>.*?<span[^>]*class="radioText"[^>]*>.*?<\/span>.*?<\/label>'
        correct_match = re.search(correct_pattern, choices_html, re.DOTALL)
        if correct_match:
            correct_answer = correct_match.group(1)
        print(f"DEBUG: extract_choices_and_answer - Extracted correct_answer: {correct_answer}") # Added debug print
        
        return choices, correct_answer
    
    def extract_fillblank_question_text(self, question_body):
        """提取填空题题目文本"""
        # 检查是否包含custom_ueditor_cn_body
        if 'custom_ueditor_cn_body' in question_body:
            # 提取div内的内容
            div_pattern = r'<div[^>]*class="custom_ueditor_cn_body"[^>]*>(.*?)</div>'
            div_match = re.search(div_pattern, question_body, re.DOTALL)
            if div_match:
                content = div_match.group(1)
                # 移除填空区域的span标签，保留题目文本
                content = re.sub(r'<span[^>]*class="blank-item-dynamic[^"]*"[^>]*>.*?</span>', '______', content)
                # 移除其他HTML标签
                text = re.sub(r'<[^>]+>', '', content)
                text = text.replace('&nbsp;', ' ')
                return self.clean_text(text)
        
        # 直接处理题目内容
        # 移除填空区域的span标签，保留题目文本
        content = re.sub(r'<span[^>]*class="blank-item-dynamic[^"]*"[^>]*>.*?</span>', '______', question_body)
        # 移除其他HTML标签
        text = re.sub(r'<[^>]+>', '', content)
        text = text.replace('&nbsp;', ' ')
        return self.clean_text(text)
    
    def extract_fillblank_answers(self, answer_section):
        """提取填空题答案"""
        answers = []
        
        # 查找所有填空答案
        answer_pattern = r'填空\d+\s*:\s*</span>\s*<span[^>]*class="exam-font"[^>]*>(.*?)</span>'
        answer_matches = re.findall(answer_pattern, answer_section, re.DOTALL)
        
        for answer_text in answer_matches:
            # 清理答案文本
            clean_answer = self.clean_text(answer_text)
            if clean_answer:
                answers.append(clean_answer)
        
        return answers
    
    def clean_text(self, text):
        """清理文本"""
        if not text:
            return ""
        text = re.sub(r'\s+', ' ', text.strip())
        text = text.replace('&nbsp;', ' ')
        return text

class HTMLToProblemXConverter:
    def __init__(self):
        self.choice_questions = []
        self.fillblank_questions = []
        self.stats = {
            'choice_count': 0,
            'fillblank_count': 0,
            'total_count': 0
        }
    
    def clean_text(self, text: str) -> str:
        """清理文本，移除多余的空白字符和HTML标签"""
        if not text:
            return ""
        # 移除多余的空白字符
        text = re.sub(r'\s+', ' ', text.strip())
        # 移除HTML实体
        text = text.replace('&nbsp;', ' ')
        return text
    

    

    

    

    

    
    def parse_html_file(self, html_file_path: str):
        """解析HTML文件并提取题目"""
        print(f"正在解析HTML文件: {html_file_path}")
        
        try:
            with open(html_file_path, 'r', encoding='utf-8') as f:
                content = f.read()
        
            parser = HTMLQuestionParser()
            questions = parser.parse(content)
            
            print(f"找到 {len(questions)} 道题目")
            
            for i, question in enumerate(questions, 1):
                try:
                    if question['type'] == 'Choice':
                        choice_question = {
                            "type": "Choice",
                            "question": question['question'],
                            "choices": question['choices'],
                            "answer": question['answer']
                        }
                        self.choice_questions.append(choice_question)
                        print(f"✓ 成功解析第{i}题 (选择题)")
                    elif question['type'] == 'FillBlank':
                        # 将单个答案转换为列表格式
                        answers = [question['answer']] if isinstance(question['answer'], str) else question['answer']
                        fillblank_question = {
                            "type": "FillBlank",
                            "question": question['question'],
                            "BlankNum": len(answers),
                            "answer": answers
                        }
                        self.fillblank_questions.append(fillblank_question)
                        print(f"✓ 成功解析第{i}题 (填空题，{len(answers)}个空)")
                        
                except Exception as e:
                    print(f"处理第{i}题时出错: {e}")
                    continue
            
            self.update_stats()
            
        except Exception as e:
            print(f"解析HTML文件时出错: {e}")
            return
        self.print_stats()
    

    

    
    def update_stats(self):
        """更新统计信息"""
        self.stats['choice_count'] = len(self.choice_questions)
        self.stats['fillblank_count'] = len(self.fillblank_questions)
        self.stats['total_count'] = self.stats['choice_count'] + self.stats['fillblank_count']
    
    def print_stats(self):
        """打印统计信息"""
        print("\n=== 转换统计 ===")
        print(f"选择题: {self.stats['choice_count']} 道")
        print(f"填空题: {self.stats['fillblank_count']} 道")
        print(f"总计: {self.stats['total_count']} 道")
    
    def save_to_json(self, output_dir: str, subject_name: str = "OS"):
        """保存为JSON文件"""
        os.makedirs(output_dir, exist_ok=True)
        
        # 保存选择题
        if self.choice_questions:
            choice_dir = os.path.join(output_dir, "Choice")
            os.makedirs(choice_dir, exist_ok=True)
            
            choice_data = {"data": self.choice_questions}
            choice_file = os.path.join(choice_dir, f"{subject_name}_Choice.json")
            
            with open(choice_file, 'w', encoding='utf-8') as f:
                json.dump(choice_data, f, ensure_ascii=False, indent=2)
            
            print(f"✓ 选择题已保存到: {choice_file}")
        
        # 保存填空题
        if self.fillblank_questions:
            fillblank_dir = os.path.join(output_dir, "FillBlank")
            os.makedirs(fillblank_dir, exist_ok=True)
            
            fillblank_data = {"data": self.fillblank_questions}
            fillblank_file = os.path.join(fillblank_dir, f"{subject_name}_FillBlank.json")
            
            with open(fillblank_file, 'w', encoding='utf-8') as f:
                json.dump(fillblank_data, f, ensure_ascii=False, indent=2)
            
            print(f"✓ 填空题已保存到: {fillblank_file}")
        
        print(f"\n转换完成！输出目录: {output_dir}")

def main():
    """主函数"""
    # 创建参数解析器
    parser = argparse.ArgumentParser(description="HTML题库转换为ProblemX格式的转换工具")
    parser.add_argument("-i", "--input", required=True, help="输入HTML文件的路径")
    parser.add_argument("-o", "--output", required=True, help="输出目录的路径")
    parser.add_argument("-name", "--name", required=True, help="输出文件的前缀名 (例如 U1)")
    
    args = parser.parse_args()
    
    html_file = args.input
    output_dir = args.output
    output_name_prefix = args.name
    
    # 检查输入文件
    if not os.path.exists(html_file):
        print(f"错误: HTML文件不存在: {html_file}")
        return
    
    # 创建转换器并执行转换
    converter = HTMLToProblemXConverter()
    
    try:
        converter.parse_html_file(html_file)
        converter.save_to_json(output_dir, output_name_prefix) # 使用命令行参数指定的文件名前缀
        
        print("\n=== 转换成功完成 ===")
        print(f"HTML文件: {html_file}")
        print(f"输出目录: {output_dir}")
        print(f"选择题: {converter.stats['choice_count']} 道")
        print(f"填空题: {converter.stats['fillblank_count']} 道")
        
    except Exception as e:
        print(f"转换过程中出现错误: {e}")
        import traceback
        traceback.print_exc()

if __name__ == "__main__":
    main()