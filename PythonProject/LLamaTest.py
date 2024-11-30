
import os
os.environ['HF_TOKEN'] = "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"

import tkinter as tk
from tkinter import ttk, filedialog, messagebox, scrolledtext
import json
import torch
from pathlib import Path
from transformers import LlamaTokenizer, LlamaForCausalLM
from tqdm import tqdm
import numpy as np
from typing import List, Dict
import logging
import threading
import queue
from datetime import datetime


class LlamaCodeProcessor:
    def __init__(self, model_name: str = "meta-llama/Llama-3.1-8B-Instruct"):
        self.tokenizer = LlamaTokenizer.from_pretrained(model_name, legacy=False)
        self.model = LlamaForCausalLM.from_pretrained(
            model_name,
            torch_dtype="auto",
            device_map="auto",
            legacy=False
        )
        self.device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
        self.setup_logging()

    def setup_logging(self):
        logging.basicConfig(
            level=logging.INFO,
            format='%(asctime)s - %(levelname)s - %(message)s',
            handlers=[
                logging.FileHandler('llama_processing.log'),
                logging.StreamHandler()
            ]
        )
        self.logger = logging.getLogger(__name__)

    def process_code(self, code: str, max_length: int = 2048) -> Dict:
        inputs = self.tokenizer(
            code,
            return_tensors="pt",
            max_length=max_length,
            truncation=True,
            padding=True
        ).to(self.device)

        with torch.no_grad():
            outputs = self.model(**inputs, output_hidden_states=True)
            embeddings = outputs.hidden_states[-1].mean(dim=1)

        return {
            'embeddings': embeddings.cpu().numpy().tolist(),
            'tokens': inputs['input_ids'].cpu().numpy().tolist()
        }

    def _split_code_into_chunks(self, code: str, max_length: int) -> List[str]:
        tokens = self.tokenizer.tokenize(code)
        chunks = []
        current_chunk = []
        current_length = 0

        for token in tokens:
            if current_length >= max_length:
                chunks.append(self.tokenizer.convert_tokens_to_string(current_chunk))
                current_chunk = []
                current_length = 0

            current_chunk.append(token)
            current_length += 1

        if current_chunk:
            chunks.append(self.tokenizer.convert_tokens_to_string(current_chunk))

        return chunks

    def process_file(self, code_content: str, file_path: str, max_length: int) -> Dict:
        chunks = self._split_code_into_chunks(code_content, max_length)
        processed_chunks = []

        for chunk in chunks:
            processed = self.process_code(chunk, max_length)
            processed_chunks.append({
                'code_chunk': chunk,
                'embeddings': processed['embeddings'],
                'tokens': processed['tokens']
            })

        return {
            'file_path': file_path,
            'chunks': processed_chunks
        }


class LlamaCodeProcessorGUI:
    def __init__(self):
        self.window = tk.Tk()
        self.window.title("Llama 코드 프로세서")
        self.window.geometry("900x700")

        self.style = ttk.Style()
        self.style.configure('Header.TLabel', font=('Helvetica', 12, 'bold'))
        self.style.configure('Status.TLabel', font=('Helvetica', 10))

        self.processor = None
        self.processing_thread = None
        self.should_cancel = False

        self.setup_gui()
        self.initialize_processor()

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
                self.process_button.config(state=tk.DISABLED)
                self.cancel_button.config(state=tk.NORMAL)
                self.should_cancel = False

                with open(input_path, 'r', encoding='utf-8') as f:
                    data = json.load(f)

                total_files = len(data['files'])
                processed_files = []

                for i, file_data in enumerate(data['files'], 1):
                    if self.should_cancel:
                        self.log_message("처리가 취소되었습니다.")
                        break

                    progress = (i / total_files) * 100
                    self.progress_var.set(progress)
                    self.current_file_var.set(f"처리 중: {file_data['file_path']}")

                    processed = self.processor.process_file(
                        file_data['content'],
                        file_data['file_path'],
                        max_length
                    )
                    processed_files.append(processed)

                    self.log_message(f"처리 완료: {file_data['file_path']}")

                if not self.should_cancel and processed_files:
                    output_data = {
                        'metadata': {
                            **data['metadata'],
                            'processed_files': len(processed_files),
                            'model_name': "meta-llama/Llama-3.1-405B-Instruct",
                            'max_length': max_length,
                            'processing_date': datetime.now().isoformat()
                        },
                        'processed_data': processed_files
                    }

                    with open(output_path, 'w', encoding='utf-8') as f:
                        json.dump(output_data, f, indent=2, ensure_ascii=False)

                    self.status_var.set("처리 완료!")
                    self.log_message("모든 파일 처리가 완료되었습니다.")
                    messagebox.showinfo("완료", "모든 파일의 처리가 완료되었습니다.")

            except Exception as e:
                self.status_var.set(f"오류 발생: {str(e)}")
                self.log_message(f"오류 발생: {str(e)}")
                messagebox.showerror("오류", str(e))

            finally:
                self.process_button.config(state=tk.NORMAL)
                self.cancel_button.config(state=tk.DISABLED)
                self.progress_var.set(0)
                self.current_file_var.set("")
                self.should_cancel = False

        # 처리를 별도의 스레드에서 실행
        self.processing_thread = threading.Thread(target=process, daemon=True)
        self.processing_thread.start()

    def setup_gui(self):
        main_container = ttk.Frame(self.window, padding="10")
        main_container.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))

        # 입력 섹션
        input_frame = ttk.LabelFrame(main_container, text="입력/출력 설정", padding="5")
        input_frame.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S), pady=5)

        ttk.Label(input_frame, text="입력 JSON 파일:").grid(row=0, column=0, sticky=tk.W, pady=2)
        self.input_path_var = tk.StringVar()
        ttk.Entry(input_frame, textvariable=self.input_path_var, width=60).grid(row=0, column=1, padx=5)
        ttk.Button(input_frame, text="찾아보기", command=self.browse_input_file).grid(row=0, column=2)

        ttk.Label(input_frame, text="출력 JSON 파일:").grid(row=1, column=0, sticky=tk.W, pady=2)
        self.output_path_var = tk.StringVar()
        ttk.Entry(input_frame, textvariable=self.output_path_var, width=60).grid(row=1, column=1, padx=5)
        ttk.Button(input_frame, text="찾아보기", command=self.browse_output_file).grid(row=1, column=2)

        # 설정 섹션
        settings_frame = ttk.LabelFrame(main_container, text="처리 설정", padding="5")
        settings_frame.grid(row=1, column=0, sticky=(tk.W, tk.E), pady=5)

        ttk.Label(settings_frame, text="최대 토큰 길이:").grid(row=0, column=0, sticky=tk.W)
        self.max_length_var = tk.StringVar(value="2048")
        ttk.Entry(settings_frame, textvariable=self.max_length_var, width=10).grid(row=0, column=1, padx=5)

        device_info = "GPU (CUDA)" if torch.cuda.is_available() else "CPU"
        ttk.Label(settings_frame, text=f"사용 중인 장치: {device_info}").grid(row=0, column=2, padx=20)

        # 상태 표시 섹션
        status_frame = ttk.LabelFrame(main_container, text="상태", padding="5")
        status_frame.grid(row=2, column=0, sticky=(tk.W, tk.E), pady=5)

        self.status_var = tk.StringVar(value="초기화 중...")
        ttk.Label(status_frame, textvariable=self.status_var, style='Status.TLabel').grid(row=0, column=0, sticky=tk.W)

        self.progress_var = tk.DoubleVar()
        self.progress_bar = ttk.Progressbar(
            status_frame,
            length=700,
            mode='determinate',
            variable=self.progress_var
        )
        self.progress_bar.grid(row=1, column=0, pady=5)

        self.current_file_var = tk.StringVar()
        ttk.Label(status_frame, textvariable=self.current_file_var, wraplength=700).grid(row=2, column=0, pady=2)

        # 로그 섹션
        log_frame = ttk.LabelFrame(main_container, text="처리 로그", padding="5")
        log_frame.grid(row=3, column=0, sticky=(tk.W, tk.E, tk.N, tk.S), pady=5)

        self.log_text = scrolledtext.ScrolledText(log_frame, height=15, width=80, wrap=tk.WORD)
        self.log_text.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))

        # 제어 버튼 섹션
        control_frame = ttk.Frame(main_container)
        control_frame.grid(row=4, column=0, pady=10)

        self.process_button = ttk.Button(
            control_frame,
            text="처리 시작",
            command=self.start_processing,
            state=tk.DISABLED
        )
        self.process_button.pack(side=tk.LEFT, padx=5)

        self.cancel_button = ttk.Button(
            control_frame,
            text="취소",
            command=self.cancel_processing,
            state=tk.DISABLED
        )
        self.cancel_button.pack(side=tk.LEFT, padx=5)

        ttk.Button(control_frame, text="로그 지우기", command=self.clear_log).pack(side=tk.LEFT, padx=5)

        # 윈도우 크기 조절 설정
        self.window.columnconfigure(0, weight=1)
        self.window.rowconfigure(0, weight=1)
        main_container.columnconfigure(0, weight=1)

    def initialize_processor(self):
        def init():
            try:
                self.processor = LlamaCodeProcessor()
                self.window.after(0, lambda: self.status_var.set("준비 완료"))
                self.window.after(0, lambda: self.process_button.config(state=tk.NORMAL))
            except Exception as e:
                error_msg = str(e)  # 예외 메시지를 로컬 변수에 저장
                self.window.after(0, lambda: self.status_var.set(f"초기화 실패: {error_msg}"))
                self.window.after(0, lambda: messagebox.showerror("초기화 오류", error_msg))

        self.log_message("Llama 모델 초기화 중...")
        threading.Thread(target=init, daemon=True).start()

    def browse_input_file(self):
        filename = filedialog.askopenfilename(
            title="입력 JSON 파일 선택",
            filetypes=[("JSON files", "*.json")]
        )
        if filename:
            self.input_path_var.set(filename)
            self.log_message(f"입력 파일 선택됨: {filename}")

    def browse_output_file(self):
        filename = filedialog.asksaveasfilename(
            title="출력 JSON 파일 선택",
            defaultextension=".json",
            filetypes=[("JSON files", "*.json")]
        )
        if filename:
            self.output_path_var.set(filename)
            self.log_message(f"출력 파일 선택됨: {filename}")

    def log_message(self, message: str):
        timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        self.log_text.insert(tk.END, f"[{timestamp}] {message}\n")
        self.log_text.see(tk.END)

    def clear_log(self):
        self.log_text.delete(1.0, tk.END)
        self.log_message("로그가 지워졌습니다.")

    def cancel_processing(self):
        self.should_cancel = True
        self.log_message("처리 취소 요청됨...")
        self.status_var.set("취소 중...")

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
                self.process_button.config(state=tk.DISABLED)
                self.cancel_button.config(state=tk.NORMAL)
                self.should_cancel = False

                with open(input_path, 'r', encoding='utf-8') as f:
                    data = json.load(f)

                total_files = len(data['files'])
                processed_files = []

                for i, file_data in enumerate(data['files'], 1):
                    if self.should_cancel:
                        self.log_message("처리가 취소되었습니다.")
                        break

                    progress = (i / total_files) * 100
                    self.progress_var.set(progress)
                    self.current_file_var.set(f"처리 중: {file_data['file_path']}")

                    processed = self.processor.process_file(
                        file_data['content'],
                        file_data['file_path'],
                        max_length
                    )
                    processed_files.append(processed)

                    self.log_message(f"처리 완료: {file_data['file_path']}")

                if not self.should_cancel and processed_files:
                    output_data = {
                        'metadata': {
                            **data['metadata'],
                            'processed_files': len(processed_files),
                            'model_name': "meta-llama/Llama-2-7b-code",
                            'max_length': max_length,
                            'processing_date': datetime.now().isoformat()
                        },
                        'processed_data': processed_files
                    }

                    with open(output_path, 'w', encoding='utf-8') as f:
                        json.dump(output_data, f, indent=2, ensure_ascii=False)

                    self.status_var.set("처리 완료!")
                    self.log_message("모든 파일 처리가 완료되었습니다.")
                    messagebox.showinfo("완료", "모든 파일의 처리가 완료되었습니다.")

            except Exception as e:
                self.status_var.set(f"오류 발생: {str(e)}")
                self.log_message(f"오류 발생: {str(e)}")
                messagebox.showerror("오류", str(e))

            finally:
                self.process_button.config(state=tk.NORMAL)
                self.cancel_button.config(state=tk.DISABLED)
                self.progress_var.set(0)
                self.current_file_var.set("")
                self.should_cancel = False

def main():
    app = LlamaCodeProcessorGUI()
    # 윈도우 종료 이벤트 처리
    app.window.protocol("WM_DELETE_WINDOW", app.window.quit)
    # 메인 루프 시작
    app.window.mainloop()

if __name__ == "__main__":
    main()