// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UT1DamageNumber.generated.h"

class UWidgetComponent;

/**
 * 맞은 자리에서 떠올랐다 사라지는 데미지 숫자. UUT1CombatFeedbackSubsystem 이 스폰한다.
 *
 * WidgetComponent 를 화면 공간(Screen)으로 써서 카메라 거리와 상관없이 같은 크기로
 * 또렷하게 보인다. 월드 위치는 액터가 들고 있고, 화면에 그리는 것은 위젯이 한다.
 *
 * 한 타격에 하나씩 생기고 Lifetime 뒤에 스스로 사라진다. 지금 규모(초당 수십 개 이하)에서는
 * 풀링 없이 스폰/파괴해도 충분하다. 다수 적을 동시에 때리는 구조가 되면 풀링을 고려한다.
 */
UCLASS()
class UT1_API AUT1DamageNumber : public AActor
{
	GENERATED_BODY()

public:
	AUT1DamageNumber();

	// SpawnActorDeferred 와 FinishSpawning 사이에 부른다.
	void Setup(float InDamage, bool bInCritical, const FLinearColor& InColor);

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DamageNumber")
	TObjectPtr<UWidgetComponent> Widget;

	UPROPERTY(EditDefaultsOnly, Category = "DamageNumber")
	float Lifetime = 0.8f;

	// 처음 속도(cm/s). 시간이 지날수록 느려지며 멈춘다.
	UPROPERTY(EditDefaultsOnly, Category = "DamageNumber")
	float RiseSpeed = 160.0f;

private:
	float Damage = 0.0f;
	bool bCritical = false;
	FLinearColor Color = FLinearColor::White;
	float Elapsed = 0.0f;
};
