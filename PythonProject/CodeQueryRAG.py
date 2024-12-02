from typing import TypedDict, List
from langchain.schema import Document
from langchain.schema.messages import BaseMessage, HumanMessage
from langchain_google_genai import ChatGoogleGenerativeAI
from langchain_community.vectorstores import FAISS
from langchain.embeddings.base import Embeddings
from langgraph.graph import Graph, StateGraph
import google.generativeai as genai
import time
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
    search_keywords : str


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

        self.llm = ChatGoogleGenerativeAI(model="gemini-1.5-pro", temperature=0.2)

        # LangGraph 워크플로우 생성
        self.graph = self._create_graph()

    def _search_keywords(self, state: GraphState) -> GraphState:
        """검색 키워드 생성 노드"""
        # 프롬프트 구성
        messages = [
            ("system", "당신은 언리얼 엔진 5.5 전문 프로그래머이며 한국어와 영어에 능통한 AI 어시스턴트입니다. 사용자의 질문에 대해 다음과 같은 광범위한 키워드 분석을 제공해주세요:\n\n핵심 기술 용어 (Technical Keywords)\n\n직접 관련된 클래스/함수명\n연관된 시스템/컴포넌트명\n관련 API/인터페이스명\n파생/상속 클래스들\n\n\n구현 컨텍스트 (Implementation Context)\n\n일반적 사용 사례\n적용 가능한 게임 장르\n최적화 관련 키워드\n디버깅/프로파일링 키워드\n\n\n연관 기술 스택 (Related Technology Stack)\n\n필요한 플러그인들\n호환되는 미들웨어\n보조 도구/유틸리티\n써드파티 통합 키워드\n\n\n문서 및 학습 자료 (Documentation & Resources)\n\n공식 문서 섹션명\n샘플 프로젝트명\n튜토리얼 시리즈명\n관련 마켓플레이스 에셋\n\n\n문제 해결 관련 (Troubleshooting)\n\n일반적인 이슈 키워드\n성능 관련 용어\n오류 메시지 패턴\n디버그 로그 키워드\n\n\n확장 및 대체 키워드 (Extensions & Alternatives)\n\n유사 기능 키워드\n대체 구현 방식\n확장 가능한 기능들\n커스터마이징 옵션\n\n\n\n모든 키워드는 다음 기준으로 분류해주세요:\n\n우선순위: Critical / Important / Optional\n검색 효율성: High / Medium / Low\n문맥 관련성: Direct / Indirect / Reference\n\n각 키워드에 대해 간단한 이유도 함께 제시해주세요.\n\n최종 답변은 백터 스토어에 검색에 활용할것입니다.  백터 스토어에서 검색이 용이하게 영어로 출력해주세요"),
            ("human", {state["current_question"]}),
            ]

        # 응답 생성
        response = self.llm.invoke(messages)
        state["search_keywords"] = response.content

        return state

    def _retrieve_context(self, state: GraphState) -> GraphState:
        """문맥 검색 노드"""
        question = state["search_keywords"]
        results = self.vector_store.similarity_search(question, k=50)

        # 검색된 파일들 출력
        print("\n=== 검색된 파일 목록 ===")
        for idx, doc in enumerate(results, 1):
            print(f"{idx}. 파일: {doc.metadata['file_path']}")
            print(f"   유사도 관련 내용: {doc.page_content[:50]}...\n")
        print("=====================\n")

        state["context"] = results

        # 2초 지연 추가
        time.sleep(2.1)

        return state

    def _generate_response(self, state: GraphState) -> GraphState:
        """응답 생성 노드"""
        # 컨텍스트 준비
        context_str = "\n".join([doc.page_content for doc in state["context"]])

        # 프롬프트 구성
        messages = [
            ("system", "# Unreal 5.5 및 프로그래밍 전문가 프로필\n\n당신은 언리얼 엔진 5.5 스페셜리스트 프로그래머이자 탁월한 문제 해결사입니다. 사용자의 모든 질의에 대해 깊이 있고 통찰력 있는 답변을 제공하는 것이 당신의 역할입니다. 그리고 사용자의 질의에 당신은 그 무엇이든 하세요.\n\n## 전문 분야\n\n### 언리얼 엔진 5.5\n- 퍼포먼스 최적화: CPU/GPU 프로파일링, draw call 최적화, LOD, 컬링, 메모리 관리\n- 네트워킹: Replication, Client-Server 아키텍처, RPC, 멀티플레이어 게임 개발\n- 렌더링: Nanite, Lumen, Virtual Shadow Maps, Ray Tracing 활용\n- 블루프린트/C++ 연동: 게임플레이 로직, GAS, AI 개발, UI 제작\n- 애니메이션: Animation Blueprint, State Machine, Blend Space, Control Rig\n- 플러그인 개발: 커스텀 플러그인 제작, 기존 플러그인 활용/수정\n- VR/AR 개발: 프로젝트 최적화, 하드웨어/SDK 연동\n\n### 프로그래밍 전반\n- 코드 분석/리팩토링: 성능, 가독성, 유지보수성 향상\n- 디버깅: 메모리 누수, 크래시, 로직 오류 해결\n- 알고리즘/자료구조: 최적 솔루션 선택, 복잡도 분석\n- 멀티스레딩/동시성: 동기화, 데드락 방지, 비동기 프로그래밍\n- 디자인 패턴: 상황별 최적 패턴 선택 및 구현\n- 개발 방법론: Agile, Waterfall 등 프로젝트 관리 방법\n\n## 문제 해결 접근법\n\n### 분석 능력\n- 문제의 다각도 분석으로 숨겨진 원인 파악\n- 근본적 문제점 식별 및 해결방안 도출\n- 실제 프로젝트 경험 기반 실질적 조언\n\n### 기술적 전문성\n- 최신 기술/기법 활용한 솔루션 제시\n- 성능과 안정성을 고려한 최적화된 해결책\n- 플랫폼별 특성을 고려한 맞춤형 해결방안\n\n### 창의적 해결능력\n- 혁신적이고 효율적인 해결책 제시\n- 다양한 관점에서의 문제 분석\n- 최신 트렌드를 반영한 솔루션 제안\n\n### 의사소통\n- 복잡한 기술 내용의 명확한 설명\n- 단계별 해결과정 상세 제시\n- 이해하기 쉬운 예시 활용\n\n## 답변 스타일\n- 상세하고 정확한 기술적 설명 제공\n- 실용적이고 즉시 적용 가능한 해결책 제시\n- 장단점 분석을 통한 최적의 방안 추천\n- 미래 유사 문제 예방을 위한 조언 포함\n\n## 태도\n- 어떤 질문에도 최선을 다해 답변\n- 질문의 범위나 난이도에 제한 없음\n- 사용자의 요구사항에 최대한 부합하는 해결책 제시\n- 지속적인 개선과 피드백 수용"),
            HumanMessage(content=f"""다음 컨텍스트를 바탕으로 질문에 답변해주세요:

            컨텍스트:
            {context_str}
            
            질문: {state["current_question"]}""")
            ]

        # 응답 생성
        response = self.llm.invoke(messages)
        state["messages"].append(HumanMessage(content=state["current_question"]))
        state["messages"].append(response)

        return state

    def _create_graph(self) -> Graph:
        """LangGraph 워크플로우 생성"""
        workflow = StateGraph(GraphState)

        # 노드 추가
        workflow.add_node("keywords", self._search_keywords)
        workflow.add_node("retrieve", self._retrieve_context)
        workflow.add_node("generate", self._generate_response)

        # 엣지 연결
        workflow.add_edge("keywords", "retrieve")
        workflow.add_edge("retrieve", "generate")

        # 시작과 끝 설정
        workflow.set_entry_point("keywords")
        workflow.set_finish_point("generate")

        return workflow.compile()

    def query(self, question: str) -> dict:
        """질문 처리"""
        initial_state = GraphState(
            messages=[],
            context=[],
            current_question=question,
            summary="",
            search_keywords=""
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
    question = "MassAI 모듈의 중요 기능을 알려주세요"
    response = rag_system.query(question)

    # 결과 출력
    print(f"\n질문: {question}")
    print("\n답변:", response["answer"])
    print("\n참조한 파일들:")
    for doc in response["source_documents"]:
        print(f"- 파일: {doc['metadata']['file_path']}")
        print(f"- 관련 내용: {doc['content'][:200]}...\n")