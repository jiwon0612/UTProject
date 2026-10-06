// Fill out your copyright notice in the Description page of Project Settings.


#include "CJW/Combat/UT1DodgeComponent.h"
#include "CJW/Entities/UT1Entity.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/RootMotionSource.h"
#include "Animation/AnimMontage.h"
#include "Engine/World.h"
#include "TimerManager.h"

UUT1DodgeComponent::UUT1DodgeComponent()
{
	// 시간 관리는 타이머로 하므로 매 프레임 볼 일이 없다.
	PrimaryComponentTick.bCanEverTick = false;
}

AUT1Entity* UUT1DodgeComponent::GetOwnerEntity() const
{
	return Cast<AUT1Entity>(GetOwner());
}

bool UUT1DodgeComponent::CanDodge() const
{
	const AUT1Entity* Owner = GetOwnerEntity();
	if (Owner == nullptr || Owner->IsDead() || bIsDodging)
	{
		return false;
	}

	return GetCooldownRemaining() <= 0.0f;
}

float UUT1DodgeComponent::GetCooldownRemaining() const
{
	const UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return 0.0f;
	}

	return FMath::Max(0.0f, static_cast<float>(CooldownEndTime - World->GetTimeSeconds()));
}

bool UUT1DodgeComponent::TryDodge(FVector Direction)
{
	if (CanDodge() == false)
	{
		return false;
	}

	AUT1Entity* Owner = GetOwnerEntity();
	UCharacterMovementComponent* Move = Owner->GetCharacterMovement();
	if (Move == nullptr)
	{
		return false;
	}

	Direction.Z = 0.0f;
	if (Direction.Normalize() == false)
	{
		Direction = Owner->GetActorForwardVector().GetSafeNormal2D();
	}

	// 몽타주가 정면으로 구르는 모션이므로 몸을 회피 방향으로 돌려 둔다.
	Owner->SetActorRotation(FRotator(0.0f, Direction.Rotation().Yaw, 0.0f));

	// Override 모드라 회피 중에는 이동 입력과 가속이 무시되고 이 속도만 쓰인다.
	TSharedPtr<FRootMotionSource_ConstantForce> Force = MakeShared<FRootMotionSource_ConstantForce>();
	Force->InstanceName = TEXT("UT1Dodge");
	Force->AccumulateMode = ERootMotionAccumulateMode::Override;
	Force->Priority = 5;
	Force->Force = Direction * (Distance / Duration);
	Force->Duration = Duration;

	// 이동이 끝난 순간의 속도. 그냥 두면 대시 속도 그대로 미끄러진다.
	// 후딜이 있으면 제자리에서 자세를 회복해야 하므로 바로 세우고,
	// 후딜이 없으면 곧장 달리기로 이어지도록 걷기 속도로만 잘라 둔다.
	if (RecoveryDuration > 0.0f)
	{
		Force->FinishVelocityParams.Mode = ERootMotionFinishVelocityMode::SetVelocity;
		Force->FinishVelocityParams.SetVelocity = FVector::ZeroVector;
	}
	else
	{
		Force->FinishVelocityParams.Mode = ERootMotionFinishVelocityMode::ClampVelocity;
		Force->FinishVelocityParams.ClampVelocity = Move->MaxWalkSpeed;
	}
	RootMotionSourceID = Move->ApplyRootMotionSource(Force);

	bIsDodging = true;

	// 이동은 Duration 에 끝나지만 회피 상태(입력 차단)는 후딜까지 유지한다.
	const float TotalDuration = Duration + RecoveryDuration;

	FTimerManager& Timers = GetWorld()->GetTimerManager();
	Timers.SetTimer(DodgeTimer, this, &UUT1DodgeComponent::FinishDodge, TotalDuration, false);

	if (InvulnerableDuration > 0.0f)
	{
		Owner->AddInvulnerable();
		bInvulnerableActive = true;
		Timers.SetTimer(InvulnerableTimer, this, &UUT1DodgeComponent::EndInvulnerable, InvulnerableDuration, false);
	}

	if (DodgeMontage != nullptr)
	{
		// 게임플레이 수치(이동 + 후딜)가 기준이고 애니메이션이 거기에 맞춘다.
		// 그래야 수치를 조절하거나 강화로 바꿔도 회피 상태와 모션이 함께 끝난다.
		// GetPlayLength 는 에셋 Rate Scale 이 빠진 길이이므로 에셋 쪽은 1 로 둔다.
		const float MontageLength = DodgeMontage->GetPlayLength();
		const float PlayRate = (MontageLength > 0.0f) ? MontageLength / TotalDuration : 1.0f;
		Owner->PlayAnimMontage(DodgeMontage, PlayRate);
	}

	OnDodgeStarted.Broadcast();
	return true;
}

void UUT1DodgeComponent::FinishDodge()
{
	// 후딜까지 끝난 시점. 이동은 Duration 이 지날 때 CharacterMovement 가 이미 정리했다.
	// 여기서 지우면 FinishVelocityParams 가 적용되지 않을 수 있어 건드리지 않는다.
	bIsDodging = false;
	CooldownEndTime = GetWorld()->GetTimeSeconds() + Cooldown;

	OnDodgeEnded.Broadcast();
}

void UUT1DodgeComponent::EndInvulnerable()
{
	if (bInvulnerableActive == false)
	{
		return;
	}
	bInvulnerableActive = false;

	if (AUT1Entity* Owner = GetOwnerEntity())
	{
		Owner->RemoveInvulnerable();
	}
}

void UUT1DodgeComponent::CancelDodge()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DodgeTimer);
		World->GetTimerManager().ClearTimer(InvulnerableTimer);
	}

	EndInvulnerable();

	if (bIsDodging == false)
	{
		return;
	}
	bIsDodging = false;

	if (AUT1Entity* Owner = GetOwnerEntity())
	{
		if (UCharacterMovementComponent* Move = Owner->GetCharacterMovement())
		{
			Move->RemoveRootMotionSourceByID(RootMotionSourceID);
		}
	}

	OnDodgeEnded.Broadcast();
}

void UUT1DodgeComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CancelDodge();

	Super::EndPlay(EndPlayReason);
}
