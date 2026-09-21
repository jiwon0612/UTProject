// Fill out your copyright notice in the Description page of Project Settings.


#include "CJW/Player/UT1Player.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimInstance.h"
#include "UT1/CJW/Weapons/UT1WeaponData.h"

AUT1Player::AUT1Player()
{
	PrimaryActorTick.bCanEverTick = true;

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->SetRelativeRotation(FRotator(-50.0f, 45.0f, 0.0f));
	SpringArm->SetUsingAbsoluteRotation(true);
	SpringArm->TargetArmLength = 400.0f;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm);

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 500.f, 0.f);
	bUseControllerRotationYaw = false;
}

void AUT1Player::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (PlayerController)
	{
		auto* Subsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer());
		if (Subsystem)
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}

}

void AUT1Player::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);


	auto* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent);
	if (EnhancedInputComponent)
	{
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this,&AUT1Player::Input_Move);
		EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Started, this, &AUT1Player::ComboAttack);
	}
}

void AUT1Player::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 윈도우가 열리기 전에 들어온 선입력을 여기서 기다렸다가 발동시킨다.
	// 예약이 없으면 볼 것도 없으므로 곧바로 빠져나간다.
	if (bIsAttacking == false || bComboQueued == false)
	{
		return;
	}

	const FComboStep* Step = GetCurrentComboStep();
	if (Step == nullptr)
	{
		return;
	}

	float Position = 0.0f;
	if (GetComboMontagePosition(Position) == false)
	{
		return;
	}

	// ComboWindowEnd 는 여기서 보지 않는다. 입력 시점에 이미 검사했고,
	// 프레임이 크게 튀어 윈도우를 통째로 건너뛴 경우까지 플레이어 탓으로
	// 돌리면 억울한 입력 소실이 된다.
	if (Position >= Step->ComboWindowStart)
	{
		AdvanceCombo();
	}
}

void AUT1Player::ComboAttack()
{
	// 공격 중이 아니면 콤보 1단부터 시작한다.
	if (bIsAttacking == false)
	{
		ComboIndex = 0;
		PlayComboStep();
		return;
	}

	const FComboStep* Step = GetCurrentComboStep();
	if (Step == nullptr)
	{
		return;
	}

	float Position = 0.0f;
	if (GetComboMontagePosition(Position) == false)
	{
		return;
	}

	// 윈도우를 놓친 입력은 흘려보낸다. 여기서 예약해 버리면 아무 때나 누른
	// 연타가 전부 콤보로 이어져서, 타이밍을 맞춘 입력과 구분되지 않는다.
	if (Position > Step->ComboWindowEnd)
	{
		return;
	}

	// 마지막 단계에서는 예약을 받지 않는다. 넘길 곳이 없는데 예약을 받으면
	// AdvanceCombo 가 범위를 벗어나 ComboReset 을 부르고, 몽타주가 아직
	// 재생 중인 상태로 이동이 풀려 버린다.
	if (HasNextComboStep() == false)
	{
		return;
	}

	// 아직 윈도우 전이라면 선입력으로 등록해 두고 Tick 이 열릴 때까지 기다린다.
	bComboQueued = true;

	// 이미 윈도우 안이면 기다릴 이유가 없다. 즉시 다음 단계로 넘어간다.
	if (Position >= Step->ComboWindowStart)
	{
		AdvanceCombo();
	}
}

void AUT1Player::AdvanceCombo()
{
	ComboIndex++;
	PlayComboStep();
}

void AUT1Player::PlayComboStep()
{
	const TArray<FComboStep>& Combo = TestWeaponData->ComboSequence;
	if (Combo.IsValidIndex(ComboIndex) == false)
	{
		ComboReset();
		return;
	}

	UAnimMontage* Montage = Combo[ComboIndex].Montage;
	if (Montage == nullptr)
	{
		ComboReset();
		return;
	}

	RotateToCursor();

	bIsAttacking = true;
	bComboQueued = false;

	UAnimInstance* Anim = GetMesh()->GetAnimInstance();
	if (Anim == nullptr) return;

	// 재생 중인 몽타주 위에 그대로 올린다. 이전 몽타주는 블렌드되며 밀려나고
	// 종료 델리게이트가 bInterrupted = true 로 불린다.
	CurrentComboMontage = Montage;
	Anim->Montage_Play(Montage);

	FOnMontageEnded EndDelegate;
	EndDelegate.BindUObject(this, &AUT1Player::OnMontageEnd);
	Anim->Montage_SetEndDelegate(EndDelegate, Montage);
}

void AUT1Player::ComboReset()
{
	ComboIndex = 0;
	bIsAttacking = false;
	bComboQueued = false;
	CurrentComboMontage = nullptr;
	GetCharacterMovement()->bOrientRotationToMovement = true;
}

void AUT1Player::OnMontageEnd(UAnimMontage* Montage, bool bInterrupted)
{
	if (bInterrupted) return;   // 다음 콤보 몽타주에 밀린 경우. 여기서 처리하지 않는다.

	// 정상 종료인데 예약이 남아 있다면 ComboWindowStart 가 몽타주 길이보다 뒤에
	// 잡혀 윈도우가 한 번도 열리지 않은 것이다. 데이터 설정 오류지만 입력을
	// 삼키지는 않고 살려 준다.
	if (bComboQueued)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[Combo] %d단 ComboWindowStart 가 몽타주 길이보다 깁니다. 선입력을 종료 시점에 처리합니다."),
			ComboIndex);
		AdvanceCombo();
		return;
	}

	ComboReset();
}

bool AUT1Player::HasNextComboStep() const
{
	return TestWeaponData != nullptr && TestWeaponData->ComboSequence.IsValidIndex(ComboIndex + 1);
}

const FComboStep* AUT1Player::GetCurrentComboStep() const
{
	if (TestWeaponData == nullptr)
	{
		return nullptr;
	}

	const TArray<FComboStep>& Combo = TestWeaponData->ComboSequence;
	return Combo.IsValidIndex(ComboIndex) ? &Combo[ComboIndex] : nullptr;
}

bool AUT1Player::GetComboMontagePosition(float& OutPosition) const
{
	if (CurrentComboMontage == nullptr)
	{
		return false;
	}

	UAnimInstance* Anim = GetMesh()->GetAnimInstance();
	if (Anim == nullptr || Anim->Montage_IsPlaying(CurrentComboMontage) == false)
	{
		return false;
	}

	OutPosition = Anim->Montage_GetPosition(CurrentComboMontage);
	return true;
}

void AUT1Player::RotateToCursor()
{
	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (PlayerController)
	{
		FHitResult Hit;
		if (PlayerController->GetHitResultUnderCursor(ECC_Visibility, false, Hit))
		{
			FVector Direction = Hit.ImpactPoint - GetActorLocation();
			Direction.Z = 0.0f;
			if (Direction.IsNearlyZero())
			{
				Direction = GetActorForwardVector();
			}

			SetActorRotation(FRotator(0.f, Direction.Rotation().Yaw, 0.f));
		}
	}
}

void AUT1Player::Input_Move(const FInputActionValue& InputValue)
{  
	if (bIsAttacking) return;

	FVector2D MovementVector = InputValue.Get<FVector2D>();

	if (Controller != nullptr)
	{
		const FRotator CamYaw(0.f, SpringArm->GetComponentRotation().Yaw, 0.f);
		const FVector Forward = FRotationMatrix(CamYaw).GetUnitAxis(EAxis::X);
		const FVector Right = FRotationMatrix(CamYaw).GetUnitAxis(EAxis::Y);

		AddMovementInput(Forward, MovementVector.Y);
		AddMovementInput(Right, MovementVector.X);
	}
}
