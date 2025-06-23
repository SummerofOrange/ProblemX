import argparse
import json
from bs4 import BeautifulSoup

def extract_choice_questions(html_content):
    soup = BeautifulSoup(html_content, 'html.parser')
    questions = []
    question_elements = soup.find_all('div', class_='questionLi')

    for element in question_elements:
        question_text_element = element.find('span', class_='qtContent')
        if not question_text_element:
            continue
        question_text = question_text_element.get_text(strip=True)

        options = []
        option_elements = element.find('ul', class_='mark_letter').find_all('li')
        for opt in option_elements:
            options.append(opt.get_text(strip=True))

        answer_element = element.find('span', class_='rightAnswerContent')
        if not answer_element:
            continue
        answer = answer_element.get_text(strip=True)

        questions.append({
            'type': 'Choice',
            'question': question_text,
            'choices': options,
            'answer': answer
        })

    return questions

def extract_true_false_questions(html_content):
    soup = BeautifulSoup(html_content, 'html.parser')
    questions = []
    question_elements = soup.find_all('div', class_='questionLi')

    for element in question_elements:
        question_text_element = element.find('span', class_='qtContent')
        if not question_text_element:
            continue
        question_text = question_text_element.get_text(strip=True)

        answer_element = element.find('span', class_='rightAnswerContent')
        if not answer_element:
            continue
        answer_text = answer_element.get_text(strip=True)
        answer = True if answer_text == '对' else False

        questions.append({
            'type': 'TrueorFalse',
            'question': question_text,
            'answer': 'T' if answer else 'F'
        })

    return questions

def extract_fill_blank_questions(html_content):
    soup = BeautifulSoup(html_content, 'html.parser')
    questions = []
    question_elements = soup.find_all('div', class_='questionLi')

    for element in question_elements:
        question_text_element = element.find('span', class_='qtContent')
        if not question_text_element:
            continue
        
        # Replace blanks with placeholders
        for span in question_text_element.find_all('span', style=lambda value: value and 'underline' in value):
            span.replace_with('【】')

        question_text = question_text_element.get_text(strip=True)

        answer_elements = element.find_all('dd', class_='rightAnswerContent')
        answers = [ans.get_text(strip=True).split(')')[-1].strip() for ans in answer_elements]

        questions.append({
            'type': 'FillBlank',
            'question': question_text,
            'BlankNum': len(answers),
            'answer': answers
        })

    return questions

def main():
    parser = argparse.ArgumentParser(description='Extract questions from Xuexitong HTML file.')
    parser.add_argument('file', help='The HTML file to extract questions from.')
    parser.add_argument('--type', help='The type of questions to extract.', choices=['Choice', 'TrueorFalse', 'FillBlank'], default='Choice')
    parser.add_argument('-o', '--output', help='The output JSON file name.')

    args = parser.parse_args()

    output_file = args.output
    if not output_file:
        output_file = f'extracted_{args.type}_questions.json'

    with open(args.file, 'r', encoding='utf-8') as f:
        html_content = f.read()

    if args.type == 'Choice':
        questions = extract_choice_questions(html_content)
    elif args.type == 'TrueorFalse':
        questions = extract_true_false_questions(html_content)
    elif args.type == 'FillBlank':
        questions = extract_fill_blank_questions(html_content)
    else:
        questions = []

    with open(output_file, 'w', encoding='utf-8') as f:
        json.dump({'data': questions}, f, ensure_ascii=False, indent=4)

    print(f"Successfully extracted {len(questions)} questions to {output_file}")

if __name__ == '__main__':
    main()