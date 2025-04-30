MCP(Model Context Protocol)와 A2A(Agent2Agent)는 멀티 에이전트 AI 시스템을 더 효과적으로 구축할 수 있게 해주는 두 가지 오픈 프로토콜입니다. 이 두 프로토콜은 서로 다른 문제를 해결하지만, 함께 사용하면 강력한 시너지 효과를 발휘합니다.

## 결론부터 말하자면

**MCP와 A2A는 멀티 에이전트 시스템 개발의 핵심 장벽을 해결합니다.** MCP는 에이전트가 외부 세계와 상호작용하는 방법을 표준화하고, A2A는 서로 다른 환경의 에이전트들이 원활하게 통신할 수 있게 해줍니다. 이 조합은 작고 전문적인 에이전트들이 함께 협력하여 복잡한 작업을 수행할 수 있는 환경을 제공합니다.

## MCP(Model Context Protocol)의 핵심

MCP는 Anthropic에서 개발한 프로토콜로, **언어 모델에 대한 "USB-C 포트"** 역할을 합니다.

### 주요 특징과 작동 원리

|특징|설명|이점|
|---|---|---|
|표준화된 인터페이스|동일한 모델을 다양한 데이터 소스에 쉽게 연결|통합 작업의 반복 없이 다양한 도구 사용 가능|
|두 가지 전송 방식|STDIO와 SSE 방식 지원|로컬 및 네트워크 환경 모두 지원|
|도구 기반 구조|@mcp.tool() 데코레이터로 기능 정의|FastAPI와 유사한 직관적 인터페이스|

---

MCP 서버를 구현하면 에이전트가 외부 세계와 소통하는 방식이 표준화됩니다. 예를 들어, 주식 정보를 검색하는 MCP 서버는 다음과 같이 구현할 수 있습니다:

python

```python
@mcp.tool()
def get_price_of_stock(symbol: str) -> Dict[str,Any]:
    """주식 심볼로 현재 가격 정보 가져오기"""
    resp = service.client.quote(symbol)
    return {
        'current_price': resp['c'],
        'change': resp['d'],
        'percentage_change': resp['dp'],
        # 기타 정보들
    }
```

이렇게 구현된 도구는 어떤 언어 모델과도 쉽게 통합될 수 있기 때문에 개발 효율성이 크게 향상됩니다.

## A2A(Agent2Agent) 프로토콜의 핵심

A2A는 Google에서 개발한 프로토콜로, **서로 다른 프레임워크와 환경의 에이전트들이 효과적으로 통신**할 수 있게 해줍니다.

### 주요 구성 요소와 작동 방식

|구성 요소|역할|작동 방식|
|---|---|---|
|에이전트 카드|에이전트의 기능과 접근 방법 정의|/.well-known/agent.json에 저장|
|A2A 서버|요청 처리 및 작업 실행 관리|HTTP 엔드포인트 노출|
|작업(Task)|에이전트 간 협업의 기본 단위|제출됨→진행 중→완료됨 등의 상태 변화|
|메시지와 아티팩트|통신 및 결과물 교환 단위|텍스트, 파일, 구조화된 데이터 포함 가능|

---

A2A의 핵심은 에이전트 카드를 통한 자기 소개와 표준화된 HTTP 인터페이스입니다. 에이전트 카드는 다음과 같이 정의될 수 있습니다:

python

```python
AGENT_CARD = generate_agent_card(
    agent_name="stock_report_agent",
    agent_description="주식 가격 및 정보 제공 에이전트",
    agent_url="http://localhost:10000/stock_agent",
    skills=[AgentSkill(
        id="SKILL_STOCK_REPORT",
        name="stock_report",
        description="주식 가격 및 정보 제공",
    )],
)
```

이 카드를 통해 다른 에이전트들은 이 에이전트의 기능을 파악하고 적절하게 소통할 수 있습니다.

## 두 프로토콜의 시너지 효과

MCP와 A2A를 함께 사용하면 **복잡한 다중 에이전트 시스템을 효율적으로 구축**할 수 있습니다. 이 시스템에서는:

1. **MCP**가 각 에이전트의 외부 세계와의 상호작용을 처리합니다 (예: 주식 데이터 검색, 웹 검색)
2. **A2A**가 에이전트 간의 통신과 작업 조정을 담당합니다

이러한 구조는 다음과 같은 이점을 제공합니다:

- **모듈성**: 각 에이전트는 특정 작업에 집중할 수 있습니다.
- **확장성**: 새로운 기능이 필요할 때 새 에이전트를 추가하기만 하면 됩니다.
- **유연성**: 서로 다른 프레임워크로 구축된 에이전트들이 협업할 수 있습니다.
- **재사용성**: 표준화된 인터페이스로 인해 구성 요소를 쉽게 재사용할 수 있습니다.

예를 들어, 주식 분석 시스템을 구축할 때:

1. 검색 에이전트는 MCP를 통해 웹에서 정보를 검색합니다.
2. 주식 에이전트는 MCP를 통해 실시간 주식 데이터를 가져옵니다.
3. 호스트 에이전트는 A2A를 통해 이 두 에이전트와 통신하고 작업을 조정합니다.

## 실제 구현 예시: ADKAgent 클래스

`ADKAgent` 클래스는 Google의 ADK 프레임워크와 A2A 프로토콜을 연결하는 핵심 요소입니다. 이 클래스의 유연성 덕분에 다양한 형태의 에이전트 시스템을 구현할 수 있습니다.

python

```python
host_agent = ADKAgent(
    model="gemini-2.5-pro",
    name="host_agent",
    description="사용자 요청을 처리하고 다른 에이전트들에게 작업 위임",
    tools=[],
    is_host_agent=True,
    remote_agent_addresses=["http://localhost:11000/search", "http://localhost:10000/stocks"]
)
```

이 코드로 생성된 호스트 에이전트는 사용자 요청을 받아 검색과 주식 데이터에 특화된 다른 에이전트들에게 작업을 위임할 수 있습니다.

## 중요한 기술적 혁신: 비동기 서버 실행

A2A 서버 구현에서 가장 중요한 기술적 개선점은 비동기 방식의 서버 실행을 위한 `astart` 메서드입니다:

python

```python
async def astart(self):
    config = uvicorn.Config(self.app, host=self.host, port=self.port, loop="asyncio")
    server = uvicorn.Server(config)
    server_task = asyncio.create_task(server.serve())
    
    # 서버가 완전히 시작될 때까지 대기
    while not server.started:
        await asyncio.sleep(0.1)
```

이 구현은 기존의 동기식 방식에서 발생하던 여러 문제를 해결하여 MCP 도구와 같은 다른 비동기 컴포넌트와의 통합을 가능하게 합니다.

## 향후 발전 방향

MCP와 A2A는 AI 에이전트 개발의 새로운 시대를 열고 있습니다. 이 프로토콜들이 더욱 발전하면 다음과 같은 가능성이 열릴 것입니다:

1. **더 복잡한 협업 패턴**: 에이전트들이 더 정교한 방식으로 협업할 수 있게 됩니다.
2. **벤더 중립적 에코시스템**: 서로 다른 회사의 모델들이 원활하게 협업할 수 있습니다.
3. **자율성과 조정의 균형**: 개별 에이전트의 자율성을 유지하면서도 효과적인 조정이 가능합니다.

이러한 발전은 AI 에이전트 시스템을 **단순한 스크립트 모음에서 진정한 협업 워크포스로 전환**시키는 데 기여할 것입니다.

## 참고 자료

- [Model Context Protocol 공식 문서](https://modelcontextprotocol.io/quickstart/server)
- [Agent2Agent GitHub 리포지토리](https://github.com/google/A2A)
- [튜토리얼 소스 코드](https://github.com/Tsadoq/a2a-mcp-tutorial)

---
---
# MCP와 A2A 프로토콜: 멀티 에이전트 시스템 구축 가이드

## 핵심 요약

MCP(Model Context Protocol)와 A2A(Agent2Agent)는 멀티 에이전트 AI 프로젝트를 실험실 수준에서 실무 환경으로 전환하는 데 필수적인 두 가지 오픈 프로토콜입니다. **MCP는 에이전트가 외부 세계와 상호작용하는 방법을 표준화**하고, **A2A는 서로 다른 환경의 에이전트들이 효과적으로 통신하는 방법을 정의**하기 때문에 협업 에이전트 시스템을 구축할 때 이상적인 조합을 형성합니다.

## 1. MCP(Model Context Protocol) 이해하기

### MCP란 무엇인가?

MCP는 Anthropic에서 소개한 프로토콜로, 언어 모델이 대화 중에 필요한 데이터와 도구에 일관되게 접근할 수 있게 해주는 표준화된 접근 방식입니다.

![MCP 아키텍처](https://matteovillosio.com/post/agent2agent-mcp-tutorial/mcp_rein_huc5cfd586eec0341f0e82a0b22315644f_2022983_5dbf2268d1a31be0c31f7d07d7158771.webp)

### MCP의 주요 특징

- **표준화된 인터페이스**: 언어 모델에 대한 "USB-C 포트"로 비유되며, 다양한 데이터 소스와 도구에 동일한 모델을 연결할 수 있게 합니다.
- **컨텍스트의 안정화**: 개발자들이 임시로 묶어놓는 컨텍스트를 모델이 의존할 수 있는 것으로 변환합니다.
- **두 가지 전송 방식**:
    - **STDIO MCP 서버**: 로컬 프로세스로 실행되며 stdin/stdout 스트림을 통해 JSON-RPC 메시지를 전달
    - **SSE MCP 서버**: HTTP 엔드포인트를 통해 클라이언트가 POST로 요청하고 서버는 Server-Sent Events(SSE) 스트림으로 결과를 반환

### MCP 서버 구현 예시

본 튜토리얼에서는 두 가지 MCP 서버를 구현했습니다:

1. **주식 검색기(Stock Retriever) 서버**:
    
    ```python
    class FinHubService:
        def get_symbol_from_query(self, query: str) -> Dict[str,Any]:
            # 회사 이름으로 주식 심볼 찾기
            return self.client.symbol_lookup(query=query)
    
        def get_price_of_stock(self, symbol: str) -> Dict[str,Any]:
            # 주식 심볼로 현재 가격 정보 가져오기
            resp = self.client.quote(symbol)
            return {
                'current_price': resp['c'],
                'change': resp['d'],
                'percentage_change': resp['dp'],
                # ...기타 정보들
            }
    ```
    
2. **웹 크롤러(Crawler) 서버**:
    
    ```python
    class SerperDevService:
        def search_google(self, query: str, n_results: int = 10) -> List[Dict[str, Any]]:
            # 구글 검색 실행
            # ...
            return response.json()['organic']
    
        def get_text_from_page(self, url_to_scrape: str) -> str:
            # 웹페이지 내용 스크래핑
            # ...
            return response.text
    ```
    

이를 MCP 서버로 변환하기 위해 FastMCP 클래스를 사용했습니다:

```python
mcp = FastMCP("Search Engine Server")

@mcp.tool()
def search_google(query: str, n_results: int = 10, page: int = 1) -> list:
    """Google 검색 실행"""
    return search_service.search_google(query, n_results, page)

@mcp.tool()
def get_text_from_page(url_to_scrape: str) -> str:
    """웹페이지 스크래핑"""
    return search_service.get_text_from_page(url_to_scrape)
```

## 2. A2A(Agent2Agent) 프로토콜 이해하기

### A2A란 무엇인가?

A2A는 Google에서 개발한 프로토콜로, 서로 다른 프레임워크와 호스팅 환경에 구축된 자율 에이전트들이 표준화된 방식으로 통신할 수 있게 해주는 시스템입니다.

### A2A의 주요 구성 요소

1. **에이전트 카드(Agent Card)**:
    
    - 에이전트의 디지털 ID 카드 역할을 하는 표준화된 메타데이터 문서
    - 보통 `/.well-known/agent.json`에 위치
    - 에이전트의 기능, 사용 가능한 스킬, 서비스 엔드포인트, 보안 요구사항을 상세히 기술
2. **A2A 서버**:
    
    - A2A 프로토콜을 구현하여 HTTP 엔드포인트를 노출하는 에이전트
    - 수신 요청 처리, 작업 실행 관리, 클라이언트와 통신 유지
3. **A2A 클라이언트**:
    
    - A2A 서비스를 소비하는 애플리케이션이나 에이전트
    - `tasks/send`나 `tasks/sendSubscribe` 등의 요청을 통해 상호작용 시작
4. **작업(Task)**:
    
    - A2A에서의 기본 작업 단위
    - 고유한 대화 스레드를 생성하고, 다양한 상태(제출됨, 진행 중, 입력 필요, 완료됨 등)를 거침
5. **메시지(Message)와 부분(Part)**:
    
    - 통신의 기본 단위로, 텍스트, 파일, 구조화된 데이터 등을 포함 가능
6. **아티팩트(Artifact)**:
    
    - 에이전트가 작업 실행 중 생성한 출력물
7. **스트리밍(Streaming)**과 **푸시 알림(Push Notifications)**:
    
    - 실시간 통신과 상태 업데이트를 위한 메커니즘

### 일반적인 A2A 상호작용 흐름

1. **발견(Discovery)**: 클라이언트가 서버의 에이전트 카드를 검색
2. **시작(Initiation)**: 클라이언트가 새 작업을 시작
3. **처리(Processing)**: 서버가 작업을 처리하고 실시간 업데이트 제공
4. **상호작용(Interaction)**: 필요시 클라이언트가 추가 입력 제공
5. **완료(Completion)**: 작업이 최종 상태(완료, 실패, 취소)에 도달

## 3. 다중 에이전트 시스템 구현하기

A2A와 MCP를 결합하여 두 가지 방식으로 다중 에이전트 시스템을 구현할 수 있습니다:

### 계층적 크루(Hierarchical Crew)

Google의 ADK(Agent Development Kit)를 사용하여 계층적인 에이전트 구조를 구현합니다:

```python
# 루트 에이전트(코디네이터) 정의
root_agent = Agent(
    name="company_analysis_assistant",
    model=MODEL,
    description="주요 회사 및 주식 정보 분석 어시스턴트",
    instruction=(
        "당신은 팀을 조정하는 메인 어시스턴트입니다. 주요 책임은 회사와 주식 보고서를 제공하고 다른 작업을 위임하는 것입니다.\n"
        "1. 사용자가 회사에 대해 물으면 자세한 보고서를 제공합니다.\n"
        "2. 현재 주식 가격에 대한 정보가 필요하면 stock_analysis_agent에게 위임합니다.\n"
        "3. 정보 검색이 필요하면 search_agent에게 위임합니다.\n"
    ),
    sub_agents=[search_agent, stock_analysis_agent],
)
```

### A2A 크루(Peer-to-Peer)

Google의 A2A 프레임워크를 사용하여 P2P 방식의 에이전트 네트워크를 구현합니다:

```python
# A2A 호스트 에이전트 정의
host_agent = ADKAgent(
    model=MODEL,
    name="host_agent",
    description="사용자 요청을 처리하고 다른 에이전트들에게 작업을 위임하는 호스트",
    tools=[],
    instructions="",
    is_host_agent=True,
    remote_agent_addresses=["http://localhost:11000/google_search_agent", 
                           "http://localhost:10000/stock_agent"],
)
```

각 에이전트는 자체 A2A 서버를 실행하며, 에이전트 카드를 통해 다른 에이전트에게 자신의 기능을 알립니다:

```python
# 에이전트 카드 생성
AGENT_CARD = generate_agent_card(
    agent_name=AGENT_NAME,
    agent_description=AGENT_DESCRIPTION,
    agent_url=AGENT_URL,
    agent_version=AGENT_VERSION,
    can_stream=False,
    can_push_notifications=False,
    skills=AGENT_SKILLS,
)

# A2A 서버 시작
server = A2AServer(
    host=HOST,
    port=PORT,
    endpoint="/google_search_agent",
    agent_card=AGENT_CARD,
    task_manager=task_manager
)
await server.astart()
```

## 4. MCP와 A2A의 통합: 핵심 기능과 이점

### ADKAgent 클래스

Google ADK 프레임워크와 A2A 프로토콜을 연결하는 핵심 클래스:

```python
class ADKAgent:
    def __init__(
            self,
            model: str,
            name: str,
            description: str,
            instructions: str,
            tools: List[Any],
            is_host_agent: bool = False,
            remote_agent_addresses: List[str] = None,
    ):
        # 초기화 코드
```

주요 기능:

- **두 가지 작동 모드**: 독립형 에이전트 또는 코디네이터 역할의 호스트 에이전트
- **원격 에이전트 관리**: 다른 에이전트를 발견하고 연결
- **작업 위임**: 작업 생성, 전송, 생명주기 관리
- **상태 관리**: 세션, 작업, 연결된 에이전트에 대한 정보 유지
- **응답 처리**: 다양한 형식의 응답 변환 처리

### A2A 서버 구현의 주요 개선점

비동기 방식의 서버 실행을 위한 `astart` 메서드 구현:

```python
async def astart(self):
    config = uvicorn.Config(self.app, host=self.host, port=self.port, loop="asyncio")
    server = uvicorn.Server(config)
    server_task = asyncio.create_task(server.serve())
    
    while not server.started:
        await asyncio.sleep(0.1)
    
    try:
        await server_task
    except KeyboardInterrupt:
        server.should_exit = True
        await server_task
```

이러한 구현의 이점:

- MCP 도구와 같은 다른 비동기 컴포넌트와 통합 가능
- 적절한 서버 수명주기 관리
- 여러 에이전트 서버의 동시 실행 지원

## 5. 결론: MCP와 A2A의 시너지 효과

이 두 프로토콜의 조합은 AI 에이전트 개발에 혁신적인 접근 방식을 제공합니다:

- **MCP**는 에이전트가 외부 세계와 상호작용하는 방법을 표준화하고, 데이터 소스와 도구에 일관된 접근을 제공합니다.
- **A2A**는 에이전트 간의 통신을 표준화하여 서로 다른 프레임워크와 호스팅 환경에서 구축된 에이전트들이 원활하게 협업할 수 있게 합니다.

이러한 조합은 에이전트 개발을 **단순한 스크립트 모음에서 협업 가능한 워크포스로 전환**시킵니다. 작고 집중된 에이전트들이 자신의 전문 영역에 집중하면서도 다른 에이전트와 협업하여 복잡한 문제를 해결할 수 있게 됩니다.

이 접근 방식은 확장 가능하고 유지 관리가 쉬운 멀티 에이전트 시스템을 구축하는 데 강력한 기반을 제공하며, 앞으로의 AI 에이전트 개발 방향을 제시합니다.

## 참고 자료

- [Model Context Protocol(MCP) 공식 문서](https://modelcontextprotocol.io/quickstart/server)
- [Agent2Agent(A2A) GitHub 리포지토리](https://github.com/google/A2A)
- [튜토리얼 소스 코드](https://github.com/Tsadoq/a2a-mcp-tutorial)