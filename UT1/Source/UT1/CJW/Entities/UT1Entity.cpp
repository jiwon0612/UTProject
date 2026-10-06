// Fill out your copyright notice in the Description page of Project Settings.


#include "CJW/Entities/UT1Entity.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimInstance.h"
#include "UT1.h"

AUT1Entity::AUT1Entity()
{
	PrimaryActorTick.bCanEverTick = true;

	// 스프링암의 카메라 충돌 검사는 ECC_Camera 채널로 탐침을 쏜다.
	// 엔티티가 이 채널을 막으면, 적이 플레이어와 카메라 사이를 지날 때마다
	// 카메라가 확 당겨졌다 풀린다. 캐릭터는 카메라를 막지 않는다.
	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	}

	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		MeshComp->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	}
}

void AUT1Entity::BeginPlay()
{
	Super::BeginPlay();

	CurrentHealth = MaxHealth;
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
}

void AUT1Entity::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AUT1Entity::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

float AUT1Entity::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
	AController* EventInstigator, AActor* DamageCauser)
{
	// 죽은 뒤 들어오는 피해는 버린다. 무기 트레이스는 날을 여러 점으로 쪼개
	// 검사하므로 치명타 프레임에 여러 건이 몰리는데, 막지 않으면 사망 처리가
	// 두 번 이상 돌아 몽타주와 델리게이트가 중복된다.
	if (bIsDead || DamageAmount <= 0.0f)
	{
		return 0.0f;
	}

	const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	if (ActualDamage <= 0.0f)
	{
		return 0.0f;
	}

	CurrentHealth = FMath::Max(0.0f, CurrentHealth - ActualDamage);
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);

	if (CurrentHealth <= 0.0f)
	{
		HandleDeath(DamageCauser);
	}
	else
	{
		HandleDamaged(ActualDamage, DamageCauser);
	}

	return ActualDamage;
}

void AUT1Entity::Heal(float Amount)
{
	if (bIsDead || Amount <= 0.0f)
	{
		return;
	}

	CurrentHealth = FMath::Min(MaxHealth, CurrentHealth + Amount);
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
}

void AUT1Entity::HandleDamaged(float ActualDamage, AActor* DamageCauser)
{
	if (HitReactMontage == nullptr || GetMesh() == nullptr)
	{
		return;
	}

	if (UAnimInstance* Anim = GetMesh()->GetAnimInstance())
	{
		Anim->Montage_Play(HitReactMontage);
	}
}

void AUT1Entity::HandleDeath(AActor* Killer)
{
	bIsDead = true;

	// 시체가 길을 막거나 계속 피격 판정에 걸리지 않도록 정리한다.
	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->StopMovementImmediately();
		Move->DisableMovement();
	}

	OnDied.Broadcast(this);

	if (DeathMontage != nullptr && GetMesh() != nullptr)
	{
		if (UAnimInstance* Anim = GetMesh()->GetAnimInstance())
		{
			Anim->Montage_Play(DeathMontage);
		}
	}

	UE_LOG(LogUT1, Log, TEXT("[Entity] %s 사망 (killer=%s)"),
		*GetName(), (Killer != nullptr) ? *Killer->GetName() : TEXT("None"));
}
