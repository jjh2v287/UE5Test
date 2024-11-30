import os
import json
import tkinter as tk
from tkinter import ttk, filedialog, messagebox
from pathlib import Path
from typing import List, Dict
from datetime import datetime


class CodeCollector:
    def __init__(self, file_extensions: List[str] = ['.h', '.cpp', '.cs']):
        self.file_extensions = file_extensions
        self.collected_data = []
        self.root_path = None

    def collect_files(self) -> List[Path]:
        """모든 하위 디렉토리에서 지정된 확장자를 가진 파일들을 수집합니다."""
        files = []
        for ext in self.file_extensions:
            files.extend(self.root_path.rglob(f'*{ext}'))
        return files

    def read_file_content(self, file_path: Path) -> Dict:
        """파일의 내용을 읽고 메타데이터와 함께 딕셔너리로 반환합니다."""
        try:
            with open(file_path, 'r', encoding='utf-8') as f:
                content = f.read()

            return {
                'file_path': str(file_path.relative_to(self.root_path)),
                'extension': file_path.suffix,
                'size_bytes': file_path.stat().st_size,
                'content': content,
                'lines': len(content.splitlines())
            }
        except Exception as e:
            print(f"Error reading file {file_path}: {str(e)}")
            return None

    def process_directory(self, progress_callback=None) -> None:
        """디렉토리 내의 모든 파일을 처리합니다."""
        if not self.root_path:
            raise ValueError("Root path is not set")

        files = list(self.collect_files())
        total_files = len(files)

        self.collected_data = []

        for i, file_path in enumerate(files, 1):
            if progress_callback:
                progress_callback(i, total_files, str(file_path))
            file_data = self.read_file_content(file_path)
            if file_data:
                self.collected_data.append(file_data)

    def save_dataset(self, output_file: str) -> None:
        """수집된 데이터를 JSON 파일로 저장합니다."""
        dataset = {
            'metadata': {
                'total_files': len(self.collected_data),
                'extensions': self.file_extensions,
                'root_path': str(self.root_path),
                'total_lines': sum(data['lines'] for data in self.collected_data),
                'created_at': datetime.now().isoformat()
            },
            'files': self.collected_data
        }

        with open(output_file, 'w', encoding='utf-8') as f:
            json.dump(dataset, f, indent=2, ensure_ascii=False)


class CodeCollectorGUI:
    def __init__(self):
        self.window = tk.Tk()
        self.window.title("코드 수집기")
        self.window.geometry("600x400")

        self.collector = CodeCollector()
        self.setup_gui()

    def setup_gui(self):
        # 메인 프레임
        main_frame = ttk.Frame(self.window, padding="10")
        main_frame.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))

        # 소스 경로 선택
        ttk.Label(main_frame, text="소스 코드 경로:").grid(row=0, column=0, sticky=tk.W)
        self.path_var = tk.StringVar()
        path_entry = ttk.Entry(main_frame, textvariable=self.path_var, width=50)
        path_entry.grid(row=0, column=1, padx=5)
        ttk.Button(main_frame, text="찾아보기", command=self.browse_directory).grid(row=0, column=2)

        # 파일 확장자 선택
        ttk.Label(main_frame, text="파일 확장자:").grid(row=1, column=0, sticky=tk.W, pady=10)
        extensions_frame = ttk.Frame(main_frame)
        extensions_frame.grid(row=1, column=1, columnspan=2, sticky=tk.W)

        self.ext_vars = {
            '.h': tk.BooleanVar(value=True),
            '.cpp': tk.BooleanVar(value=True),
            '.cs': tk.BooleanVar(value=True)
        }

        for i, (ext, var) in enumerate(self.ext_vars.items()):
            ttk.Checkbutton(extensions_frame, text=ext, variable=var).grid(row=0, column=i, padx=5)

        # 진행 상황 표시
        ttk.Label(main_frame, text="진행 상황:").grid(row=2, column=0, sticky=tk.W, pady=10)
        self.progress_var = tk.StringVar(value="대기 중...")
        ttk.Label(main_frame, textvariable=self.progress_var).grid(row=2, column=1, columnspan=2, sticky=tk.W)

        self.progress_bar = ttk.Progressbar(main_frame, length=400, mode='determinate')
        self.progress_bar.grid(row=3, column=0, columnspan=3, pady=10)

        # 현재 처리 중인 파일 표시
        self.current_file_var = tk.StringVar()
        ttk.Label(main_frame, textvariable=self.current_file_var, wraplength=500).grid(row=4, column=0, columnspan=3,
                                                                                       pady=5)

        # 실행 버튼
        ttk.Button(main_frame, text="수집 시작", command=self.start_collection).grid(row=5, column=0, columnspan=3, pady=20)

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

        # 선택된 확장자 필터링
        selected_extensions = [ext for ext, var in self.ext_vars.items() if var.get()]
        if not selected_extensions:
            messagebox.showerror("오류", "최소한 하나의 파일 확장자를 선택해주세요.")
            return

        try:
            self.collector.root_path = Path(source_path)
            self.collector.file_extensions = selected_extensions

            # 저장 경로 선택
            save_path = filedialog.asksaveasfilename(
                defaultextension=".json",
                filetypes=[("JSON files", "*.json")],
                title="데이터셋 저장 위치 선택"
            )

            if not save_path:
                return

            self.collector.process_directory(self.update_progress)
            self.collector.save_dataset(save_path)

            stats = f"""수집 완료!
            처리된 파일 수: {len(self.collector.collected_data)}
            총 코드 라인 수: {sum(data['lines'] for data in self.collector.collected_data)}
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