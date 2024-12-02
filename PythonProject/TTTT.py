from typing import TypedDict, List, Dict, Tuple, Any, Annotated
from langchain.schema import Document
from langchain.schema.messages import BaseMessage, HumanMessage, AIMessage
from langchain_google_genai import ChatGoogleGenerativeAI
from langchain_community.vectorstores import FAISS
from langchain.embeddings.base import Embeddings
from langgraph.graph import Graph, StateGraph
import google.generativeai as genai
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


class EnhancedGraphState(TypedDict):
    messages: Annotated[List[BaseMessage], operator.add]  # 리스트를 append-only로 만듦
    context: Annotated[List[Document], operator.add]  # 문서 리스트도 append-only로 만듦
    current_question: str
    refined_question: str
    search_results: Annotated[List[SearchResult], operator.add]  # 검색 결과도 append-only로 만듦
    intermediate_thoughts: Annotated[List[str], operator.add]  # 중간 생각도 append-only로 만듦
    confidence_score: float
    iteration_count: int
    should_continue: bool


class EnhancedCodebaseRAGSystem:
    def __init__(self, vector_store_path: str, api_key: str,
                 max_iterations: int = 3,
                 min_confidence_threshold: float = 0.8):
        self.api_key = api_key
        self.max_iterations = max_iterations
        self.min_confidence_threshold = min_confidence_threshold

        genai.configure(api_key=api_key)

        # 벡터 스토어 로드
        self.vector_store = FAISS.load_local(
            vector_store_path,
            DummyEmbeddings(),
            allow_dangerous_deserialization=True
        )

        self.llm = ChatGoogleGenerativeAI(
            model="gemini-1.5-pro",
            temperature=0.2,
            max_tokens=None,
        )

        # 그래프 생성
        self.graph = self._create_enhanced_graph()

    def _should_continue(self, state: EnhancedGraphState) -> str:
        """다음 단계 결정 함수"""
        # 검색 결과의 품질 평가
        avg_relevance = sum(result.relevance_score for result in state["search_results"]) / len(
            state["search_results"]) if state["search_results"] else 0

        # 코드 관련 키워드 매칭 검사
        code_patterns = {
            'class_definition': r'class\s+\w+',
            'function_definition': r'def\s+\w+',
            'variable_declaration': r'\w+\s*=\s*\w+'
        }

        pattern_matches = 0
        for result in state["search_results"][:5]:
            content = result.document.page_content
            for pattern in code_patterns.values():
                if re.search(pattern, content):
                    pattern_matches += 1

        code_relevance = pattern_matches / (5 * len(code_patterns))

        # 반복 횟수 확인
        iterations_left = state["iteration_count"] < self.max_iterations

        # 종료 조건 평가
        quality_sufficient = (
                avg_relevance >= 0.7 and
                code_relevance >= 0.5
        )

        return "generate" if quality_sufficient or not iterations_left else "retrieve"

    def _refine_question(self, state: EnhancedGraphState) -> EnhancedGraphState:
        """질문을 코드 컨텍스트에 맞게 정제"""
        iteration = state["iteration_count"]
        current_question = state["refined_question"] if iteration > 1 else state["current_question"]

        prompt = f"""Iteration {iteration}: 아래 질문을 코드 검색에 최적화된 형태로 다시 작성해주세요.
        원본 질문: {current_question}
        지금까지의 중간 생각들:
        {chr(10).join(state["intermediate_thoughts"])}
        """

        response = self.llm.invoke([HumanMessage(content=prompt)])
        return {"refined_question": response.content}

    def _enhanced_retrieve(self, state: EnhancedGraphState) -> EnhancedGraphState:
        """개선된 문맥 검색 로직"""
        iteration = state["iteration_count"]
        k = max(100 - (iteration * 20), 40)

        results = self.vector_store.similarity_search_with_score(
            state["refined_question"],
            k=k
        )

        code_patterns = {
            'class_definition': r'class\s+\w+',
            'function_definition': r'def\s+\w+',
            'imports': r'import\s+\w+|from\s+\w+\s+import',
            'variable_declaration': r'\w+\s*=\s*\w+',
            'code_comments': r'#.*|"""[\s\S]*?"""'
        }

        pattern_weights = {pattern: 1.2 + (iteration * 0.1) for pattern in code_patterns}
        weighted_results = []

        for doc, score in results:
            relevance_score = score
            is_duplicate = any(
                prev_result.document.page_content == doc.page_content
                for prev_result in state.get("search_results", [])
            )

            if is_duplicate:
                relevance_score *= 0.8

            for pattern, weight in pattern_weights.items():
                if re.search(code_patterns[pattern], doc.page_content):
                    relevance_score *= weight

            if doc.metadata['extension'] in ['.py', '.cpp', '.h']:
                relevance_score *= (1.1 + (iteration * 0.05))

            weighted_results.append(
                SearchResult(
                    document=doc,
                    similarity=score,
                    relevance_score=relevance_score
                )
            )

        filtered_results = sorted(
            weighted_results,
            key=lambda x: x.relevance_score,
            reverse=True
        )[:10]

        return {
            "search_results": filtered_results,
            "context": [r.document for r in filtered_results]
        }

    def _analyze_context(self, state: EnhancedGraphState) -> EnhancedGraphState:
        """검색된 문맥 분석"""
        iteration = state["iteration_count"]
        context_str = "\n".join([doc.page_content for doc in state["context"]])

        prompt = f"""Iteration {iteration}: 다음 코드 컨텍스트를 분석하고 주요 포인트를 추출해주세요.
        코드 컨텍스트:
        {context_str}
        이전 분석 결과들:
        {chr(10).join(state["intermediate_thoughts"])}
        질문:
        {state["refined_question"]}
        """

        response = self.llm.invoke([HumanMessage(content=prompt)])
        return {"intermediate_thoughts": [f"Iteration {iteration}: {response.content}"]}

    def _generate_enhanced_response(self, state: EnhancedGraphState) -> EnhancedGraphState:
        """개선된 응답 생성"""
        iteration = state["iteration_count"]
        relevant_contexts = []

        for result in state["search_results"][:5]:
            doc = result.document
            relevant_contexts.append(f"""
            파일: {doc.metadata['file_path']}
            관련도: {result.relevance_score:.2f}
            내용:
            {doc.page_content}
            """)

        context_str = "\n---\n".join(relevant_contexts)
        analysis = "\n".join(state["intermediate_thoughts"])

        prompt = f"""Iteration {iteration}: 다음 정보를 바탕으로 질문에 대한 포괄적이고 정확한 답변을 제공해주세요.
        원본 질문: {state["current_question"]}
        정제된 질문: {state["refined_question"]}
        분석된 컨텍스트:
        {analysis}
        관련 코드:
        {context_str}
        """

        response = self.llm.invoke([HumanMessage(content=prompt)])
        return {"messages": [response]}

    def _evaluate_response(self, state: EnhancedGraphState) -> Tuple[EnhancedGraphState, bool]:
        """응답의 품질을 평가하고 계속 진행할지 결정"""
        current_response = state["messages"][-1].content

        evaluation_prompt = f"""다음 응답의 품질을 평가해주세요:
        질문: {state["current_question"]}
        응답: {current_response}
        """

        response = self.llm.invoke([HumanMessage(content=evaluation_prompt)])

        try:
            score_match = re.search(r'(?i)(?:score|점수):\s*(\d*\.?\d+)', response.content)
            confidence_score = float(score_match.group(1)) if score_match else 0.0
        except:
            confidence_score = 0.0

        should_continue = (
                confidence_score < self.min_confidence_threshold and
                state["iteration_count"] < self.max_iterations
        )

        return {
            "confidence_score": confidence_score,
            "should_continue": should_continue,
            "iteration_count": state["iteration_count"] + 1
        }, should_continue

    def _create_enhanced_graph(self) -> Graph:
        """향상된 LangGraph 워크플로우 생성"""
        workflow = StateGraph(EnhancedGraphState)

        workflow.add_node("refine", self._refine_question)
        workflow.add_node("retrieve", self._enhanced_retrieve)
        workflow.add_node("analyze", self._analyze_context)
        workflow.add_node("generate", self._generate_enhanced_response)
        workflow.add_node("evaluate", self._evaluate_response)

        workflow.add_edge("refine", "retrieve")
        workflow.add_edge("retrieve", "analyze")
        workflow.add_edge("analyze", "generate")
        workflow.add_edge("generate", "evaluate")

        workflow.add_conditional_edges(
            "retrieve",
            self._should_continue,
            {
                "retrieve": "retrieve",
                "generate": "generate"
            }
        )

        workflow.set_entry_point("refine")

        return workflow.compile()

    def query(self, question: str) -> dict:
        """개선된 질문 처리"""
        initial_state = EnhancedGraphState(
            messages=[],
            context=[],
            current_question=question,
            refined_question="",
            search_results=[],
            intermediate_thoughts=[],
            confidence_score=0.0,
            iteration_count=1,
            should_continue=True
        )

        final_state = self.graph.invoke(initial_state)

        iterations = []
        for i in range(final_state["iteration_count"]):
            iterations.append({
                "iteration": i + 1,
                "refined_question": final_state["refined_question"],
                "confidence_score": final_state["confidence_score"],
                "intermediate_thoughts": [
                    thought for thought in final_state["intermediate_thoughts"]
                    if f"Iteration {i + 1}:" in thought
                ]
            })

        return {
            "final_answer": final_state["messages"][-1].content,
            "iterations": iterations,
            "total_iterations": final_state["iteration_count"],
            "final_confidence_score": final_state["confidence_score"],
            "source_documents": [
                {
                    "content": doc.page_content,
                    "metadata": doc.metadata,
                    "relevance_score": score.relevance_score
                }
                for doc, score in zip(final_state["context"], final_state["search_results"])
            ]
        }


if __name__ == "__main__":
    VECTOR_STORE_PATH = "vector_store"

    rag_system = EnhancedCodebaseRAGSystem(
        VECTOR_STORE_PATH,
        GOOGLE_API_KEY,
        max_iterations=3,
        min_confidence_threshold=0.8
    )

    question = "MassAI 모듈의 주요 기능을 알려주세요"
    response = rag_system.query(question)

    print(f"\n원본 질문: {question}")
    print(f"\n총 반복 횟수: {response['total_iterations']}")
    print(f"최종 신뢰도 점수: {response['final_confidence_score']:.2f}")