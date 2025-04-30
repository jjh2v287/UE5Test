from crewai import Agent, Task, Crew, Process, LLM
from dotenv import load_dotenv
import os

# .env 파일에서 환경 변수 로드
# load_dotenv()는 보통 스크립트 최상단에서 한번만 호출합니다.
load_dotenv()

gemini_llm = LLM(model="gemini/gemini-2.5-flash-preview-04-17", api_key=os.environ["GOOGLE_API_KEY"])

researcher = Agent(
  role='선임 연구 분석가',
  goal='AI와 데이터 과학 분야의 최첨단 개발 동향 파악',
  backstory="""당신은 선도적인 기술 싱크탱크에서 근무합니다.
  당신의 전문 분야는 새로운 트렌드를 식별하는 것입니다.
  복잡한 데이터를 분석하고 실행 가능한 통찰력을 제시하는 데 능숙합니다.""",
  verbose=True,
  allow_delegation=False,
  llm=gemini_llm  # 생성한 Gemini LLM 객체 전달
  # tools=[...] # 필요한 도구 추가
)

# 예시 Task 설정
task1 = Task(
  description="""AI 및 데이터 과학 분야의 최신 발전 사항을 조사합니다.
  주요 트렌드, 혁신적인 기술, 잠재적 영향에 중점을 둡니다.""",
  expected_output="주요 조사 결과와 통찰력을 요약한 전체 보고서",
  agent=researcher
)

# 예시 Crew 설정 (Crew 레벨에서 LLM을 설정할 수도 있습니다)
crew = Crew(
  agents=[researcher],
  tasks=[task1],
  verbose=True,
  process=Process.sequential # 또는 다른 프로세스
)

# Crew 실행
result = crew.kickoff()
print(result)
