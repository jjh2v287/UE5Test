import json
from typing import Dict, List
import os
import tkinter as tk
from tkinter import ttk, filedialog, messagebox
from langchain.text_splitter import RecursiveCharacterTextSplitter
from langchain.embeddings.base import Embeddings
from langchain_community.vectorstores import FAISS
import google.generativeai as genai
import numpy as np
import threading
from datetime import datetime


class DummyEmbeddings(Embeddings):
    """FAISS 저장을 위한 더미 임베딩 클래스"""

    def embed_documents(self, texts: List[str]) -> List[List[float]]:
        return [[0.0] * 768] * len(texts)

    def embed_query(self, text: str) -> List[float]:
        return [0.0] * 768


class VectorStoreCreatorGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("Vector Store Creator")
        self.root.geometry("600x400")

        # 스타일 설정
        style = ttk.Style()
        style.configure("Custom.TButton", padding=5)
        style.configure("Custom.TEntry", padding=5)

        # 메인 프레임
        self.main_frame = ttk.Frame(root, padding="10")
        self.main_frame.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))

        # API Key 입력
        ttk.Label(self.main_frame, text="Google API Key:").grid(row=0, column=0, sticky=tk.W, pady=5)
        self.api_key_var = tk.StringVar(value="AIzaSyArl1FcFn9wyaUIZZaMij-QsnI4EGW0Aik")  # 기본값 설정
        self.api_key_entry = ttk.Entry(self.main_frame, textvariable=self.api_key_var, width=50)
        self.api_key_entry.grid(row=0, column=1, columnspan=2, sticky=(tk.W, tk.E), pady=5)

        # 입력 파일 선택
        ttk.Label(self.main_frame, text="Input JSON File:").grid(row=1, column=0, sticky=tk.W, pady=5)
        self.input_file_var = tk.StringVar()
        self.input_file_entry = ttk.Entry(self.main_frame, textvariable=self.input_file_var, width=40)
        self.input_file_entry.grid(row=1, column=1, sticky=(tk.W, tk.E), pady=5)
        ttk.Button(self.main_frame, text="Browse", command=self.browse_input_file).grid(row=1, column=2, pady=5)

        # 출력 디렉토리 선택
        ttk.Label(self.main_frame, text="Output Directory:").grid(row=2, column=0, sticky=tk.W, pady=5)
        self.output_dir_var = tk.StringVar()
        self.output_dir_entry = ttk.Entry(self.main_frame, textvariable=self.output_dir_var, width=40)
        self.output_dir_entry.grid(row=2, column=1, sticky=(tk.W, tk.E), pady=5)
        ttk.Button(self.main_frame, text="Browse", command=self.browse_output_dir).grid(row=2, column=2, pady=5)

        # 진행 상황 표시
        self.progress_var = tk.StringVar(value="Ready")
        ttk.Label(self.main_frame, text="Status:").grid(row=3, column=0, sticky=tk.W, pady=5)
        ttk.Label(self.main_frame, textvariable=self.progress_var).grid(row=3, column=1, columnspan=2, sticky=tk.W,
                                                                        pady=5)

        # 진행바
        self.progress = ttk.Progressbar(self.main_frame, mode='determinate', length=400)
        self.progress.grid(row=4, column=0, columnspan=3, sticky=(tk.W, tk.E), pady=10)

        # 로그 표시 영역
        self.log_text = tk.Text(self.main_frame, height=10, width=70)
        self.log_text.grid(row=5, column=0, columnspan=3, pady=10)

        # 시작 버튼
        self.start_button = ttk.Button(self.main_frame, text="Start Processing",
                                       command=self.start_processing, style="Custom.TButton")
        self.start_button.grid(row=6, column=0, columnspan=3, pady=10)

    def browse_input_file(self):
        filename = filedialog.askopenfilename(
            title="Select input JSON file",
            filetypes=[("JSON files", "*.json"), ("All files", "*.*")]
        )
        if filename:
            self.input_file_var.set(filename)

    def browse_output_dir(self):
        dirname = filedialog.askdirectory(title="Select output directory")
        if dirname:
            self.output_dir_var.set(dirname)

    def log_message(self, message: str):
        timestamp = datetime.now().strftime("%H:%M:%S")
        self.log_text.insert(tk.END, f"[{timestamp}] {message}\n")
        self.log_text.see(tk.END)
        self.root.update_idletasks()

    def update_progress(self, message: str, progress_value: float = None):
        self.progress_var.set(message)
        if progress_value is not None:
            self.progress['value'] = progress_value
        self.root.update_idletasks()

    def process_data(self):
        try:
            # API Key 설정
            api_key = self.api_key_var.get()
            input_file = self.input_file_var.get()
            output_dir = self.output_dir_var.get()

            if not all([api_key, input_file, output_dir]):
                messagebox.showerror("Error", "Please fill in all fields")
                return

            genai.configure(api_key=api_key)

            # 데이터 로드
            self.update_progress("Loading JSON data...", 10)
            self.log_message("Reading JSON file...")
            with open(input_file, 'r', encoding='utf-8') as file:
                json_data = json.load(file)

            # 코드 콘텐츠 추출
            self.update_progress("Extracting code contents...", 20)
            self.log_message("Extracting code contents...")
            documents = []
            for file in json_data["files"]:
                document = {
                    "content": file["content"],
                    "metadata": {
                        "file_path": file["file_path"],
                        "extension": file["extension"],
                        "lines": file["lines"]
                    }
                }
                documents.append(document)

            # 문서 분할
            self.update_progress("Splitting documents...", 30)
            self.log_message("Splitting documents into chunks...")
            text_splitter = RecursiveCharacterTextSplitter(
                chunk_size=1000,
                chunk_overlap=200,
                length_function=len,
                separators=["\n\n", "\n", " ", ""]
            )

            chunks = []
            for doc in documents:
                splits = text_splitter.split_text(doc["content"])
                for split in splits:
                    chunks.append({
                        "content": split,
                        "metadata": doc["metadata"]
                    })

            # 임베딩 생성
            self.update_progress("Generating embeddings...", 50)
            self.log_message(f"Generating embeddings for {len(chunks)} chunks...")

            texts = [chunk["content"] for chunk in chunks]
            metadatas = [chunk["metadata"] for chunk in chunks]

            embedding_vectors = genai.embed_content(
                model="models/text-embedding-004",
                content=texts,
                output_dimensionality=768
            )

            self.update_progress("Processing embeddings...", 70)
            self.log_message("Processing embedding vectors...")

            embeddings = []
            for embedding in embedding_vectors['embedding']:
                if isinstance(embedding, np.ndarray):
                    embedding = embedding.tolist()
                embeddings.append(embedding)

            # 벡터 스토어 생성
            self.update_progress("Creating vector store...", 80)
            self.log_message("Creating FAISS vector store...")

            dummy_embeddings = DummyEmbeddings()
            text_embeddings = list(zip(texts, embeddings))
            vector_store = FAISS.from_embeddings(
                text_embeddings=text_embeddings,
                metadatas=metadatas,
                embedding=dummy_embeddings
            )

            # 저장
            self.update_progress("Saving vector store...", 90)
            self.log_message(f"Saving vector store to {output_dir}...")

            os.makedirs(output_dir, exist_ok=True)
            vector_store.save_local(output_dir)

            self.update_progress("Complete!", 100)
            self.log_message("Vector store creation completed successfully!")
            messagebox.showinfo("Success", "Vector store has been created successfully!")

        except Exception as e:
            self.log_message(f"Error: {str(e)}")
            messagebox.showerror("Error", f"An error occurred: {str(e)}")
            self.update_progress("Failed")

        finally:
            self.start_button['state'] = 'normal'

    def start_processing(self):
        self.start_button['state'] = 'disabled'
        self.progress['value'] = 0
        self.log_text.delete(1.0, tk.END)
        self.log_message("Starting vector store creation process...")

        # 별도 스레드에서 처리 실행
        threading.Thread(target=self.process_data, daemon=True).start()


if __name__ == "__main__":
    root = tk.Tk()
    app = VectorStoreCreatorGUI(root)
    root.mainloop()