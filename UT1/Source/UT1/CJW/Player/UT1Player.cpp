// Fill out your copyright notice in the Description page of Project Settings.


#include "CJW/Player/UT1Player.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimInstance.h"
#include "UT1/CJW/Weapons/UT1WeaponData.h"
#include "CJW/Weapons/UT1Weapon.h"
#include "CJW/Combat/UT1DodgeComponent.h"
#include "Engine/World.h"
#include "UT1.h"

// 무기가 붙을 소켓 이름. FEquipmentData::bIsRightHanded 가 둘 중 하나를 고른다.
// 스켈레톤과의 약속이라, 무기를 드는 적을 추가할 때도 그 스켈레톤에 같은
// 이름의 소켓을 만들어 두어야 코드가 상대를 구분하지 않아도 된다.
static const FName WeaponSocket_Right(TEXT("Hand_R"));
static const FName WeaponSocket_Left(TEXT("Hand_L"));

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

	DodgeComponent = CreateDefaultSubobject<UUT1DodgeComponent>(TEXT("DodgeComponent"));
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

		if (DodgeAction != nullptr)
		{
			EnhancedInputComponent->BindAction(DodgeAction, ETriggerEvent::Started, this, &AUT1Player::Input_Dodge);
		}

		// 회피 방향은 "지금 누르고 있는 이동 키"로 정한다. Input_Move 는 공격 중에
		// 일찍 빠져나가므로 거기서 값을 저장할 수 없다. 대신 이동 액션의 현재 값을
		// 언제든 읽을 수 있도록 값 바인딩을 걸어 둔다.
		EnhancedInputComponent->BindActionValue(MoveAction);
	}
}

void AUT1Player::BeginPlay()
{
	Super::BeginPlay();

	// TestWeaponData 는 에디터(BP_Player)에서 지정하는 시작 무기다.
	// 상점이나 획득으로 무기를 바꿀 때도 같은 EquipWeaponData 를 쓰면 된다.
	EquipWeaponData(TestWeaponData);
}

void AUT1Player::EquipWeaponData(UUT1WeaponData* NewWeaponData)
{
	// 콤보 도중에 무기가 바뀌면 ComboIndex 가 새 무기의 시퀀스를 가리켜
	// 엉뚱한 단계가 재생된다. 재생 중인 몽타주를 먼저 끊고 상태를 되돌린다.
	CancelCombo();

	ClearEquippedWeapons();
	CurrentWeaponData = NewWeaponData;

	if (CurrentWeaponData == nullptr)
	{
		return;   // 맨손 상태
	}

	USkeletalMeshComponent* OwnerMesh = GetMesh();
	if (OwnerMesh == nullptr)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	for (const FEquipmentData& Equip : CurrentWeaponData->EquipMeshes)
	{
		if (Equip.WeaponClass == nullptr)
		{
			continue;
		}

		const FName SocketName = Equip.bIsRightHanded ? WeaponSocket_Right : WeaponSocket_Left;

		// 소켓 이름이 틀리면 AttachToComponent 는 경고 없이 캐릭터 원점에
		// 붙여 버린다. 무기가 발밑에 누워 있는 증상의 원인이라 미리 걸러 둔다.
		if (OwnerMesh->DoesSocketExist(SocketName) == false)
		{
			UE_LOG(LogUT1, Warning,
				TEXT("[Equip] 소켓 '%s' 를 찾지 못해 '%s' 를 장착하지 않습니다."),
				*SocketName.ToString(), *Equip.WeaponClass->GetName());
			continue;
		}

		FActorSpawnParameters Params;
		Params.Owner = this;
		Params.Instigator = this;
		// 무기는 캐릭터 몸 안쪽에 생성된다. 기본 겹침 처리를 그대로 두면
		// 위치가 밀리거나 스폰 자체가 취소된다.
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		AUT1Weapon* Weapon = World->SpawnActor<AUT1Weapon>(Equip.WeaponClass, Params);
		if (Weapon == nullptr)
		{
			continue;
		}

		// 소켓에 스냅되는 것은 무기의 GripRoot 다.
		// 손잡이를 GripRoot 에 맞추는 것은 무기 BP 의 책임이므로,
		// 여기서는 무기별 보정값을 알 필요가 없다.
		Weapon->AttachToComponent(
			OwnerMesh,
			FAttachmentTransformRules::SnapToTargetNotIncludingScale,
			SocketName);

		EquippedWeapons.Add(Weapon);
	}
}

void AUT1Player::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 부착된 액터는 부모가 파괴돼도 따라 사라지지 않는다.
	// 정리하지 않으면 플레이어가 죽은 뒤 무기만 월드에 남는다.
	ClearEquippedWeapons();

	Super::EndPlay(EndPlayReason);
}

void AUT1Player::HandleDeath(AActor* Killer)
{
	// 베이스보다 먼저 정리한다. 베이스가 사망 몽타주를 재생하면서
	// 콤보 몽타주를 밀어내면 OnMontageEnd 가 bInterrupted 로 들어와
	// 판정이 켜진 채 남을 수 있다.
	StopWeaponTrace();
	CancelCombo();

	// 회피 도중 죽으면 이동과 무적을 즉시 끊는다.
	DodgeComponent->CancelDodge();

	Super::HandleDeath(Killer);
}

void AUT1Player::StartWeaponTrace()
{
	for (TObjectPtr<AUT1Weapon>& Weapon : EquippedWeapons)
	{
		if (IsValid(Weapon))
		{
			Weapon->BeginAttackTrace();
		}
	}
}

void AUT1Player::TickWeaponTrace()
{
	const float Damage = GetCurrentAttackDamage();
	for (TObjectPtr<AUT1Weapon>& Weapon : EquippedWeapons)
	{
		if (IsValid(Weapon))
		{
			Weapon->TickAttackTrace(Damage);
		}
	}
}

void AUT1Player::StopWeaponTrace()
{
	for (TObjectPtr<AUT1Weapon>& Weapon : EquippedWeapons)
	{
		if (IsValid(Weapon))
		{
			Weapon->EndAttackTrace();
		}
	}
}

float AUT1Player::GetCurrentAttackDamage() const
{
	if (CurrentWeaponData == nullptr)
	{
		return 0.0f;
	}

	const FComboStep* Step = GetCurrentComboStep();
	const float Multiplier = (Step != nullptr) ? Step->DamageMultiplier : 1.0f;
	return CurrentWeaponData->BaseDamage * Multiplier;
}

void AUT1Player::ClearEquippedWeapons()
{
	for (TObjectPtr<AUT1Weapon>& Weapon : EquippedWeapons)
	{
		if (IsValid(Weapon))
		{
			Weapon->Destroy();
		}
	}
	EquippedWeapons.Reset();
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
	// 회피 중 공격 입력은 버린다. 선입력으로 받아 두지는 않는다.
	if (IsDead() || DodgeComponent->IsDodging())
	{
		return;
	}

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
	// 애니메이션이 주도하는 상태는 언제든 샐 수 있다. NotifyEnd 가 어떤
	// 이유로든 불리지 않았더라도 다음 단계로 넘어갈 때 반드시 꺼 둔다.
	StopWeaponTrace();

	if (CurrentWeaponData == nullptr)
		return;

	const TArray<FComboStep>& Combo = CurrentWeaponData->ComboSequence;
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
	StopWeaponTrace();
	ComboIndex = 0;
	bIsAttacking = false;
	bComboQueued = false;
	CurrentComboMontage = nullptr;
	GetCharacterMovement()->bOrientRotationToMovement = true;
}

void AUT1Player::CancelCombo()
{
	if (CurrentComboMontage != nullptr)
	{
		if (UAnimInstance* Anim = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
		{
			Anim->Montage_Stop(0.1f, CurrentComboMontage);
		}
	}
	ComboReset();
}

void AUT1Player::OnMontageEnd(UAnimMontage* Montage, bool bInterrupted)
{
	if (bInterrupted) return;   // 다음 콤보 몽타주에 밀린 경우. 여기서 처리하지 않는다.

	// 정상 종료인데 예약이 남아 있다면 ComboWindowStart 가 몽타주 길이보다 뒤에
	// 잡혀 윈도우가 한 번도 열리지 않은 것이다. 데이터 설정 오류지만 입력을
	// 삼키지는 않고 살려 준다.
	if (bComboQueued)
	{
		UE_LOG(LogUT1, Warning,
			TEXT("[Combo] %d단 ComboWindowStart 가 몽타주 길이보다 깁니다. 선입력을 종료 시점에 처리합니다."),
			ComboIndex);
		AdvanceCombo();
		return;
	}

	ComboReset();
}

bool AUT1Player::HasNextComboStep() const
{
	return CurrentWeaponData != nullptr && CurrentWeaponData->ComboSequence.IsValidIndex(ComboIndex + 1);
}

const FComboStep* AUT1Player::GetCurrentComboStep() const
{
	if (CurrentWeaponData == nullptr)
	{
		return nullptr;
	}

	const TArray<FComboStep>& Combo = CurrentWeaponData->ComboSequence;
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
	if (bIsAttacking || IsDead() || DodgeComponent->IsDodging()) return;

	FVector2D MovementVector = InputValue.Get<FVector2D>();

	if (Controller != nullptr)
	{
		AddMovementInput(GetCameraRelativeDirection(MovementVector));
	}
}

FVector AUT1Player::GetCameraRelativeDirection(const FVector2D& Input) const
{
	const FRotator CamYaw(0.f, SpringArm->GetComponentRotation().Yaw, 0.f);
	const FRotationMatrix CamMatrix(CamYaw);
	return CamMatrix.GetUnitAxis(EAxis::X) * Input.Y + CamMatrix.GetUnitAxis(EAxis::Y) * Input.X;
}

void AUT1Player::Input_Dodge()
{
	// 쿨다운 중이면 공격을 끊지 않는다. 회피도 못 하고 공격만 날아가면 억울하다.
	if (DodgeComponent->CanDodge() == false)
	{
		return;
	}

	FVector Direction = FVector::ZeroVector;
	if (auto* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{
		Direction = GetCameraRelativeDirection(EnhancedInputComponent->GetBoundActionValue(MoveAction).Get<FVector2D>());
	}

	// 공격 캔슬. 판정이 켜진 채 구르지 않도록 콤보를 먼저 정리한다.
	if (bIsAttacking)
	{
		CancelCombo();
	}

	// 방향이 0 이면 컴포넌트가 바라보는 방향으로 대신 구른다.
	DodgeComponent->TryDodge(Direction);
}
