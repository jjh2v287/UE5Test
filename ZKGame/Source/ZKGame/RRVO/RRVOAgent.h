// RRVO_Agent.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Containers/Array.h"
#include "Math/Vector.h"
#include "Math/UnrealMathUtility.h"
#include "RRVOAgent.generated.h"

/**
 * RRVO 알고리즘에서 사용할 사각형 구조체
 */
USTRUCT(BlueprintType)
struct FRRVORect
{
    GENERATED_BODY()

    // 사각형의 각 꼭지점
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RRVO")
    FVector TopLeft;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RRVO")
    FVector TopRight;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RRVO")
    FVector BottomLeft;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RRVO")
    FVector BottomRight;

    // 기본 생성자
    FRRVORect()
    {
        TopLeft = FVector::ZeroVector;
        TopRight = FVector::ZeroVector;
        BottomLeft = FVector::ZeroVector;
        BottomRight = FVector::ZeroVector;
    }

    // 사각형 생성 (중심점, 너비, 높이, 회전)
    FRRVORect(const FVector& Center, float Width, float Height, float Rotation = 0.0f)
    {
        float HalfWidth = Width * 0.5f;
        float HalfHeight = Height * 0.5f;

        // 회전 적용 전 기본 꼭지점 위치
        FVector LocalTopLeft(-HalfWidth, HalfHeight, 0.0f);
        FVector LocalTopRight(HalfWidth, HalfHeight, 0.0f);
        FVector LocalBottomLeft(-HalfWidth, -HalfHeight, 0.0f);
        FVector LocalBottomRight(HalfWidth, -HalfHeight, 0.0f);

        // 회전 행렬 적용 (2D 회전, Z축 중심)
        float CosTheta = FMath::Cos(FMath::DegreesToRadians(Rotation));
        float SinTheta = FMath::Sin(FMath::DegreesToRadians(Rotation));

        // 회전 변환 적용
        TopLeft = FVector(
            LocalTopLeft.X * CosTheta - LocalTopLeft.Y * SinTheta,
            LocalTopLeft.X * SinTheta + LocalTopLeft.Y * CosTheta,
            0.0f) + Center;

        TopRight = FVector(
            LocalTopRight.X * CosTheta - LocalTopRight.Y * SinTheta,
            LocalTopRight.X * SinTheta + LocalTopRight.Y * CosTheta,
            0.0f) + Center;

        BottomLeft = FVector(
            LocalBottomLeft.X * CosTheta - LocalBottomLeft.Y * SinTheta,
            LocalBottomLeft.X * SinTheta + LocalBottomLeft.Y * CosTheta,
            0.0f) + Center;

        BottomRight = FVector(
            LocalBottomRight.X * CosTheta - LocalBottomRight.Y * SinTheta,
            LocalBottomRight.X * SinTheta + LocalBottomRight.Y * CosTheta,
            0.0f) + Center;
    }

    // 사각형의 중심점 계산
    FVector GetCenter() const
    {
        return (TopLeft + TopRight + BottomLeft + BottomRight) * 0.25f;
    }

    // 사각형의 모든 꼭지점을 배열로 반환
    TArray<FVector> GetVertices() const
    {
        return { TopLeft, TopRight, BottomRight, BottomLeft };
    }

    // 사각형의 모든 변(edge)을 반환
    TArray<TPair<FVector, FVector>> GetEdges() const
    {
        TArray<TPair<FVector, FVector>> Edges;
        Edges.Add(TPair<FVector, FVector>(TopLeft, TopRight));
        Edges.Add(TPair<FVector, FVector>(TopRight, BottomRight));
        Edges.Add(TPair<FVector, FVector>(BottomRight, BottomLeft));
        Edges.Add(TPair<FVector, FVector>(BottomLeft, TopLeft));
        return Edges;
    }
};

/**
 * 속도 장애물(Velocity Obstacle) 구조체
 */
USTRUCT(BlueprintType)
struct FVelocityObstacle
{
    GENERATED_BODY()

    // VO는 unbounded polygon이므로 경계 정보와 탄젠트 벡터를 저장
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RRVO")
    TArray<FVector> Boundary;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RRVO")
    FVector LeftLeg;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RRVO")
    FVector RightLeg;

    FVelocityObstacle()
    {
        Boundary.Empty();
        LeftLeg = FVector::ZeroVector;
        RightLeg = FVector::ZeroVector;
    }
};

/**
 * 선형 프로그램 구조체 (Linear Program)
 */
USTRUCT(BlueprintType)
struct FLinearProgram
{
    GENERATED_BODY()

    // Ax <= b 형태의 선형 제약 조건
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RRVO")
    TArray<FVector2D> A; // 제약 조건의 계수 (x와 y 계수만 필요)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RRVO")
    TArray<float> b; // 제약 조건의 상수

    // 목적 함수 계수 (최소화할 대상)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RRVO")
    FVector2D ObjectiveCoefficients;

    FLinearProgram()
    {
        A.Empty();
        b.Empty();
        ObjectiveCoefficients = FVector2D(0.0f, 0.0f);
    }

    // 제약 조건 추가
    void AddConstraint(const FVector2D& Coefficients, float Constant)
    {
        A.Add(Coefficients);
        b.Add(Constant);
    }
};

UCLASS()
class RRVO_API ARRVOAgent : public AActor
{
    GENERATED_BODY()
    
public:    
    // 기본 설정
    ARRVOAgent();

protected:
    virtual void BeginPlay() override;

public:    
    virtual void Tick(float DeltaTime) override;

    // 에이전트 속성
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RRVO")
    FRRVORect Geometry;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RRVO")
    FVector Velocity;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RRVO")
    float Orientation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RRVO")
    float MaxAngularVelocity;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RRVO")
    float PreferredSpeed;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RRVO")
    FVector TargetPosition;

    // 이웃 에이전트 목록
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RRVO")
    TArray<ARRVOAgent*> Neighbors;

    // RRVO 알고리즘 구현 메서드
    
    /**
     * 알고리즘 1: 새로운 속도 계산 (선형 제약 조건 기반)
     */
    UFUNCTION(BlueprintCallable, Category = "RRVO")
    FVector ComputeNewVelocity();

    /**
     * 알고리즘 2: 에이전트 b가 에이전트 a에 유도하는 속도 장애물 계산
     */
    UFUNCTION(BlueprintCallable, Category = "RRVO")
    FVelocityObstacle ComputeVelocityObstacle(ARRVOAgent* OtherAgent, float TimeHorizon);

    /**
     * 알고리즘 3: 새로운 속도와 방향 계산
     */
    UFUNCTION(BlueprintCallable, Category = "RRVO")
    TPair<FVector, float> ComputeNewVelocityAndOrientation();

    /**
     * Minkowski Sum 계산
     */
    UFUNCTION(BlueprintCallable, Category = "RRVO")
    FRRVORect ComputeMinkowskiSum(const FRRVORect& RectA, const FRRVORect& RectB);

    /**
     * 원점을 지나는 접선 계산
     */
    UFUNCTION(BlueprintCallable, Category = "RRVO")
    TPair<FVector, FVector> ComputeTangents(const FRRVORect& Rect, const FVector& Point);

    /**
     * 선형 프로그램 해결
     */
    UFUNCTION(BlueprintCallable, Category = "RRVO")
    FVector SolveLinearProgram(const FLinearProgram& LP);

    /**
     * 에이전트 업데이트 (새 속도와 방향으로 이동)
     */
    UFUNCTION(BlueprintCallable, Category = "RRVO")
    void UpdateAgent(float DeltaTime);
};