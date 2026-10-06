// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UT1Weapon.generated.h"

class UStaticMeshComponent;

/**
 * 한 번의 판정 Tick 에 필요한 공격 정보. 플레이어가 강화 스탯을 반영해 채워 넘긴다.
 * 치명타는 여기서 굴리지 않고 무기가 "맞힐 때마다" 굴린다. 한 번 휘둘러
 * 여러 적을 맞혔을 때 적마다 따로 판정되게 하기 위해서다.
 */
struct FUT1AttackInfo
{
	float Damage = 0.0f;
	float CritChance = 0.0f;
	float CritDamageMultiplier = 1.5f;
};

/**
 * 손에 들리는 무기 액터.
 *
 * GripRoot 가 캐릭터의 Weapon_R / Weapon_L 소켓에 붙는 기준점이다.
 * 무기 메시마다 피벗 위치가 제각각이라(어떤 건 바닥, 어떤 건 중심)
 * 소켓에 그대로 붙이면 엉뚱한 곳에 생긴다.
 *
 * 그래서 보정을 코드가 아니라 무기 BP 가 책임진다. 무기 BP 에서
 * WeaponMesh 를 옮겨 손잡이가 GripRoot 에 오도록 맞춰 두면,
 * 장착 코드는 보정값을 전혀 몰라도 모든 무기를 같은 방식으로 붙일 수 있다.
 */
UCLASS()
class UT1_API AUT1Weapon : public AActor
{
	GENERATED_BODY()

public:
	AUT1Weapon();

	UStaticMeshComponent* GetWeaponMesh() const { return WeaponMesh; }

	// --- 공격 판정 ---
	// NotifyState 가 Begin/Tick/End 로 불러 준다.
	// 상태(적중 목록, 직전 위치)는 노티파이가 아니라 여기에 둔다.
	// UAnimNotifyState 는 CDO 하나를 공유하므로 인스턴스 상태를 담을 수 없다.
	void BeginAttackTrace();
	void TickAttackTrace(const FUT1AttackInfo& AttackInfo);
	void EndAttackTrace();

public:
	// 소켓에 붙는 기준점. 곧 손잡이 위치다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<USceneComponent> GripRoot;

	// 무기 BP 에서 손잡이가 GripRoot 에 오도록 위치를 맞춘다.
	// 나중에 타격 판정용 tip / base 소켓도 이 메시에 붙이게 된다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<UStaticMeshComponent> WeaponMesh;

	// 날의 양 끝. 무기 BP 에서 칼밑동과 칼끝에 각각 가져다 놓는다.
	// 스태틱 메시에 소켓을 파지 않아도 되고, GripRoot 를 맞출 때와
	// 같은 뷰포트 작업으로 끝난다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|Trace")
	TObjectPtr<USceneComponent> TraceStart;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|Trace")
	TObjectPtr<USceneComponent> TraceEnd;

	// 날을 따라 몇 점을 검사할지. 길고 얇은 무기일수록 늘린다.
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Trace", meta = (ClampMin = "2"))
	int32 TraceSampleCount = 4;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Trace", meta = (ClampMin = "0.0"))
	float TraceRadius = 4.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Trace")
	bool bDrawDebugTrace = false;

private:
	// 이번 스윙에서 이미 때린 대상. 한 번 휘두를 때 한 번만 맞게 한다.
	UPROPERTY()
	TSet<TObjectPtr<AActor>> HitActorsThisSwing;

	// 직전 프레임의 샘플 위치. 프레임 사이를 이어서 쓸어야
	// 빠른 휘두르기가 적을 통과해 버리지 않는다.
	TArray<FVector> PreviousSamplePoints;

	bool bTracing = false;

	void BuildSamplePoints(TArray<FVector>& Out) const;
};
