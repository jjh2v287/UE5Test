import json
from langchain.text_splitter import RecursiveCharacterTextSplitter
import google.generativeai as genai
from langchain_community.vectorstores import FAISS
from langchain.embeddings.base import Embeddings
from langchain_google_genai import ChatGoogleGenerativeAI
from langchain.chains import ConversationalRetrievalChain
from langchain.memory import ConversationBufferMemory
import numpy as np
import os


# Google API 키 설정
GOOGLE_API_KEY = "AIzaSyArl1FcFn9wyaUIZZaMij-QsnI4EGW0Aik"

if "GOOGLE_API_KEY" not in os.environ:
    os.environ["GOOGLE_API_KEY"] = GOOGLE_API_KEY

def read_json_file(file_path: str) -> dict:
    """JSON 파일을 읽어서 데이터를 반환합니다."""
    with open(file_path, 'r', encoding='utf-8') as file:
        data = json.load(file)
    return data


def extract_code_contents(json_data: dict) -> list:
    """JSON 데이터에서 코드 내용을 추출합니다."""
    documents = []
    for file in json_data["files"]:
        # 각 파일의 정보를 포함하여 문서 생성
        document = {
            "content": file["content"],
            "metadata": {
                "file_path": file["file_path"],
                "extension": file["extension"],
                "lines": file["lines"]
            }
        }
        documents.append(document)

    return documents


def split_documents(documents: list) -> list:
    """문서를 더 작은 청크로 분할합니다."""

    # 텍스트 분할기 초기화
    text_splitter = RecursiveCharacterTextSplitter(
        chunk_size=1000,  # 각 청크의 최대 크기
        chunk_overlap=200,  # 청크 간 중복되는 문자 수
        length_function=len,
        separators=["\n\n", "\n", " ", ""]  # 분할 기준
    )

    chunks = []
    for doc in documents:
        # 각 청크에 원본 파일 메타데이터 유지
        splits = text_splitter.split_text(doc["content"])
        for split in splits:
            chunks.append({
                "content": split,
                "metadata": doc["metadata"]
            })

    return chunks


class DummyEmbeddings(Embeddings):
    """이미 임베딩된 벡터를 사용할 때 쓸 더미 임베딩 클래스"""

    def embed_documents(self, texts: list[str]) -> list[list[float]]:
        """실제로는 호출되지 않을 메서드"""
        return [[0.0] * 768] * len(texts)  # 임베딩 차원에 맞게 조정

    def embed_query(self, text: str) -> list[float]:
        """실제로는 호출되지 않을 메서드"""
        return [0.0] * 768  # 임베딩 차원에 맞게 조정

def create_vector_store(chunks: list, api_key: str):
    """청크로부터 임베딩을 생성하고 벡터 저장소를 구축합니다."""

    # Google API 설정
    genai.configure(api_key=api_key)

    # 더미 임베딩 객체 생성
    dummy_embeddings = DummyEmbeddings()

    # 임베딩 모델 초기화
    # embeddings = genai.GenerativeModel(
    #     model_name='models/text-embedding-004'  # Google의 임베딩 모델
    # )

    # 텍스트와 메타데이터 분리
    texts = [chunk["content"] for chunk in chunks]
    metadatas = [chunk["metadata"] for chunk in chunks]

    # 임베딩 데이터 생성
    embedding_vectors = genai.embed_content(
        model="models/text-embedding-004",  # Google의 임베딩 모델
        content=texts,                      # 필수 항목입니다. 삽입할 콘텐츠입니다.
        output_dimensionality=768           # 임베딩 기본 768차원 벡터
    )

    # 임베딩 배열을 float 리스트로 변환
    embeddings = []
    for embedding in embedding_vectors['embedding']:
        # numpy array를 일반 리스트로 변환
        if isinstance(embedding, np.ndarray):
            embedding = embedding.tolist()
        embeddings.append(embedding)

    """미리 계산된 임베딩으로 벡터 저장소를 생성합니다."""
    # FAISS 벡터 저장소 생성
    text_embeddings = list(zip(texts, embeddings))
    vector_store = FAISS.from_embeddings(
        text_embeddings=text_embeddings,
        metadatas=metadatas,
        embedding=dummy_embeddings
    )

    return vector_store


class CodebaseRAG:
    def __init__(self, vector_store, api_key: str):
        """RAG 시스템을 초기화합니다."""
        self.vector_store = vector_store
        self.api_key = api_key
        self.chain = self._create_conversation_chain()

    def _create_conversation_chain(self):
        """대화형 검색 체인을 생성합니다."""

        # Gemini 모델 초기화
        llm = ChatGoogleGenerativeAI(
            model="gemini-1.5-pro",
            temperature=0,
            max_tokens=None,
            timeout=None,
            max_retries=2,
            # other params...
        )
        # llm = ChatGoogleGenerativeAI(
        #     model="gemini-pro",
        #     google_api_key=self.api_key,
        #     temperature=0.7,
        #     convert_system_message_to_human=True
        # )

        # 대화 기록 관리를 위한 메모리 설정
        memory = ConversationBufferMemory(
            memory_key="chat_history",
            return_messages=True,
            output_key='answer'  # 'answer' 키를 저장하도록 지정
        )

        # 대화형 검색 체인 생성
        chain = ConversationalRetrievalChain.from_llm(
            llm=llm,
            retriever=self.vector_store.as_retriever(search_kwargs={"k": 3}),
            memory=memory,
            return_source_documents=True
        )

        return chain

    def query(self, question: str) -> dict:
        """질문에 대한 답변을 생성합니다."""
        response = self.chain.invoke({"question": question})

        return {
            "answer": response["answer"],
            "source_documents": [
                {
                    "content": doc.page_content,
                    "metadata": doc.metadata
                }
                for doc in response["source_documents"]
            ]
        }

# 사용 예시
if __name__ == "__main__":
    # Google API 키 설정
    # GOOGLE_API_KEY = "AIzaSyArl1FcFn9wyaUIZZaMij-QsnI4EGW0Aik"

    # 파일 읽기
    json_data = read_json_file('RAG/RAGTestData.json')

    # 1단계: 코드 내용 추출
    documents = extract_code_contents(json_data)

    # 2단계: 청크 분할
    chunks = split_documents(documents)

    # 3단계: 벡터 저장소 생성
    vector_store = create_vector_store(chunks, GOOGLE_API_KEY)

    # 간단한 검색 테스트
    # query = "PawnAction class의 목적은 무엇인가요?"
    # results = vector_store.similarity_search(query, k=2)

    # 이전 단계에서 생성한 vector_store를 사용하여 RAG 시스템 초기화
    rag_system = CodebaseRAG(vector_store, GOOGLE_API_KEY)

    # 예시 질문들로 테스트
    questions = [
        "PawnAction 클래스의 주요 기능은 무엇인가요?"
    ]

    # 각 질문에 대한 응답 생성
    for question in questions:
        print(f"\n질문: {question}")
        response = rag_system.query(question)

        print("\n답변:", response["answer"])
        print("\n참조한 파일들:")
        for doc in response["source_documents"]:
            print(f"- 파일: {doc['metadata']['file_path']}")
            print(f"- 관련 내용: {doc['content'][:200]}...\n")


def search_codebase(query: str):
    """코드베이스 검색을 위한 편의 함수"""
    rag_system = CodebaseRAG(vector_store, GOOGLE_API_KEY)
    response = rag_system.query(query)

    print(f"\n질문: {query}")
    print("\n답변:", response["answer"])
    print("\n참조한 소스:")
    for doc in response["source_documents"]:
        print(f"\n파일: {doc['metadata']['file_path']}")
        print(f"코드 조각:\n{doc['content'][:200]}...")