import json
import torch
from pathlib import Path
from tqdm.notebook import tqdm
import numpy as np
from typing import List, Dict
import logging
from datetime import datetime
from transformers import AutoTokenizer, AutoModelForCausalLM
import torch
from google.colab import files


class LlamaCodeProcessor:
    def __init__(self):
        # 토크나이저 및 모델 로드
        # self.tokenizer = AutoTokenizer.from_pretrained(model_id)
        # self.model = AutoModelForCausalLM.from_pretrained(
        #     model_id,
        #     torch_dtype=torch.float16,
        #     device_map="auto"
        # )


        self.device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
        self.setup_logging()

    def setup_logging(self):
        logging.basicConfig(
            level=logging.INFO,
            format='%(asctime)s - %(levelname)s - %(message)s',
            handlers=[
                logging.StreamHandler()
            ]
        )
        self.logger = logging.getLogger(__name__)

    def process_code(self, code: str, max_length: int = 4096) -> Dict:
        inputs = tokenizer(
            code,
            return_tensors="pt",
            max_length=max_length,
            truncation=True
        ).to(self.device)

        with torch.no_grad():
            outputs = model(**inputs, output_hidden_states=True)
            embeddings = outputs.hidden_states[-1].mean(dim=1)

        return {
            'embeddings': embeddings.cpu().numpy().tolist(),
            'tokens': inputs['input_ids'].cpu().numpy().tolist()
        }

    def _split_code_into_chunks(self, code: str, max_length: int) -> List[str]:
        tokens = tokenizer.tokenize(code)
        chunks = []
        current_chunk = []
        current_length = 0

        for token in tokens:
            if current_length >= max_length:
                chunks.append(tokenizer.convert_tokens_to_string(current_chunk))
                current_chunk = []
                current_length = 0

            current_chunk.append(token)
            current_length += 1

        if current_chunk:
            chunks.append(tokenizer.convert_tokens_to_string(current_chunk))

        return chunks

    def process_file(self, code_content: str, file_path: str, max_length: int) -> Dict:
        chunks = self._split_code_into_chunks(code_content, max_length)
        processed_chunks = []

        for chunk in tqdm(chunks, desc=f"Processing {file_path}"):
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


def process_files(input_json_path: str, max_length: int = 4096):
    """코드 파일들을 처리하는 메인 함수"""
    print("Initializing Llama model...")
    processor = LlamaCodeProcessor()
    print(f"Using device: {processor.device}")

    print("\nReading input JSON file...")
    with open(input_json_path, 'r', encoding='utf-8') as f:
        data = json.load(f)

    print(f"\nFound {len(data['files'])} files to process")
    processed_files = []

    for file_data in tqdm(data['files'], desc="Processing files"):
        processed = processor.process_file(
            file_data['content'],
            file_data['file_path'],
            max_length
        )
        processed_files.append(processed)

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

    output_path = 'processed_output.json'
    with open(output_path, 'w', encoding='utf-8') as f:
        json.dump(output_data, f, indent=2, ensure_ascii=False)

    print(f"\nProcessing complete! Output saved to {output_path}")
    files.download(output_path)  # Colab에서 자동으로 파일 다운로드


# 사용 예시:
# 1. JSON 파일 업로드
from google.colab import files

uploaded = files.upload()  # 입력 JSON 파일을 업로드하기 위한 위젯이 나타납니다

# 2. 업로드된 파일 처리
input_filename = list(uploaded.keys())[0]  # 업로드된 첫 번째 파일 사용
process_files(input_filename, max_length=4096)