import json
import os
import sys


def load_json(path: str):
    with open(path, "r", encoding="utf-8-sig") as f:
        return json.load(f)


def save_json(path: str, obj):
    text = json.dumps(obj, ensure_ascii=False, indent=4)
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        f.write(text)
        f.write("\n")


def normalize_image_to_img1(image_value):
    if isinstance(image_value, str):
        v = image_value.strip()
        return ({"img1": v}, True) if v else ({"img1": ""}, True)

    if isinstance(image_value, dict):
        if "img1" in image_value and isinstance(image_value["img1"], str):
            v = image_value["img1"]
            new_obj = {"img1": v}
            return new_obj, (image_value != new_obj)

        v = None
        for _, val in image_value.items():
            if isinstance(val, str) and val.strip():
                v = val
                break
        if v is None:
            return image_value, False

        new_obj = {"img1": v}
        return new_obj, (image_value != new_obj)

    return image_value, False


def ensure_img1_reference(question_text: str):
    if "![](img1)" in question_text:
        return question_text, False
    return question_text + "\n\n![](img1)", True


def convert_question_obj(qobj: dict):
    if "image" not in qobj:
        return False

    changed = False
    new_image, img_changed = normalize_image_to_img1(qobj.get("image"))
    if isinstance(new_image, dict):
        qobj["image"] = new_image
        changed = changed or img_changed

        qt = qobj.get("question")
        if isinstance(qt, str):
            new_qt, qt_changed = ensure_img1_reference(qt)
            qobj["question"] = new_qt
            changed = changed or qt_changed

    return changed


def convert_file(path: str):
    root = load_json(path)
    changed = False

    if isinstance(root, dict) and isinstance(root.get("data"), list):
        for item in root["data"]:
            if isinstance(item, dict):
                changed = convert_question_obj(item) or changed
    elif isinstance(root, list):
        for item in root:
            if isinstance(item, dict):
                changed = convert_question_obj(item) or changed
    else:
        return False

    if changed:
        save_json(path, root)
    return changed


def main():
    if len(sys.argv) < 2:
        print("Usage: python convert_bank_images.py <root_dir>")
        return 2

    root_dir = sys.argv[1]
    total_files = 0
    changed_files = 0

    for dirpath, _, filenames in os.walk(root_dir):
        for name in filenames:
            if not name.lower().endswith(".json"):
                continue
            total_files += 1
            path = os.path.join(dirpath, name)
            try:
                if convert_file(path):
                    changed_files += 1
            except Exception as e:
                print(f"ERROR: {path}\n{e}")

    print(f"Scanned: {total_files} json files")
    print(f"Changed: {changed_files} json files")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())