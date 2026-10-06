#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EnemyProjectile.generated.h"

UCLASS(Blueprintable)
class UT1_API AEnemyProjectile : public AActor
{
	GENERATED_BODY()

public:
	AEnemyProjectile();
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	float Damage = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	float Speed = 1200.0f;

	// 0이면 직선으로 날아감. 1이면 월드 중력을 받아 포물선을 그림
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile", meta = (ClampMin = "0.0"))
	float GravityScale = 0.0f;

	// 보여 주기 전용 포탄임. 충돌·피해 없이 수명이 다하면 사라지고, 피해는 던진 쪽의 범위 판정이 맡음
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	bool bVisualOnly = false;

	// 비행 중 따라붙는 선택 이펙트임. 머리 파티클이 스스로 속도를 갖는 시스템(ProjectileVFX 팩 등)은
	// 액터와 따로 날아가므로 쓰지 않음. 기본 비주얼은 아래 코어와 꼬리 메시임
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|VFX")
	TObjectPtr<class UNiagaraSystem> TrailEffect;

	// 코어 구체에 입히는 머티리얼임. 발광 머티리얼을 주면 이펙트가 작아도 판정 위치가 잘 보임
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|VFX")
	TObjectPtr<class UMaterialInterface> CoreMaterial;

	// 코어 뒤로 뻗는 원뿔 꼬리의 머티리얼임. 같은 액터의 컴포넌트라 투사체와 항상 함께 움직임
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|VFX")
	TObjectPtr<class UMaterialInterface> TailMaterial;

	// 꼬리 길이(cm)임. 0이면 꼬리를 숨김
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|VFX", meta = (ClampMin = "0.0"))
	float TailLength = 120.0f;

	// 충돌 구체 지름 대비 바깥 꼬리 밑면 지름 배율임. 헤일로보다 좁으면 빛무리에 묻혀 안 보임
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|VFX", meta = (ClampMin = "0.0"))
	float TailWidthScale = 1.0f;

	// 바깥 꼬리 안쪽을 지나는 가늘고 밝은 선임. 진행 방향을 한눈에 읽히게 함
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|VFX")
	TObjectPtr<class UMaterialInterface> InnerTailMaterial;

	// 바깥 꼬리 대비 안쪽 선의 길이·굵기 배율임
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|VFX", meta = (ClampMin = "0.0"))
	float InnerTailLengthScale = 1.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|VFX", meta = (ClampMin = "0.0"))
	float InnerTailWidthScale = 0.4f;

	// 코어를 감싸는 반투명 발광 구체임. 코어(판정 크기)는 그대로 두고 눈에 띄는 크기만 키움
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|VFX")
	TObjectPtr<class UMaterialInterface> HaloMaterial;

	// 충돌 구체 지름 대비 헤일로 지름 배율임
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|VFX", meta = (ClampMin = "0.0"))
	float HaloScale = 2.6f;

	// 투사체 주변 바닥과 벽을 비추는 빛임. 0이면 끔
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|VFX", meta = (ClampMin = "0.0"))
	float GlowIntensity = 40.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|VFX")
	FLinearColor GlowColor = FLinearColor(0.7f, 0.15f, 1.0f);

	// 발사 지점에서 한 번 터뜨리는 섬광임. 월드 위치에 남으므로 투사체와 따로 놀지 않음
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|VFX")
	TObjectPtr<class UNiagaraSystem> LaunchEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|VFX")
	FVector LaunchEffectScale = FVector::OneVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|VFX")
	TArray<FName> LaunchDisabledEmitters;

	// 이펙트 팩 기본 크기(약 2m)가 충돌 구체보다 훨씬 커서, 투사체 크기에 맞게 줄여 씀
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|VFX")
	FVector TrailEffectScale = FVector::OneVector;

	// 충돌 지점에 한 번 터뜨리는 이펙트임
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|VFX")
	TObjectPtr<class UNiagaraSystem> ImpactEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|VFX")
	FVector ImpactEffectScale = FVector::OneVector;

	// 충돌 지점에서 터지는 섬광(AEnemyHitFlash)의 반지름임. 색은 GlowColor를 따름
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|VFX", meta = (ClampMin = "0.0"))
	float ImpactFlashRadius = 40.0f;

	// 이펙트 팩 시스템은 발사·비행·폭발 이미터를 한 시스템에 담고 있어서, 단계에 맞지 않는 이미터를 꺼서 씀
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|VFX")
	TArray<FName> TrailDisabledEmitters;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|VFX")
	TArray<FName> ImpactDisabledEmitters;

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<class USphereComponent> Collision;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<class UStaticMeshComponent> Visual;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<class UStaticMeshComponent> Tail;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<class UStaticMeshComponent> InnerTail;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<class UStaticMeshComponent> Halo;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<class UPointLightComponent> Glow;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<class UProjectileMovementComponent> Movement;

	/** 원뿔 밑면을 코어에 붙이고 꼭짓점이 진행 반대 방향(-X)을 향하도록 크기와 위치를 맞춤 */
	void LayoutTail(class UStaticMeshComponent* Cone, float Length, float Diameter) const;

	UFUNCTION()
	void OnProjectileHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit);

	UFUNCTION()
	void OnProjectileOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	void ApplyProjectileDamage(AActor* OtherActor);

	/** 피해, 폭발 이펙트, 제거를 한 번만 처리함. 같은 프레임에 Hit와 Overlap이 함께 와도 중복되지 않음 */
	void Explode(AActor* OtherActor, const FVector& ImpactLocation);

	bool bExploded = false;
};
