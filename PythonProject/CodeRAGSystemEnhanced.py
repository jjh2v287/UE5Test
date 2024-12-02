from langchain.embeddings.base import Embeddings
from typing import TypedDict, List, Dict, Annotated, Set
from langchain.schema import Document
from langchain.schema.messages import BaseMessage, HumanMessage
from langchain_google_genai import ChatGoogleGenerativeAI
from langchain_community.vectorstores import FAISS
from langgraph.graph import END, START, StateGraph
from dataclasses import dataclass
import operator
import re
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

@dataclass
class SearchResult:
    document: Document
    similarity: float
    relevance_score: float


# 상태에 검색 이력 추가
class EnhancedGraphState(TypedDict):
    messages: Annotated[List[BaseMessage], operator.add]
    context: List[Document]
    current_question: str
    search_results: List[SearchResult]
    intermediate_thoughts: Annotated[List[str], operator.add]
    confidence_score: float
    iteration_count: int
    searched_docs: Set[str]  # 검색된 문서 추적
    question_variations: List[str]  # 질문 변형 이력


class EnhancedCodebaseRAGSystem:
    def __init__(self, vector_store_path: str, api_key: str,
                 max_iterations: int = 3,
                 min_confidence_threshold: float = 0.8):
        self.api_key = api_key
        self.max_iterations = max_iterations
        self.min_confidence_threshold = min_confidence_threshold
        # 벡터 스토어 로드
        self.vector_store = FAISS.load_local(
            vector_store_path,
            DummyEmbeddings(),
            allow_dangerous_deserialization=True  # 이 줄을 추가
        )
        self.llm = ChatGoogleGenerativeAI(model="gemini-1.5-pro", temperature=0.2)
        self.graph = self._create_enhanced_graph()

    def _should_continue(self, state: EnhancedGraphState) -> str:
        """AI 모델을 활용하여 검색 결과의 품질을 평가하고 다음 단계를 결정"""

        # 검색 결과가 없는 경우
        if not state["search_results"]:
            print("\n검색 결과 없음 - 검색 재시도")
            return "retrieve"

        # 현재까지의 검색 결과 요약
        result_summaries = []
        for idx, result in enumerate(state["search_results"][:5], 1):
            summary = f"""
            문서 {idx}:
            파일: {result.document.metadata.get('file_path', 'Unknown')}
            관련도: {result.relevance_score:.2f}
            내용 미리보기: {result.document.page_content[:200]}
            """
            result_summaries.append(summary)

        # AI 모델에 평가 요청
        evaluation_prompt = f"""
        다음 검색 결과들이 질문에 대해 충분한 정보를 제공하는지 평가해주세요:

        질문: {state["current_question"]}

        현재 반복 횟수: {state["iteration_count"]}
        검색된 문서:
        {"\n".join(result_summaries)}

        다음 기준으로 평가해주세요:
        1. 검색 결과가 질문과 얼마나 관련이 있는지 (0-1)
        2. 코드 구현 세부사항을 충분히 포함하는지 (0-1)
        3. 추가 검색이 필요한지 (Yes/No)
        4. 응답 생성을 위한 정보가 충분한지 (Yes/No)

        응답 형식:
        관련성 점수: [0-1]
        구현 상세 점수: [0-1]
        추가 검색 필요: [Yes/No]
        정보 충분성: [Yes/No]
        근거: [평가 근거 설명]
        """

        response = self.llm.invoke([HumanMessage(content=evaluation_prompt)])

        # 응답 파싱
        try:
            content = response.content
            relevance_score = float(re.search(r'관련성 점수:\s*(0?\.\d+|1\.0)', content).group(1))
            detail_score = float(re.search(r'구현 상세 점수:\s*(0?\.\d+|1\.0)', content).group(1))
            needs_more = 'Yes' in re.search(r'추가 검색 필요:\s*(Yes|No)', content).group(1)
            info_sufficient = 'Yes' in re.search(r'정보 충분성:\s*(Yes|No)', content).group(1)
        except:
            print("\n응답 파싱 오류 - 기본값 사용")
            relevance_score = 0.5
            detail_score = 0.5
            needs_more = True
            info_sufficient = False

        # 평가 결과 출력
        print(f"\nIteration {state['iteration_count']} AI 평가:")
        print(f"- 관련성 점수: {relevance_score:.3f}")
        print(f"- 구현 상세 점수: {detail_score:.3f}")
        print(f"- 추가 검색 필요: {'예' if needs_more else '아니오'}")
        print(f"- 정보 충분성: {'예' if info_sufficient else '아니오'}")

        # 결정 로직
        quality_sufficient = (
                (relevance_score >= 0.7 and detail_score >= 0.7) or
                info_sufficient or
                state["iteration_count"] >= self.max_iterations
        )

        if quality_sufficient:
            print("→ 응답 생성으로 진행")
            return "generate"

        print("→ 검색 재시도")
        return "retrieve"

    def _generate_question_variation(self, original_question: str, iteration: int) -> str:
        """반복마다 질문을 변형하여 다른 관점의 검색 수행"""
        variations = {
            1: original_question,  # 첫 번째는 원본 질문 사용
            2: f"implementation details of {original_question}",  # 구현 세부사항 관점
            3: f"usage examples of {original_question}",  # 사용 예시 관점
            4: f"related classes and functions to {original_question}",  # 관련 클래스/함수 관점
            5: f"configuration and setup for {original_question}"  # 설정/환경 관점
        }
        return variations.get(iteration, f"alternative implementations of {original_question}")

    def _retrieve(self, state: EnhancedGraphState) -> Dict:
        """개선된 검색 노드"""
        iteration = state["iteration_count"]
        k = max(100 - (iteration * 20), 40)

        # 질문 변형
        varied_question = self._generate_question_variation(state["current_question"], iteration)

        # 검색 수행
        results = self.vector_store.similarity_search_with_score(varied_question, k=k)

        # 검색 결과 출력
        print(f"\n{'=' * 50}")
        print(f"검색 반복 {iteration} - 질의: {varied_question}")
        print(f"검색된 총 문서 수: {len(results)}")
        print(f"{'=' * 50}")

        for idx, (doc, score) in enumerate(results[:5], 1):
            print(f"\n문서 {idx}:")
            print(f"파일: {doc.metadata.get('file_path', 'Unknown')}")
            print(f"유사도 점수: {1.0 - score:.4f}")  # 거리를 유사도로 변환
            print(f"내용 미리보기: {doc.page_content[:200]}...")
            print(f"{'-' * 50}")

        # 이미 검색된 문서 필터링 및 가중치 계산
        weighted_results = []
        for doc, score in results:
            doc_id = f"{doc.metadata.get('file_path', 'Unknown')}:{doc.page_content[:50]}"

            # 이미 검색된 문서 건너뛰기
            if doc_id in state.get("searched_docs", set()):
                continue

            relevance_score = 1.0 - score  # 거리를 유사도로 변환

            # 코드 패턴 가중치
            code_patterns = {
                'class_def': r'class\s+\w+',
                'func_def': r'def\s+\w+',
                'import': r'import\s+\w+|from\s+\w+\s+import'
            }

            # 반복 횟수에 따른 가중치 조정
            iteration_weight = 1.0 + (iteration * 0.1)  # 후속 반복에서 새로운 결과 선호

            for pattern in code_patterns.values():
                if re.search(pattern, doc.page_content):
                    relevance_score *= 1.2

            relevance_score *= iteration_weight

            weighted_results.append(SearchResult(
                document=doc,
                similarity=score,
                relevance_score=relevance_score
            ))

        # 상위 결과 필터링
        filtered_results = sorted(
            weighted_results,
            key=lambda x: x.relevance_score,
            reverse=True
        )[:5]

        # 검색된 문서 ID 추가
        searched_docs = state.get("searched_docs", set())
        for result in filtered_results:
            doc_id = f"{result.document.metadata.get('file_path', 'Unknown')}:{result.document.page_content[:50]}"
            searched_docs.add(doc_id)

        thought = f"""Iteration {iteration}:
        - 변형된 질문: {varied_question}
        - 검색된 새로운 문서: {len(filtered_results)}개
        - 총 누적 검색 문서: {len(searched_docs)}개"""

        return {
            "search_results": filtered_results,
            "context": [r.document for r in filtered_results],
            "iteration_count": iteration + 1,
            "intermediate_thoughts": [thought],
            "searched_docs": searched_docs,
            "question_variations": state.get("question_variations", []) + [varied_question]
        }

    def _generate(self, state: EnhancedGraphState) -> Dict:
        """응답 생성 노드"""
        # 상위 문서 컨텍스트 구성
        contexts = []
        for result in state["search_results"][:5]:
            contexts.append(f"""
            파일: {result.document.metadata.get('file_path', 'Unknown')}
            관련도: {result.relevance_score:.2f}
            내용:
            {result.document.page_content}
            """)

        context_str = "\n---\n".join(contexts)
        thoughts_str = "\n".join(state["intermediate_thoughts"])

        prompt = f"""다음 정보를 바탕으로 질문에 답변해주세요:

        질문: {state["current_question"]}
        
        검색된 코드 문맥:
        {context_str}
        
        분석 내용:
        {thoughts_str}
        
        다음 사항을 포함하여 답변해주세요:
        1. 코드의 주요 기능과 목적
        2. 구현 세부사항과 사용된 기술
        3. 관련 파일들의 상호작용
        4. 주요 코드 예시 설명
        """

        response = self.llm.invoke([HumanMessage(content=prompt)])
        confidence_score = self._evaluate_response_quality(response.content, state["current_question"])

        return {
            "messages": [response],
            "confidence_score": confidence_score
        }

    def _evaluate_response_quality(self, response: str, question: str) -> float:
        """응답 품질 평가"""
        eval_prompt = f"""다음 응답의 품질을 평가해주세요:

        질문: {question}
        응답: {response}
        
        0부터 1 사이의 점수로 평가해주세요:
        - 정확성
        - 완성도
        - 코드 이해도
        - 설명 명확성"""

        eval_response = self.llm.invoke([HumanMessage(content=eval_prompt)])

        try:
            score_match = re.search(r'(?i)(?:score|점수):\s*(\d*\.?\d+)', eval_response.content)
            return float(score_match.group(1)) if score_match else 0.5
        except:
            return 0.5

    def _create_enhanced_graph(self) -> StateGraph:
        """워크플로우 그래프 생성"""
        workflow = StateGraph(EnhancedGraphState)

        # 노드 추가
        workflow.add_node("retrieve", self._retrieve)
        workflow.add_node("generate", self._generate)

        # 엣지 설정
        workflow.add_edge(START, "retrieve")

        # 조건부 엣지
        workflow.add_conditional_edges(
            "retrieve",
            self._should_continue,
            {
                "retrieve": "retrieve",
                "generate": "generate"
            }
        )

        workflow.add_edge("generate", END)

        return workflow.compile()

    def query(self, question: str) -> Dict:
        """질의 처리"""
        initial_state = EnhancedGraphState(
            messages=[],
            context=[],
            current_question=question,
            search_results=[],
            intermediate_thoughts=[],
            confidence_score=0.0,
            iteration_count=1,
            searched_docs=set(),  # 검색된 문서 추적을 위한 집합
            question_variations=[]  # 질문 변형 이력
        )

        final_state = self.graph.invoke(initial_state)

        return {
            "answer": final_state["messages"][-1].content,
            "confidence": final_state["confidence_score"],
            "iterations": final_state["iteration_count"],
            "question_variations": final_state["question_variations"],
            "sources": [
                {
                    "file": result.document.metadata.get('file_path', 'Unknown'),
                    "relevance": result.relevance_score,
                    "content": result.document.page_content[:200] + "..."
                }
                for result in final_state["search_results"][:3]
            ]
        }


# 사용 예시
if __name__ == "__main__":
    VECTOR_STORE_PATH = "vector_store"

    # RAG 시스템 초기화 - 최대 3번 반복, 최소 신뢰도 0.8
    rag_system = EnhancedCodebaseRAGSystem(
        VECTOR_STORE_PATH,
        GOOGLE_API_KEY,
        max_iterations=10,
        min_confidence_threshold=0.8
    )

    # 테스트 질문
    question = "MassAI 모듈의 주요 기능을 알려주세요"
    response = rag_system.query(question)

    # 결과 출력
    print(f"\n원본 질문: {question}")
    print(f"\n총 반복 횟수: {response['iterations']}")
    print(f"최종 신뢰도 점수: {response['confidence']:.2f}")
    print(f"\n총 반복 횟수: {response['answer']}")