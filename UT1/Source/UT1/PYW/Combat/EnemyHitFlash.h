#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EnemyHitFlash.generated.h"

/**
 * 공격이 맞은 자리에서 잠깐 터지는 섬광임. 빛 구체와 바닥과 나란한 고리가 커지며 사라지고, 라이트가 함께 번쩍임.
 *
 * ProjectileVFX 팩 Niagara 시스템은 이미터끼리 이벤트로 엮여 있어서 일부 이미터만 켜면 아무것도 그려지지 않고,
 * 전부 켜면 스스로 날아가는 투사체 파티클까지 나옴. 그래서 타격 표시는 메시와 머티리얼로 직접 만들어 항상 제자리에서 보이게 함.
 */
UCLASS()
class UT1_API AEnemyHitFlash : public AActor
{
	GENERATED_BODY()

public:
	AEnemyHitFlash();

	/** Location에 섬광을 만듦. Radius는 다 커졌을 때 빛 구체 반지름(cm)임 */
	static AEnemyHitFlash* Spawn(UWorld* World, const FVector& Location, const FLinearColor& Color, float Radius);

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitFlash")
	FLinearColor Color = FLinearColor(1.0f, 0.2f, 1.6f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitFlash", meta = (ClampMin = "1.0"))
	float Radius = 60.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitFlash", meta = (ClampMin = "0.05"))
	float Lifetime = 0.25f;

	// 시작 순간 머티리얼 밝기임. HDR이라 블룸으로 번져 보임
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitFlash", meta = (ClampMin = "0.0"))
	float PeakIntensity = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitFlash", meta = (ClampMin = "0.0"))
	float PeakLightIntensity = 120.0f;

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<class UStaticMeshComponent> Burst;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<class UStaticMeshComponent> Ring;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<class UPointLightComponent> Flash;

	UPROPERTY(Transient)
	TObjectPtr<class UMaterialInstanceDynamic> BurstMaterial;

	UPROPERTY(Transient)
	TObjectPtr<class UMaterialInstanceDynamic> RingMaterial;

	float Age = 0.0f;
	float RingMeshSize = 100.0f;

	void UpdateFlash(float Alpha);
};
