// Fill out your copyright notice in the Description page of Project Settings.


#include "UETest/Public/UKBlueprintFunctionLibrary.h"

FVector UUKBlueprintFunctionLibrary::GetMiniMapUIRotationLocation(FVector Location, float Angle, FVector NewLocation, float NewAngle)
{
	FTransform CurrentTransform(FRotator(0.0f, Angle, 0.0f), Location);

	FTransform NewTranslationTransform(FQuat::Identity, NewLocation);
	FRotator NewRotation(0.f, NewAngle, 0.f); // Pitch, Yaw, Roll
	FTransform RotationTransform(NewRotation);

	// 최종 변환을 계산합니다: 먼저 회전 후 이동
	FTransform FinalTransform = NewTranslationTransform * RotationTransform * CurrentTransform;

	return FinalTransform.GetLocation();
}
