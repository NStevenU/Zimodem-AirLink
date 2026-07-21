import os

def process_file(filepath):
    with open(filepath, 'r', encoding='utf-8') as f:
        content = f.read()

    new_content = ""
    in_string = False
    in_single_line_comment = False
    in_multi_line_comment = False
    i = 0
    while i < len(content):
        if in_single_line_comment:
            new_content += content[i]
            if content[i] == '\n':
                in_single_line_comment = False
            i += 1
            continue

        if in_multi_line_comment:
            new_content += content[i]
            if content[i] == '*' and i + 1 < len(content) and content[i+1] == '/':
                new_content += '/'
                in_multi_line_comment = False
                i += 2
            else:
                i += 1
            continue

        if in_string:
            if content[i] == '\\': # escaped character
                new_content += content[i:i+2]
                i += 2
                continue
            if content[i] == '"':
                in_string = False
                new_content += content[i]
                i += 1
                continue
            
            char = content[i]
            if '\uac00' <= char <= '\ud7a3' or '\u3131' <= char <= '\u318e' or '\u1100' <= char <= '\u11FF':
                try:
                    euckr_bytes = char.encode('euc-kr')
                    for b in euckr_bytes:
                        new_content += f"\\x{b:02X}"
                    
                    # Prevent hex consumption if next char is a hex digit
                    if i + 1 < len(content):
                        next_char = content[i+1]
                        is_next_korean = ('\uac00' <= next_char <= '\ud7a3' or '\u3131' <= next_char <= '\u318e' or '\u1100' <= next_char <= '\u11FF')
                        if not is_next_korean and next_char in '0123456789abcdefABCDEF':
                            new_content += '""'
                except UnicodeEncodeError:
                    new_content += char
            else:
                new_content += char
            i += 1
            continue
            
        if content[i] == '/' and i + 1 < len(content) and content[i+1] == '/':
            in_single_line_comment = True
            new_content += '//'
            i += 2
            continue
            
        if content[i] == '/' and i + 1 < len(content) and content[i+1] == '*':
            in_multi_line_comment = True
            new_content += '/*'
            i += 2
            continue
            
        if content[i] == '"':
            in_string = True
            new_content += '"'
            i += 1
            continue
            
        new_content += content[i]
        i += 1

    if content != new_content:
        with open(filepath, 'w', encoding='utf-8') as f:
            f.write(new_content)
        return True
    return False

if __name__ == '__main__':
    directory = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'zimodem')
    for root, _, files in os.walk(directory):
        for file in files:
            if file.endswith(('.ino', '.cpp', '.h')):
                filepath = os.path.join(root, file)
                if process_file(filepath):
                    print(f"Updated {filepath}")
