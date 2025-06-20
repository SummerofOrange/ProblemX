#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
从HTML文件中提取判断题或选择题并转换为JSON格式。
"""

import json
import logging
import argparse
from bs4 import BeautifulSoup

# --- 日志设置 ---
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

# --- HTML 选择器 ---
QUESTION_CONTAINER_SELECTOR = "div.pc-x[id]"
QUESTION_TEXT_SELECTOR = "div.rendered-markdown p"
RADIO_INPUT_SELECTOR = "input[type=radio]"

def extract_true_false_questions(soup: BeautifulSoup) -> list:
    """
    从解析后的HTML内容中提取判断题。

    Args:
        soup (BeautifulSoup): BeautifulSoup 对象。

    Returns:
        list: 包含提取的判断题数据的列表。
    """
    questions = []
    question_containers = soup.select(QUESTION_CONTAINER_SELECTOR)
    logger.info(f"找到 {len(question_containers)} 个潜在的判断题容器。")

    for container in question_containers:
        question_text_element = container.select_one(QUESTION_TEXT_SELECTOR)
        if not question_text_element:
            continue
        question_text = question_text_element.get_text(strip=True)

        correct_answer = None
        choices_found = []
        radio_inputs = container.select(RADIO_INPUT_SELECTOR)
        for radio in radio_inputs:
            parent_label = radio.find_parent('label')
            if parent_label:
                choice_text = parent_label.get_text(strip=True)
                choices_found.append(choice_text)
                if radio.has_attr('checked'):
                    correct_answer = choice_text
        
        if 'T' in choices_found and 'F' in choices_found:
            question_data = {
                "type": "TrueorFalse",
                "question": question_text,
                "answer": correct_answer
            }
            questions.append(question_data)
            logger.info(f"成功提取判断题: {question_text[:30]}...")

    return questions

def extract_fill_blank_questions(soup: BeautifulSoup) -> list:
    """
    从解析后的HTML内容中提取填空题。

    Args:
        soup (BeautifulSoup): BeautifulSoup 对象。

    Returns:
        list: 包含提取的填空题数据的列表。
    """
    questions = []
    question_containers = soup.select(QUESTION_CONTAINER_SELECTOR)
    logger.info(f"找到 {len(question_containers)} 个潜在的填空题容器。")

    for container in question_containers:
        question_html_element = container.select_one("div.rendered-markdown")
        if not question_html_element:
            continue

        answers = []
        # Find all blank containers. Iterate over a copy as we modify the tree.
        blank_containers = list(question_html_element.select('span[data-blank="true"]'))
        
        if not blank_containers:
            continue

        for i, blank_container in enumerate(blank_containers, 1):
            input_tag = blank_container.find('input', {'data-blank': 'true'})
            answers.append(input_tag.get('value', '') if input_tag else '')

            score_element = blank_container.find(class_="pc-text-raw")
            score = 'x'
            if score_element and '分' in score_element.get_text():
                score_text = score_element.get_text(strip=True)
                score_num = ''.join(filter(str.isdigit, score_text))
                if score_num:
                    score = score_num
            
            placeholder = f"【第{i}空({score}分)】"
            blank_container.replace_with(placeholder)
        
        if not answers:
            continue

        question_text = question_html_element.get_text(strip=True, separator='\n')

        question_data = {
            "type": "FillBlank",
            "question": question_text,
            "BlankNum": len(answers),
            "answer": answers
        }
        questions.append(question_data)
        logger.info(f"成功提取填空题: {question_text[:30]}...")

    return questions

def extract_choice_questions(soup: BeautifulSoup) -> list:
    """
    从解析后的HTML内容中提取选择题。

    Args:
        soup (BeautifulSoup): BeautifulSoup 对象。

    Returns:
        list: 包含提取的选择题数据的列表。
    """
    questions = []
    question_containers = soup.select(QUESTION_CONTAINER_SELECTOR)
    logger.info(f"找到 {len(question_containers)} 个潜在的选择题容器。")

    for container in question_containers:
        question_text_element = container.select_one(QUESTION_TEXT_SELECTOR)
        if not question_text_element:
            continue
        question_text = question_text_element.get_text(strip=True).replace('\n', ' ')

        choices = []
        correct_answer = None
        # 选择题的选项在不同的label结构中
        option_labels = container.select('label.w-full')
        for label in option_labels:
            input_elem = label.select_one(RADIO_INPUT_SELECTOR)
            # 提取选项标识 (A, B, C, D)
            option_tag_elem = label.select_one('span:not([class])')
            # 提取选项文本
            option_text_elem = label.select_one('div.rendered-markdown p')

            if input_elem and option_tag_elem and option_text_elem:
                option_tag = option_tag_elem.get_text(strip=True).replace('.', '')
                option_text = option_text_elem.get_text(strip=True)
                
                choices.append(f"{option_tag}.{option_text}")
                if input_elem.has_attr('checked'):
                    correct_answer = option_tag

        if correct_answer and choices:
            question_data = {
                "type": "Choice",
                "question": question_text,
                "choices": choices,
                "answer": correct_answer
            }
            questions.append(question_data)
            logger.info(f"成功提取选择题: {question_text[:30]}...")

    return questions

def main():
    """
    主执行函数，处理命令行参数并调用相应功能。
    """
    parser = argparse.ArgumentParser(description='从HTML文件中提取题目')
    parser.add_argument('input_file', help='输入的HTML文件名')
    parser.add_argument('-type', choices=['TrueorFalse', 'Choice', 'FillBlank'], required=True, help='要提取的题目类型')
    parser.add_argument('-o', '--output', default='extracted_questions.json', help='输出的JSON文件名')
    args = parser.parse_args()

    try:
        with open(args.input_file, 'r', encoding='utf-8') as f:
            html_content = f.read()
        logger.info(f"成功读取HTML文件: {args.input_file}")
    except FileNotFoundError:
        logger.error(f"错误: 文件未找到 {args.input_file}")
        return

    soup = BeautifulSoup(html_content, 'html.parser')

    if args.type == 'TrueorFalse':
        extracted_data = extract_true_false_questions(soup)
    elif args.type == 'Choice':
        extracted_data = extract_choice_questions(soup)
    elif args.type == 'FillBlank':
        extracted_data = extract_fill_blank_questions(soup)
    else:
        logger.error(f"不支持的题目类型: {args.type}")
        return

    result = {"data": extracted_data}

    try:
        with open(args.output, 'w', encoding='utf-8') as f:
            json.dump(result, f, ensure_ascii=False, indent=4)
        logger.info(f"成功提取 {len(extracted_data)} 道题目并保存到: {args.output}")
    except Exception as e:
        logger.error(f"写入JSON文件时出错: {e}")

if __name__ == "__main__":
    main()