import tkinter as tk
from tkinter import ttk, filedialog, messagebox
import json
import torch
from pathlib import Path
from transformers import AutoTokenizer, AutoModel
from tqdm import tqdm
import numpy as np
from typing import List, Dict
import logging
import threading
import queue


class GraphCodeBERTProcessorGUI:
    def __init__(self):
        self.window = tk.Tk()
        self.window.title("GraphCodeBERT 코드 처리기")
        self.window.geometry("800x600")

        # 모델 초기화를 위한 큐
        self.status_queue = queue.Queue()

        self.setup_gui()
        self.initialize_model_in_background()

    def setup_gui(self):
        # 메인 프레임
        main_frame = ttk.Frame(self.window, padding="10")
        main_frame.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))

        # 입력 파일/폴더 선택
        ttk.Label(main_frame, text="입력 JSON 파일:").grid(row=0, column=0, sticky=tk.W)
        self.input_path_var = tk.StringVar()
        input_entry = ttk.Entry(main_frame, textvariable=self.input_path_var, width=60)
        input_entry.grid(row=0, column=1, padx=5)
        ttk.Button(main_frame, text="찾아보기", command=self.browse_input_file).grid(row=0, column=2)

        # 출력 파일 선택
        ttk.Label(main_frame, text="출력 JSON 파일:").grid(row=1, column=0, sticky=tk.W, pady=10)
        self.output_path_var = tk.StringVar()
        output_entry = ttk.Entry(main_frame, textvariable=self.output_path_var, width=60)
        output_entry.grid(row=1, column=1, padx=5)
        ttk.Button(main_frame, text="찾아보기", command=self.browse_output_file).grid(row=1, column=2)

        # 설정 프레임
        settings_frame = ttk.LabelFrame(main_frame, text="설정", padding="5")
        settings_frame.grid(row=2, column=0, columnspan=3, sticky=(tk.W, tk.E), pady=10)

        # 최대 토큰 길이 설정
        ttk.Label(settings_frame, text="최대 토큰 길이:").grid(row=0, column=0, sticky=tk.W)
        self.max_length_var = tk.StringVar(value="512")
        ttk.Entry(settings_frame, textvariable=self.max_length_var, width=10).grid(row=0, column=1, padx=5)

        # 상태 표시
        self.status_var = tk.StringVar(value="모델 초기화 중...")
        ttk.Label(main_frame, textvariable=self.status_var, wraplength=700).grid(
            row=3, column=0, columnspan=3, pady=10
        )

        # 진행 상황 바
        self.progress_var = tk.DoubleVar()
        self.progress_bar = ttk.Progressbar(
            main_frame,
            length=700,
            mode='determinate',
            variable=self.progress_var
        )
        self.progress_bar.grid(row=4, column=0, columnspan=3, pady=10)

        # 현재 처리 중인 파일 표시
        self.current_file_var = tk.StringVar()
        ttk.Label(
            main_frame,
            textvariable=self.current_file_var,
            wraplength=700
        ).grid(row=5, column=0, columnspan=3, pady=5)

        # 로그 표시
        log_frame = ttk.LabelFrame(main_frame, text="로그", padding="5")
        log_frame.grid(row=6, column=0, columnspan=3, sticky=(tk.W, tk.E, tk.N, tk.S), pady=10)

        self.log_text = tk.Text(log_frame, height=10, width=80)
        self.log_text.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)

        scrollbar = ttk.Scrollbar(log_frame, orient=tk.VERTICAL, command=self.log_text.yview)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        self.log_text.config(yscrollcommand=scrollbar.set)

        # 실행 버튼
        self.process_button = ttk.Button(
            main_frame,
            text="처리 시작",
            command=self.start_processing,
            state=tk.DISABLED
        )
        self.process_button.grid(row=7, column=0, columnspan=3, pady=10)

        # 윈도우 크기 조절 가능하도록 설정
        self.window.columnconfigure(0, weight=1)
        self.window.rowconfigure(0, weight=1)
        main_frame.columnconfigure(1, weight=1)

    def initialize_model_in_background(self):
        def init_model():
            try:
                self.tokenizer = AutoTokenizer.from_pretrained("microsoft/graphcodebert-base")
                self.model = AutoModel.from_pretrained("microsoft/graphcodebert-base")
                self.status_queue.put(("success", "모델 초기화 완료"))
            except Exception as e:
                self.status_queue.put(("error", f"모델 초기화 실패: {str(e)}"))

        # 백그라운드에서 모델 초기화 시작
        threading.Thread(target=init_model, daemon=True).start()

        # 상태 확인
        self.check_initialization_status()

    def check_initialization_status(self):
        try:
            status_type, message = self.status_queue.get_nowait()
            if status_type == "success":
                self.status_var.set(message)
                self.process_button.config(state=tk.NORMAL)
            else:
                self.status_var.set(message)
                messagebox.showerror("초기화 오류", message)
        except queue.Empty:
            self.window.after(100, self.check_initialization_status)

    def browse_input_file(self):
        filename = filedialog.askopenfilename(
            title="입력 JSON 파일 선택",
            filetypes=[("JSON files", "*.json")]
        )
        if filename:
            self.input_path_var.set(filename)

    def browse_output_file(self):
        filename = filedialog.asksaveasfilename(
            title="출력 JSON 파일 선택",
            defaultextension=".json",
            filetypes=[("JSON files", "*.json")]
        )
        if filename:
            self.output_path_var.set(filename)

    def log_message(self, message: str):
        self.log_text.insert(tk.END, f"{message}\n")
        self.log_text.see(tk.END)

    def process_file(self, code_content: str, file_path: str, max_length: int) -> Dict:
        # 코드를 적절한 크기로 분할
        lines = code_content.split('\n')
        chunks = []
        current_chunk = []
        current_length = 0

        for line in lines:
            line_tokens = len(self.tokenizer.tokenize(line))

            if current_length + line_tokens > max_length and current_chunk:
                chunks.append('\n'.join(current_chunk))
                current_chunk = []
                current_length = 0

            current_chunk.append(line)
            current_length += line_tokens

        if current_chunk:
            chunks.append('\n'.join(current_chunk))

        processed_chunks = []
        for chunk in chunks:
            tokens = self.tokenizer(
                chunk,
                return_tensors='pt',
                max_length=max_length,
                truncation=True,
                padding='max_length'
            )

            with torch.no_grad():
                outputs = self.model(**tokens)
                embeddings = outputs.last_hidden_state.mean(dim=1).numpy()

            processed_chunks.append({
                'code_chunk': chunk,
                'embeddings': embeddings.tolist()
            })

        return {
            'file_path': file_path,
            'chunks': processed_chunks
        }

    def start_processing(self):
        input_path = self.input_path_var.get()
        output_path = self.output_path_var.get()

        if not input_path or not output_path:
            messagebox.showerror("오류", "입력과 출력 파일을 모두 선택해주세요.")
            return

        try:
            max_length = int(self.max_length_var.get())
        except ValueError:
            messagebox.showerror("오류", "최대 토큰 길이는 숫자여야 합니다.")
            return

        def process():
            try:
                with open(input_path, 'r', encoding='utf-8') as f:
                    data = json.load(f)

                total_files = len(data['files'])
                processed_files = []

                for i, file_data in enumerate(data['files'], 1):
                    # 진행 상황 업데이트
                    progress = (i / total_files) * 100
                    self.progress_var.set(progress)
                    self.current_file_var.set(f"처리 중: {file_data['file_path']}")

                    # 파일 처리
                    processed = self.process_file(
                        file_data['content'],
                        file_data['file_path'],
                        max_length
                    )
                    processed_files.append(processed)

                    # 로그 메시지
                    self.log_message(f"처리 완료: {file_data['file_path']}")

                # 결과 저장
                output_data = {
                    'metadata': {
                        **data['metadata'],
                        'processed_files': len(processed_files),
                        'model_name': "microsoft/graphcodebert-base",
                        'max_length': max_length
                    },
                    'processed_data': processed_files
                }

                with open(output_path, 'w', encoding='utf-8') as f:
                    json.dump(output_data, f, indent=2, ensure_ascii=False)

                self.status_var.set("처리 완료!")
                messagebox.showinfo("완료", "모든 파일의 처리가 완료되었습니다.")

            except Exception as e:
                self.status_var.set(f"오류 발생: {str(e)}")
                messagebox.showerror("오류", str(e))

            finally:
                self.process_button.config(state=tk.NORMAL)

        self.process_button.config(state=tk.DISABLED)
        self.status_var.set("처리 중...")
        threading.Thread(target=process, daemon=True).start()

    def run(self):
        self.window.mainloop()


if __name__ == "__main__":
    app = GraphCodeBERTProcessorGUI()
    app.run()