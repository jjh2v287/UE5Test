// RRVO_Agent.cpp
#include "RRVOAgent.h"
#include "DrawDebugHelpers.h"

// 기본 생성자
ARRVOAgent::ARRVOAgent()
{
    PrimaryActorTick.bCanEverTick = true;

    // 기본값 설정
    Geometry = FRRVORect(FVector::ZeroVector, 100.0f, 100.0f);
    Velocity = FVector::ZeroVector;
    Orientation = 0.0f;
    MaxAngularVelocity = 45.0f; // 초당 45도
    PreferredSpeed = 200.0f; // 초당 200 유닛
    TargetPosition = FVector(500.0f, 500.0f, 0.0f);
}

void ARRVOAgent::BeginPlay()
{
    Super::BeginPlay();
    
    // 시작시 위치 기반으로 지오메트리 업데이트
    FVector Location = GetActorLocation();
    Geometry = FRRVORect(Location, 100.0f, 100.0f, Orientation);
}

void ARRVOAgent::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // 매 프레임마다 에이전트 업데이트
    UpdateAgent(DeltaTime);
    
    // 디버그 시각화
    DrawDebugSolidBox(GetWorld(), GetActorLocation(), FVector(50.0f, 50.0f, 10.0f), FQuat::MakeFromEuler(FVector(0.0f, 0.0f, Orientation)), FColor::Green);
    
    // 속도 벡터 시각화
    DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + Velocity, FColor::Red, false, -1.0f, 0, 3.0f);
}

// 알고리즘 1: 새로운 속도 계산
FVector ARRVOAgent::ComputeNewVelocity()
{
    // 1. 선형 프로그램 초기화
    FLinearProgram LP;
    
    // 선호 속도를 목표 방향으로 설정 (목적 함수)
    FVector PreferredVelocity = (TargetPosition - GetActorLocation()).GetSafeNormal() * PreferredSpeed;
    LP.ObjectiveCoefficients = FVector2D(PreferredVelocity.X - Velocity.X, PreferredVelocity.Y - Velocity.Y);

    // 2. 각 이웃에 대한 제약 조건 추가
    for (ARRVOAgent* Neighbor : Neighbors)
    {
        if (Neighbor == nullptr) continue;

        // 이웃이 유도하는 속도 장애물 계산
        FVelocityObstacle VO = ComputeVelocityObstacle(Neighbor, 5.0f); // 5초 시간 지평선

        // VO의 각 경계에 대한 선형 제약 조건 추가
        for (int32 i = 0; i < VO.Boundary.Num(); i++)
        {
            int32 NextIdx = (i + 1) % VO.Boundary.Num();
            FVector Edge = VO.Boundary[NextIdx] - VO.Boundary[i];
            
            // 경계의 법선 벡터 계산 (2D에서 90도 회전)
            FVector2D Normal(-Edge.Y, Edge.X);
            Normal.Normalize();
            
            // Ax <= b 형태의 제약 조건 생성
            float b = FVector2D::DotProduct(Normal, FVector2D(VO.Boundary[i].X, VO.Boundary[i].Y));
            LP.AddConstraint(Normal, b);
        }

        // 왼쪽 다리 제약 조건
        if (!VO.LeftLeg.IsZero())
        {
            FVector2D Normal(-VO.LeftLeg.Y, VO.LeftLeg.X);
            Normal.Normalize();
            LP.AddConstraint(Normal, 0.0f); // 원점을 통과하는 선이므로 b = 0
        }

        // 오른쪽 다리 제약 조건
        if (!VO.RightLeg.IsZero())
        {
            FVector2D Normal(-VO.RightLeg.Y, VO.RightLeg.X);
            Normal.Normalize();
            LP.AddConstraint(Normal, 0.0f); // 원점을 통과하는 선이므로 b = 0
        }
    }

    // 3. 속도 제한 제약 조건 추가 (선택 사항)
    float MaxSpeed = PreferredSpeed * 1.5f;
    LP.AddConstraint(FVector2D(1.0f, 0.0f), MaxSpeed); // vx <= MaxSpeed
    LP.AddConstraint(FVector2D(-1.0f, 0.0f), MaxSpeed); // -vx <= MaxSpeed => vx >= -MaxSpeed
    LP.AddConstraint(FVector2D(0.0f, 1.0f), MaxSpeed); // vy <= MaxSpeed
    LP.AddConstraint(FVector2D(0.0f, -1.0f), MaxSpeed); // -vy <= MaxSpeed => vy >= -MaxSpeed

    // 4. 선형 프로그램 해결
    FVector NewVelocity = SolveLinearProgram(LP);
    
    return NewVelocity;
}

// 알고리즘 2: 속도 장애물 계산
FVelocityObstacle ARRVOAgent::ComputeVelocityObstacle(ARRVOAgent* OtherAgent, float TimeHorizon)
{
    FVelocityObstacle VO;
    
    if (OtherAgent == nullptr)
    {
        return VO;
    }

    // 1. Minkowski Sum 계산
    FRRVORect M = ComputeMinkowskiSum(Geometry, OtherAgent->Geometry);
    
    // 2. 상대 위치 벡터로 변환 (Translate)
    FVector Pa = GetActorLocation();
    FVector Pb = OtherAgent->GetActorLocation();
    FVector RelativePos = (Pb * (1.0f - TimeHorizon) - Pa * (1.0f + TimeHorizon)) / TimeHorizon;
    
    // 변환된 Minkowski Sum 계산 (모든 포인트를 RelativePos만큼 이동)
    TArray<FVector> TranslatedVertices;
    for (const FVector& Vertex : M.GetVertices())
    {
        TranslatedVertices.Add(Vertex + RelativePos);
    }
    
    // Translated 사각형 생성
    FRRVORect TranslatedM;
    TranslatedM.TopLeft = TranslatedVertices[0];
    TranslatedM.TopRight = TranslatedVertices[1];
    TranslatedM.BottomRight = TranslatedVertices[2];
    TranslatedM.BottomLeft = TranslatedVertices[3];
    
    // 3. Scale (1/τ로 스케일링)
    FVector Center = TranslatedM.GetCenter();
    TArray<FVector> ScaledVertices;
    for (const FVector& Vertex : TranslatedM.GetVertices())
    {
        FVector Direction = Vertex - Center;
        ScaledVertices.Add(Center + Direction / TimeHorizon);
    }
    
    FRRVORect ScaledM;
    ScaledM.TopLeft = ScaledVertices[0];
    ScaledM.TopRight = ScaledVertices[1];
    ScaledM.BottomRight = ScaledVertices[2];
    ScaledM.BottomLeft = ScaledVertices[3];
    
    // 4. 원점에서의 접선 계산
    TPair<FVector, FVector> Tangents = ComputeTangents(ScaledM, FVector::ZeroVector);
    FVector LeftTangent = Tangents.Key;
    FVector RightTangent = Tangents.Value;
    
    // 5. VO 계산 시작
    // 먼저 경계 생성 (ScaledM의 모든 꼭지점 검사)
    for (const FVector& Vertex : ScaledM.GetVertices())
    {
        // 원점에서 꼭지점까지의 벡터 생성
        FVector OriginToVertex = Vertex;
        
        // Cross product를 통해 꼭지점이 접선 사이에 있는지 확인
        float CrossLeftRight = FVector2D::CrossProduct(
            FVector2D(RightTangent.X, RightTangent.Y),
            FVector2D(LeftTangent.X, LeftTangent.Y)
        );
        
        float CrossRightVertex = FVector2D::CrossProduct(
            FVector2D(OriginToVertex.X, OriginToVertex.Y),
            FVector2D(RightTangent.X, RightTangent.Y)
        );
        
        float CrossVertexLeft = FVector2D::CrossProduct(
            FVector2D(LeftTangent.X, LeftTangent.Y),
            FVector2D(OriginToVertex.X, OriginToVertex.Y)
        );
        
        // 원점에서 볼 때 두 접선 사이에 꼭지점이 있으면 VO에 포함
        if ((CrossLeftRight <= 0 && CrossRightVertex <= 0 && CrossVertexLeft <= 0) || 
            (CrossLeftRight >= 0 && CrossRightVertex >= 0 && CrossVertexLeft >= 0))
        {
            VO.Boundary.Add(Vertex);
        }
    }
    
    // 6. VO의 경계를 시계 방향으로 정렬
    if (VO.Boundary.Num() > 2)
    {
        FVector Center = FVector::ZeroVector;
        for (const FVector& Vertex : VO.Boundary)
        {
            Center += Vertex;
        }
        Center /= VO.Boundary.Num();
        
        VO.Boundary.Sort([Center](const FVector& A, const FVector& B) {
            return FMath::Atan2(A.Y - Center.Y, A.X - Center.X) < FMath::Atan2(B.Y - Center.Y, B.X - Center.X);
        });
    }
    
    // 7. 무한한 구간을 표현하기 위한 접선 다리 설정
    VO.LeftLeg = LeftTangent * 2.0f;
    VO.RightLeg = RightTangent * 2.0f;
    
    return VO;
}

// 알고리즘 3: 새로운 속도와 방향 계산
TPair<FVector, float> ARRVOAgent::ComputeNewVelocityAndOrientation()
{
    // 초기화
    TArray<FLinearProgram> LPs;
    FVector BestVelocity = Velocity;
    float BestOrientation = Orientation;
    float BestCost = FLT_MAX;
    
    // 1. 도달 가능한 방향 목록 생성 (현재 방향에서 MaxAngularVelocity 범위 내)
    TArray<float> ReachableOrientations;
    float OrientationStep = 15.0f; // 15도 간격으로 샘플링
    
    for (float Theta = Orientation - MaxAngularVelocity; Theta <= Orientation + MaxAngularVelocity; Theta += OrientationStep)
    {
        ReachableOrientations.Add(FMath::Fmod(Theta + 360.0f, 360.0f)); // 0-360도 범위로 정규화
    }
    
    // 2. 각 도달 가능한 방향에 대해 선형 프로그램 생성 및 해결
    for (float NewOrientation : ReachableOrientations)
    {
        // 새 방향에 맞는 지오메트리 변환
        FRRVORect RotatedGeometry = FRRVORect(GetActorLocation(), 100.0f, 100.0f, NewOrientation);
        
        // 선형 프로그램 초기화
        FLinearProgram LP;
        
        // 선호 속도를 목표 방향으로 설정 (목적 함수)
        FVector PreferredVelocity = (TargetPosition - GetActorLocation()).GetSafeNormal() * PreferredSpeed;
        LP.ObjectiveCoefficients = FVector2D(PreferredVelocity.X - Velocity.X, PreferredVelocity.Y - Velocity.Y);
        
        // 선형 제약 조건 추가
        for (ARRVOAgent* Neighbor : Neighbors)
        {
            if (Neighbor == nullptr) continue;
            
            // 이웃의 도달 가능한 방향 목록 생성
            TArray<float> NeighborOrientations;
            float NeighborOrientation = Neighbor->Orientation;
            float NeighborMaxAngVel = Neighbor->MaxAngularVelocity;
            
            for (float Theta = NeighborOrientation - NeighborMaxAngVel; Theta <= NeighborOrientation + NeighborMaxAngVel; Theta += OrientationStep)
            {
                NeighborOrientations.Add(FMath::Fmod(Theta + 360.0f, 360.0f));
            }
            
            // 각 이웃의 방향에 대한 제약 조건 추가
            for (float NeighborNewOrientation : NeighborOrientations)
            {
                // 이웃의 새 방향에 맞는 지오메트리 변환
                FRRVORect NeighborRotatedGeometry = FRRVORect(Neighbor->GetActorLocation(), 100.0f, 100.0f, NeighborNewOrientation);
                
                // Minkowski Sum 계산
                FRRVORect M = ComputeMinkowskiSum(RotatedGeometry, NeighborRotatedGeometry);
                
                // 속도 장애물 생성 및 제약 조건 추가
                // (여기서는 단순화를 위해 ComputeVelocityObstacle 함수를 다시 사용)
                FVelocityObstacle VO = ComputeVelocityObstacle(Neighbor, 5.0f);
                
                // VO 경계에 대한 선형 제약 조건 추가
                for (int32 i = 0; i < VO.Boundary.Num(); i++)
                {
                    int32 NextIdx = (i + 1) % VO.Boundary.Num();
                    FVector Edge = VO.Boundary[NextIdx] - VO.Boundary[i];
                    
                    // 경계의 법선 벡터 계산
                    FVector2D Normal(-Edge.Y, Edge.X);
                    Normal.Normalize();
                    
                    // Ax <= b 형태의 제약 조건 생성
                    float b = FVector2D::DotProduct(Normal, FVector2D(VO.Boundary[i].X, VO.Boundary[i].Y));
                    LP.AddConstraint(Normal, b);
                }
                
                // 탄젠트 다리 제약 조건 추가
                if (!VO.LeftLeg.IsZero())
                {
                    FVector2D Normal(-VO.LeftLeg.Y, VO.LeftLeg.X);
                    Normal.Normalize();
                    LP.AddConstraint(Normal, 0.0f);
                }
                
                if (!VO.RightLeg.IsZero())
                {
                    FVector2D Normal(-VO.RightLeg.Y, VO.RightLeg.X);
                    Normal.Normalize();
                    LP.AddConstraint(Normal, 0.0f);
                }
            }
        }
        
        // 속도 제한 제약 조건 추가
        float MaxSpeed = PreferredSpeed * 1.5f;
        LP.AddConstraint(FVector2D(1.0f, 0.0f), MaxSpeed);
        LP.AddConstraint(FVector2D(-1.0f, 0.0f), MaxSpeed);
        LP.AddConstraint(FVector2D(0.0f, 1.0f), MaxSpeed);
        LP.AddConstraint(FVector2D(0.0f, -1.0f), MaxSpeed);
        
        // 선형 프로그램 해결
        FVector NewVelocity = SolveLinearProgram(LP);
        
        // 비용 계산 (선호 속도와의 차이)
        float Cost = (NewVelocity - PreferredVelocity).SizeSquared();
        
        // 더 나은 결과를 찾았다면 업데이트
        if (Cost < BestCost)
        {
            BestCost = Cost;
            BestVelocity = NewVelocity;
            BestOrientation = NewOrientation;
        }
    }
    
    return TPair<FVector, float>(BestVelocity, BestOrientation);
}

// Minkowski Sum 계산
FRRVORect ARRVOAgent::ComputeMinkowskiSum(const FRRVORect& RectA, const FRRVORect& RectB)
{
    // Minkowski Sum은 모든 가능한 A + B 조합을 계산하는 것
    TArray<FVector> SumVertices;
    
    // 사각형 A의 모든 꼭지점
    TArray<FVector> VerticesA = RectA.GetVertices();
    
    // 사각형 B의 모든 꼭지점
    TArray<FVector> VerticesB = RectB.GetVertices();

    return FRRVORect();
}