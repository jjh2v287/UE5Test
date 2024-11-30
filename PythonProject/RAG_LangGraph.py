import json
from typing import TypedDict, Annotated, Sequence
from typing import Dict, List, Tuple, Any

from langchain.text_splitter import RecursiveCharacterTextSplitter
import google.generativeai as genai
from langchain_community.vectorstores import FAISS
from langchain.embeddings.base import Embeddings
from langchain_google_genai import ChatGoogleGenerativeAI
from langchain.schema import Document
from langchain.schema.messages import BaseMessage, HumanMessage, AIMessage
from langchain.memory import ConversationBufferMemory
import numpy as np
import os

from langgraph.graph import Graph, StateGraph
from langgraph.prebuilt import ToolExecutor

# Google API 키 설정
GOOGLE_API_KEY = "AIzaSyArl1FcFn9wyaUIZZaMij-QsnI4EGW0Aik"
os.environ["GOOGLE_API_KEY"] = GOOGLE_API_KEY


# 상태 정의
class GraphState(TypedDict):
    """대화 그래프의 상태를 정의하는 클래스"""
    messages: List[BaseMessage]
    context: List[Document]
    current_question: str
    summary: str


# 기존의 유틸리티 함수들은 그대로 유지
def read_json_file(file_path: str) -> dict:
    with open(file_path, 'r', encoding='utf-8') as file:
        return json.load(file)


def extract_code_contents(json_data: dict) -> list:
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
    return documents


def split_documents(documents: list) -> list:
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
    return chunks


class DummyEmbeddings(Embeddings):
    def embed_documents(self, texts: list[str]) -> list[list[float]]:
        return [[0.0] * 768] * len(texts)

    def embed_query(self, text: str) -> list[float]:
        return [0.0] * 768

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

# LangGraph 노드 함수들
def retrieve_context(state: GraphState) -> GraphState:
    """문맥 검색 노드"""
    question = state["current_question"]
    results = vector_store.similarity_search(question, k=3)
    state["context"] = results
    return state


def generate_response(state: GraphState) -> GraphState:
    """응답 생성 노드"""
    llm = ChatGoogleGenerativeAI(
        model="gemini-1.5-pro",
        temperature=0,
        max_tokens=None,
    )

    # 컨텍스트 준비
    context_str = "\n".join([doc.page_content for doc in state["context"]])

    # 프롬프트 구성
    messages = [
        HumanMessage(content=f"""다음 컨텍스트를 바탕으로 질문에 답변해주세요:

컨텍스트:
{context_str}

질문: {state["current_question"]}""")
    ]

    # 응답 생성
    response = llm.invoke(messages)
    state["messages"].append(HumanMessage(content=state["current_question"]))
    state["messages"].append(response)

    return state


class CodebaseRAGGraph:
    def __init__(self, vector_store):
        self.vector_store = vector_store
        self.graph = self._create_graph()

    def _create_graph(self) -> Graph:
        """LangGraph 워크플로우 생성"""
        workflow = StateGraph(GraphState)

        # 노드 추가
        workflow.add_node("retrieve", retrieve_context)
        workflow.add_node("generate", generate_response)

        # 엣지 연결
        workflow.add_edge("retrieve", "generate")

        # 시작과 끝 설정
        workflow.set_entry_point("retrieve")
        workflow.set_finish_point("generate")

        return workflow.compile()

    def query(self, question: str) -> dict:
        """질문 처리"""
        initial_state = GraphState(
            messages=[],
            context=[],
            current_question=question,
            summary=""
        )

        # 그래프 실행
        final_state = self.graph.invoke(initial_state)

        # 결과 포맷팅
        return {
            "answer": final_state["messages"][-1].content,
            "source_documents": [
                {
                    "content": doc.page_content,
                    "metadata": doc.metadata
                }
                for doc in final_state["context"]
            ]
        }


# 사용 예시
if __name__ == "__main__":
    # 초기화
    json_data = read_json_file('RAG/RAGTestData.json')
    documents = extract_code_contents(json_data)
    chunks = split_documents(documents)
    vector_store = create_vector_store(chunks, GOOGLE_API_KEY)

    # LangGraph 기반 RAG 시스템 초기화
    rag_system = CodebaseRAGGraph(vector_store)

    # 테스트 질문
    question = "PawnAction 클래스의 주요 기능은 무엇인가요?"
    response = rag_system.query(question)

    print(f"\n질문: {question}")
    print("\n답변:", response["answer"])
    print("\n참조한 파일들:")
    for doc in response["source_documents"]:
        print(f"- 파일: {doc['metadata']['file_path']}")
        print(f"- 관련 내용: {doc['content'][:200]}...\n")