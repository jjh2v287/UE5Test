from typing import TypedDict, List
from langchain.schema import Document
from langchain.schema.messages import BaseMessage, HumanMessage
from langchain_google_genai import ChatGoogleGenerativeAI
from langchain_community.vectorstores import FAISS
from langchain.embeddings.base import Embeddings
from langgraph.graph import Graph, StateGraph
import google.generativeai as genai
import os

# Google API 키 설정
GOOGLE_API_KEY = "AIzaSyArl1FcFn9wyaUIZZaMij-QsnI4EGW0Aik"
os.environ["GOOGLE_API_KEY"] = GOOGLE_API_KEY

# 더미 임베딩 클래스 (벡터 스토어 로드용)
class DummyEmbeddings(Embeddings):
    def embed_documents(self, texts: List[str]) -> List[List[float]]:
        return [[0.0] * 768] * len(texts)

    def embed_query(self, text: str) -> List[float]:
        return [0.0] * 768


# 상태 정의
class GraphState(TypedDict):
    messages: List[BaseMessage]
    context: List[Document]
    current_question: str
    summary: str


class CodebaseRAGSystem:
    def __init__(self, vector_store_path: str, api_key: str):
        """RAG 시스템 초기화"""
        self.api_key = api_key
        genai.configure(api_key=api_key)

        # 벡터 스토어 로드
        self.vector_store = FAISS.load_local(
            vector_store_path,
            DummyEmbeddings(),
            allow_dangerous_deserialization=True  # 이 줄을 추가
        )

        # LangGraph 워크플로우 생성
        self.graph = self._create_graph()

    def _retrieve_context(self, state: GraphState) -> GraphState:
        """문맥 검색 노드"""
        question = state["current_question"]
        results = self.vector_store.similarity_search(question, k=100)
        state["context"] = results
        return state

    def _generate_response(self, state: GraphState) -> GraphState:
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

    def _create_graph(self) -> Graph:
        """LangGraph 워크플로우 생성"""
        workflow = StateGraph(GraphState)

        # 노드 추가
        workflow.add_node("retrieve", self._retrieve_context)
        workflow.add_node("generate", self._generate_response)

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


if __name__ == "__main__":
    GOOGLE_API_KEY = "AIzaSyArl1FcFn9wyaUIZZaMij-QsnI4EGW0Aik"
    VECTOR_STORE_PATH = "vector_store"

    # RAG 시스템 초기화
    rag_system = CodebaseRAGSystem(VECTOR_STORE_PATH, GOOGLE_API_KEY)

    # 테스트 질문
    question = "MassAI 모듈의 주요 기능을 알려주세요"
    response = rag_system.query(question)

    # 결과 출력
    print(f"\n질문: {question}")
    print("\n답변:", response["answer"])
    print("\n참조한 파일들:")
    for doc in response["source_documents"]:
        print(f"- 파일: {doc['metadata']['file_path']}")
        print(f"- 관련 내용: {doc['content'][:200]}...\n")