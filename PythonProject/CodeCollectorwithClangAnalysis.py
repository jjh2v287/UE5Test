import tkinter as tk
from tkinter import ttk, filedialog, messagebox
from typing import List, Dict, Optional, Set
from dataclasses import dataclass, field
from pathlib import Path
import json
from datetime import datetime
from clang.cindex import Index, CursorKind, Cursor
import clang.cindex

# Clang 설정
clang.cindex.Config.set_library_file("llvm/bin/libclang.dll")


@dataclass
class CodeContext:
    """코드 컨텍스트 정보"""
    code_block: str
    comments: List[str] = field(default_factory=list)
    semantic_type: Optional[str] = None
    namespace: Optional[str] = None


@dataclass
class CodeRelation:
    """코드 관계 정보"""
    source_file: str
    target_file: Optional[str]
    relation_type: str
    source_symbol: Optional[str]
    target_symbol: Optional[str]
    location: Optional[str]
    is_virtual: bool = False


class CodeCollector:
    def __init__(self, file_extensions: List[str] = ['.h', '.cpp']):
        self.file_extensions = file_extensions
        self.collected_data = []
        self.root_path = None
        self.index = Index.create()

    def collect_files(self) -> List[Path]:
        """지정된 확장자의 파일 수집"""
        files = []
        for ext in self.file_extensions:
            files.extend(self.root_path.rglob(f'*{ext}'))
        return files

    def analyze_file(self, file_path: Path) -> Dict:
        """파일 분석"""
        try:
            with open(file_path, 'r', encoding='utf-8') as f:
                content = f.read()
                content_lines = content.splitlines(keepends=True)  # 줄바꿈 유지

            # 컴파일러 인자 설정
            compilation_args = [
                '-x', 'c++',
                '-std=c++17',
                f'-I{self.root_path}',
                f'-I{self.root_path}/Source'
            ]

            tu = self.index.parse(str(file_path), args=compilation_args)

            if not tu:
                print(f"Failed to parse {file_path}")
                return None

            relations = []
            semantic_data = []

            def extract_code_block(cursor: Cursor) -> str:
                """커서의 위치에서 코드 블록 추출"""
                try:
                    start_line = cursor.extent.start.line - 1  # 0-based index
                    start_col = cursor.extent.start.column - 1
                    end_line = cursor.extent.end.line - 1
                    end_col = cursor.extent.end.column - 1

                    if start_line < 0 or end_line >= len(content_lines):
                        return ""

                    # 단일 라인인 경우
                    if start_line == end_line:
                        line = content_lines[start_line]
                        return line[start_col:end_col]

                    # 여러 라인인 경우
                    code_block = []
                    for i in range(start_line, end_line + 1):
                        if i == start_line:
                            code_block.append(content_lines[i][start_col:])
                        elif i == end_line:
                            code_block.append(content_lines[i][:end_col])
                        else:
                            code_block.append(content_lines[i])

                    return ''.join(code_block)
                except Exception as e:
                    print(f"Error extracting code block: {str(e)}")
                    return ""

            def process_cursor(cursor: Cursor):
                try:
                    if cursor.location.file:
                        curr_file = cursor.location.file.name
                    else:
                        curr_file = str(file_path)

                    if curr_file != str(file_path):
                        return

                    # AST 노드 처리
                    if cursor.kind in [CursorKind.CLASS_DECL, CursorKind.STRUCT_DECL,
                                       CursorKind.FUNCTION_DECL, CursorKind.CXX_METHOD,
                                       CursorKind.NAMESPACE, CursorKind.ENUM_DECL]:

                        code = extract_code_block(cursor)
                        if code.strip():  # 빈 코드 블록 제외
                            semantic_data.append({
                                'code': code,
                                'type': cursor.kind.name,
                                'location': f"{cursor.location.line}:{cursor.location.column}",
                                'spelling': cursor.spelling,
                                'comments': [],  # 필요한 경우 주석 추출 로직 추가
                                'namespace': cursor.semantic_parent.spelling if cursor.semantic_parent else None
                            })

                    # 상속 관계 처리
                    if cursor.kind == CursorKind.CXX_BASE_SPECIFIER:
                        parent = cursor.type.get_canonical().spelling
                        child = cursor.semantic_parent.spelling if cursor.semantic_parent else None
                        if child and parent:
                            relations.append(CodeRelation(
                                source_file=str(file_path),
                                target_file=curr_file,
                                relation_type='inherit',
                                source_symbol=child,
                                target_symbol=parent,
                                location=f"{cursor.location.line}:{cursor.location.column}",
                                is_virtual=cursor.is_virtual_base()
                            ))

                    # 재귀적으로 자식 처리
                    for child in cursor.get_children():
                        process_cursor(child)

                except Exception as e:
                    print(f"Error processing cursor: {str(e)}")

            process_cursor(tu.cursor)

            return {
                'file_path': str(file_path.relative_to(self.root_path)),
                'size': file_path.stat().st_size,
                'lines': len(content_lines),
                'content': content,
                'relations': [vars(r) for r in relations],
                'semantic_data': semantic_data
            }

        except Exception as e:
            print(f"Error analyzing {file_path}: {str(e)}")
            return None

    def process_directory(self, progress_callback=None) -> None:
        """디렉토리 처리"""
        if not self.root_path:
            raise ValueError("Root path is not set")

        files = list(self.collect_files())
        total_files = len(files)
        self.collected_data = []

        for i, file_path in enumerate(files, 1):
            if progress_callback:
                progress_callback(i, total_files, str(file_path))
            file_data = self.analyze_file(file_path)
            if file_data:
                self.collected_data.append(file_data)

    def save_dataset(self, output_file: str) -> None:
        """결과 저장"""
        dataset = {
            'metadata': {
                'total_files': len(self.collected_data),
                'extensions': self.file_extensions,
                'root_path': str(self.root_path),
                'created_at': datetime.now().isoformat()
            },
            'files': self.collected_data
        }

        with open(output_file, 'w', encoding='utf-8') as f:
            json.dump(dataset, f, indent=2, ensure_ascii=False)


class CodeCollectorGUI:
    def __init__(self):
        self.window = tk.Tk()
        self.window.title("코드 분석기")
        self.window.geometry("600x400")
        self.collector = CodeCollector()
        self.setup_gui()

    def setup_gui(self):
        main_frame = ttk.Frame(self.window, padding="10")
        main_frame.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))

        # 소스 경로 선택
        ttk.Label(main_frame, text="소스 코드 경로:").grid(row=0, column=0, sticky=tk.W)
        self.path_var = tk.StringVar()
        ttk.Entry(main_frame, textvariable=self.path_var, width=50).grid(row=0, column=1, padx=5)
        ttk.Button(main_frame, text="찾아보기", command=self.browse_directory).grid(row=0, column=2)

        # 파일 확장자 선택
        ttk.Label(main_frame, text="파일 확장자:").grid(row=1, column=0, sticky=tk.W, pady=10)
        extensions_frame = ttk.Frame(main_frame)
        extensions_frame.grid(row=1, column=1, columnspan=2, sticky=tk.W)

        self.ext_vars = {
            '.h': tk.BooleanVar(value=True),
            '.cpp': tk.BooleanVar(value=True)
        }

        for i, (ext, var) in enumerate(self.ext_vars.items()):
            ttk.Checkbutton(extensions_frame, text=ext, variable=var).grid(row=0, column=i, padx=5)

        # 진행 상황
        ttk.Label(main_frame, text="진행 상황:").grid(row=2, column=0, sticky=tk.W, pady=10)
        self.progress_var = tk.StringVar(value="대기 중...")
        ttk.Label(main_frame, textvariable=self.progress_var).grid(row=2, column=1, columnspan=2, sticky=tk.W)

        self.progress_bar = ttk.Progressbar(main_frame, length=400, mode='determinate')
        self.progress_bar.grid(row=3, column=0, columnspan=3, pady=10)

        self.current_file_var = tk.StringVar()
        ttk.Label(main_frame, textvariable=self.current_file_var, wraplength=500).grid(
            row=4, column=0, columnspan=3, pady=5)

        ttk.Button(main_frame, text="분석 시작", command=self.start_collection).grid(
            row=5, column=0, columnspan=3, pady=20)

    def browse_directory(self):
        directory = filedialog.askdirectory(title="소스 코드 디렉토리 선택")
        if directory:
            self.path_var.set(directory)

    def update_progress(self, current, total, current_file):
        progress = (current / total) * 100
        self.progress_bar['value'] = progress
        self.progress_var.set(f"처리 중... ({current}/{total} 파일)")
        self.current_file_var.set(f"현재 파일: {current_file}")
        self.window.update()

    def start_collection(self):
        source_path = self.path_var.get()
        if not source_path:
            messagebox.showerror("오류", "소스 코드 경로를 선택해주세요.")
            return

        selected_extensions = [ext for ext, var in self.ext_vars.items() if var.get()]
        if not selected_extensions:
            messagebox.showerror("오류", "최소한 하나의 파일 확장자를 선택해주세요.")
            return

        try:
            self.collector.root_path = Path(source_path)
            self.collector.file_extensions = selected_extensions

            save_path = filedialog.asksaveasfilename(
                defaultextension=".json",
                filetypes=[("JSON files", "*.json")],
                title="분석 결과 저장 위치 선택"
            )

            if not save_path:
                return

            self.collector.process_directory(self.update_progress)
            self.collector.save_dataset(save_path)

            stats = f"""분석 완료!
            처리된 파일 수: {len(self.collector.collected_data)}
            데이터셋 저장 위치: {save_path}"""

            messagebox.showinfo("완료", stats)
            self.progress_var.set("완료!")
            self.current_file_var.set("")

        except Exception as e:
            messagebox.showerror("오류", f"처리 중 오류가 발생했습니다: {str(e)}")

    def run(self):
        self.window.mainloop()


if __name__ == "__main__":
    app = CodeCollectorGUI()
    app.run()