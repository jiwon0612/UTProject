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

	// 비행 중 따라붙는 이펙트임. 지정하면 기본 구체 메시는 숨김
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|VFX")
	TObjectPtr<class UNiagaraSystem> TrailEffect;

	// 충돌 지점에 한 번 터뜨리는 이펙트임
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|VFX")
	TObjectPtr<class UNiagaraSystem> ImpactEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|VFX")
	FVector ImpactEffectScale = FVector::OneVector;

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<class USphereComponent> Collision;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<class UStaticMeshComponent> Visual;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<class UProjectileMovementComponent> Movement;

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
