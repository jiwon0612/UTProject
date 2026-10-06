// Fill out your copyright notice in the Description page of Project Settings.


#include "CJW/Player/UT1Player.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimInstance.h"
#include "UT1/CJW/Weapons/UT1WeaponData.h"
#include "CJW/Weapons/UT1Weapon.h"
#include "CJW/Crafting/UT1RunInventoryComponent.h"
#include "CJW/Crafting/UT1MaterialData.h"
#include "CJW/Interaction/UT1Interactable.h"
#include "CJW/Combat/UT1DodgeComponent.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
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
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> OutlineMaterial(
		TEXT("/Game/LSW/Materials/M_UT1_OccludedCharacterOutline.M_UT1_OccludedCharacterOutline"));
	if (OutlineMaterial.Succeeded())
	{
		OccludedCharacterOutlineMaterial = OutlineMaterial.Object;
		Camera->PostProcessSettings.AddBlendable(OccludedCharacterOutlineMaterial, 1.0f);
	}

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 500.f, 0.f);
	bUseControllerRotationYaw = false;

	RunInventory = CreateDefaultSubobject<UUT1RunInventoryComponent>(TEXT("RunInventory"));
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
		EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Started, this, &AUT1Player::Input_AttackPressed);
		EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Triggered, this, &AUT1Player::Input_AttackHeld);

		if (InteractAction != nullptr)
		{
			EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &AUT1Player::Input_Interact);
		}

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

	// Apply this after Blueprint component defaults are loaded so they cannot
	// silently replace the constructor's post-process blendable.
	if (!OccludedCharacterOutlineMaterial)
	{
		OccludedCharacterOutlineMaterial = LoadObject<UMaterialInterface>(nullptr,
			TEXT("/Game/LSW/Materials/M_UT1_OccludedCharacterOutline.M_UT1_OccludedCharacterOutline"));
	}
	if (Camera && OccludedCharacterOutlineMaterial)
	{
		Camera->PostProcessSettings.AddBlendable(OccludedCharacterOutlineMaterial, 1.0f);
	}
	else
	{
		UE_LOG(LogUT1, Warning, TEXT("[Outline] Camera or outline material is missing on %s."), *GetName());
	}

	// 장착 변경은 인벤토리가 결정하고, 무기 액터 교체는 플레이어가 한다.
	// 시작 무기 장착보다 먼저 바인딩해야 첫 장착 알림을 놓치지 않는다.
	RunInventory->OnEquippedWeaponChanged.AddDynamic(this, &AUT1Player::HandleEquippedWeaponChanged);
	RunInventory->OnInventoryChanged.AddDynamic(this, &AUT1Player::HandleInventoryChanged);

	// TestWeaponData 는 에디터(BP_Player)에서 지정하는 시작 무기다.
	// 설계도도 같이 주므로, 다른 무기로 바꾼 뒤 분해해도 다시 만들 수 있다.
	if (TestWeaponData != nullptr)
	{
		RunInventory->UnlockBlueprint(TestWeaponData);
		RunInventory->AddOwnedWeapon(TestWeaponData);
		RunInventory->EquipWeapon(TestWeaponData);
	}
}

void AUT1Player::HandleEquippedWeaponChanged(UUT1WeaponData* NewWeapon)
{
	EquipWeaponData(NewWeapon);
}

void AUT1Player::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);

	if (OtherActor != nullptr && OtherActor->Implements<UUT1Interactable>())
	{
		NearbyInteractables.AddUnique(OtherActor);
		RefreshFocusedInteractable();
	}
}

void AUT1Player::NotifyActorEndOverlap(AActor* OtherActor)
{
	Super::NotifyActorEndOverlap(OtherActor);

	if (NearbyInteractables.Remove(OtherActor) > 0)
	{
		RefreshFocusedInteractable();
	}
}

AActor* AUT1Player::GetFocusedInteractable() const
{
	if (IsDead())
	{
		return nullptr;
	}

	// 뒤에서부터(가장 최근에 들어온 대상부터) 살아 있는 것을 찾는다.
	// 파괴된 항목은 겹침 끝 알림이 올 때 정리되므로 여기서는 건너뛰기만 한다.
	for (int32 i = NearbyInteractables.Num() - 1; i >= 0; --i)
	{
		if (AActor* Target = NearbyInteractables[i].Get())
		{
			return Target;
		}
	}
	return nullptr;
}

void AUT1Player::RefreshFocusedInteractable()
{
	AActor* NewTarget = GetFocusedInteractable();
	if (LastFocusedInteractable.Get() == NewTarget)
	{
		return;
	}

	LastFocusedInteractable = NewTarget;
	OnFocusedInteractableChanged.Broadcast(NewTarget);
}

void AUT1Player::Input_Interact()
{
	// 공격 중에 열리면 몽타주가 멈춘 채 UI 가 떠서 상태가 꼬인다.
	if (bIsAttacking || DodgeComponent->IsDodging())
	{
		return;
	}

	// 사망 검사는 GetFocusedInteractable 안에 있다.
	if (AActor* Target = GetFocusedInteractable())
	{
		IUT1Interactable::Execute_Interact(Target, this);
	}
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

	ApplyWeaponRangeScale();
	RefreshWeaponAura();
}

void AUT1Player::RefreshWeaponAura()
{
	// 쌍검처럼 손에 든 무기가 여럿이면 전부 같은 상태로 맞춘다.
	const bool bShowAura = RunInventory->ShouldShowAura(CurrentWeaponData);
	for (TObjectPtr<AUT1Weapon>& Weapon : EquippedWeapons)
	{
		if (IsValid(Weapon))
		{
			Weapon->SetAuraActive(bShowAura);
		}
	}
}

void AUT1Player::ApplyWeaponRangeScale()
{
	const float Scale = RunInventory->GetCombatStats(CurrentWeaponData).RangeScale;

	// 루트(GripRoot)의 스케일을 바꾸므로 손잡이를 기준으로 커진다. 손에서 빠져 보이지 않는다.
	for (TObjectPtr<AUT1Weapon>& Weapon : EquippedWeapons)
	{
		if (IsValid(Weapon))
		{
			Weapon->SetActorRelativeScale3D(FVector(Scale));
		}
	}
}

void AUT1Player::HandleInventoryChanged()
{
	// 장착 중인 무기를 작업대에서 강화하면 무기 크기와 오라가 바로 바뀌어야 한다.
	// 데미지/공속/치명타는 공격할 때마다 새로 읽으므로 여기서 할 일이 없다.
	ApplyWeaponRangeScale();
	RefreshWeaponAura();
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

	// 죽으면 상호작용할 수 없으므로 안내를 내린다. bIsDead 는 베이스에서 켜지므로 그 뒤에 부른다.
	RefreshFocusedInteractable();
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
	const FUT1WeaponCombatStats Stats = RunInventory->GetCombatStats(CurrentWeaponData);

	FUT1AttackInfo AttackInfo;
	AttackInfo.Damage = GetCurrentAttackDamage();
	AttackInfo.CritChance = Stats.CritChance;
	AttackInfo.CritDamageMultiplier = Stats.CritDamageMultiplier;

	for (TObjectPtr<AUT1Weapon>& Weapon : EquippedWeapons)
	{
		if (IsValid(Weapon))
		{
			Weapon->TickAttackTrace(AttackInfo);
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
	const float EnhanceMultiplier = RunInventory->GetCombatStats(CurrentWeaponData).DamageMultiplier;
	return CurrentWeaponData->BaseDamage * Multiplier * EnhanceMultiplier;
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

void AUT1Player::Input_AttackPressed()
{
	AttackPressedTime = GetWorld()->GetTimeSeconds();
	ComboAttack();
}

void AUT1Player::Input_AttackHeld()
{
	// 짧은 클릭은 Started 한 번으로 끝낸다. 기준 시간을 넘겨 누르고 있을 때만 이어 준다.
	if (GetWorld()->GetTimeSeconds() - AttackPressedTime < HoldAttackThreshold)
	{
		return;
	}

	// 이미 다음 타가 예약돼 있으면 Tick 이 윈도우에서 발동시킨다. 다시 넣을 필요 없다.
	if (bComboQueued)
	{
		return;
	}

	if (bIsAttacking == false && bRepeatComboWhileHeld == false)
	{
		return;
	}

	// 연타와 똑같은 경로를 탄다. 윈도우 전이면 예약, 윈도우 안이면 즉시 진행,
	// 윈도우가 지났거나 마지막 단계면 그냥 버려진다. 꾹 누르기용 규칙을 따로 두지 않아
	// 연타와 꾹 누르기의 결과가 항상 같다.
	ComboAttack();
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

	// 공격속도 강화는 재생 속도로 반영한다. 콤보 윈도우와 판정 노티파이는
	// 몽타주 "안의 시간"(Montage_GetPosition) 기준이라 PlayRate 를 올려도
	// 따로 맞출 필요 없이 실제 시간 기준으로 함께 짧아진다.
	const float PlayRate = RunInventory->GetCombatStats(CurrentWeaponData).AttackSpeed;
	Anim->Montage_Play(Montage, PlayRate);

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

// ---------------------------------------------------------------- 테스트 명령 (작업대 UI 전까지)

namespace
{
	FString WeaponLabel(const UUT1WeaponData* Weapon)
	{
		return Weapon != nullptr ? Weapon->GetDisplayText().ToString() : TEXT("(없음)");
	}

	FString MaterialLabel(const UUT1MaterialData* Material)
	{
		return Material != nullptr ? Material->GetDisplayText().ToString() : TEXT("(없음)");
	}

	// 로그와 화면에 같이 찍는다. PIE 중에는 화면이, 나중에 확인할 때는 로그가 편하다.
	void PrintLine(const FString& Line, const FColor& Color = FColor::Cyan)
	{
		UE_LOG(LogUT1, Log, TEXT("%s"), *Line);
		if (GEngine != nullptr)
		{
			GEngine->AddOnScreenDebugMessage(-1, 8.0f, Color, Line);
		}
	}

	void PrintResult(const TCHAR* Action, const UUT1WeaponData* Weapon, EUT1WorkbenchResult Result)
	{
		const bool bSuccess = (Result == EUT1WorkbenchResult::Success);
		PrintLine(FString::Printf(TEXT("[Workbench] %s %s -> %s"),
			Action, *WeaponLabel(Weapon), *UEnum::GetValueAsString(Result)),
			bSuccess ? FColor::Green : FColor::Red);
	}
}

void AUT1Player::UT1_GiveTestLoot(int32 Count)
{
	for (UUT1MaterialData* Material : TestMaterials)
	{
		RunInventory->AddMaterial(Material, Count);
	}
	for (UUT1WeaponData* Weapon : TestBlueprints)
	{
		RunInventory->UnlockBlueprint(Weapon);
	}
	UT1_PrintInventory();
}

void AUT1Player::UT1_Craft(int32 BlueprintIndex)
{
	const TArray<TObjectPtr<UUT1WeaponData>>& Blueprints = RunInventory->GetUnlockedBlueprints();
	UUT1WeaponData* Weapon = Blueprints.IsValidIndex(BlueprintIndex) ? Blueprints[BlueprintIndex].Get() : nullptr;

	PrintResult(TEXT("제작"), Weapon, RunInventory->Craft(Weapon));
	UT1_PrintInventory();
}

void AUT1Player::UT1_Enhance(int32 OwnedIndex, const FString& StatName)
{
	const int64 StatValue = StaticEnum<EUT1WeaponStat>()->GetValueByNameString(StatName);
	if (StatValue == INDEX_NONE)
	{
		PrintLine(FString::Printf(TEXT("[Workbench] 알 수 없는 스탯 '%s' (AttackPower / AttackSpeed / AttackRange / CritChance)"), *StatName), FColor::Red);
		return;
	}
	const EUT1WeaponStat Stat = static_cast<EUT1WeaponStat>(StatValue);

	const TArray<FUT1OwnedWeapon>& Owned = RunInventory->GetOwnedWeapons();
	UUT1WeaponData* Weapon = Owned.IsValidIndex(OwnedIndex) ? Owned[OwnedIndex].WeaponData.Get() : nullptr;

	PrintResult(*FString::Printf(TEXT("강화(%s)"), *StatName), Weapon, RunInventory->Enhance(Weapon, Stat));
	UT1_PrintInventory();
}

void AUT1Player::UT1_Equip(int32 OwnedIndex)
{
	const TArray<FUT1OwnedWeapon>& Owned = RunInventory->GetOwnedWeapons();
	UUT1WeaponData* Weapon = Owned.IsValidIndex(OwnedIndex) ? Owned[OwnedIndex].WeaponData.Get() : nullptr;

	PrintResult(TEXT("장착"), Weapon, RunInventory->EquipWeapon(Weapon));
}

void AUT1Player::UT1_Dismantle(int32 OwnedIndex)
{
	const TArray<FUT1OwnedWeapon>& Owned = RunInventory->GetOwnedWeapons();
	UUT1WeaponData* Weapon = Owned.IsValidIndex(OwnedIndex) ? Owned[OwnedIndex].WeaponData.Get() : nullptr;

	PrintResult(TEXT("분해"), Weapon, RunInventory->Dismantle(Weapon));
	UT1_PrintInventory();
}

void AUT1Player::UT1_PrintInventory()
{
	// 화면 메시지는 새 줄이 위에 쌓이므로, 로그는 순서대로 찍고 화면에는 뒤집어 찍는다.
	TArray<FString> Lines;

	Lines.Add(TEXT("===== 런 인벤토리 ====="));

	FString MaterialLine = TEXT("재료:");
	for (const TPair<TObjectPtr<UUT1MaterialData>, int32>& Pair : RunInventory->GetMaterials())
	{
		MaterialLine += FString::Printf(TEXT(" %s x%d"), *MaterialLabel(Pair.Key), Pair.Value);
	}
	Lines.Add(MaterialLine);

	const TArray<TObjectPtr<UUT1WeaponData>>& Blueprints = RunInventory->GetUnlockedBlueprints();
	for (int32 i = 0; i < Blueprints.Num(); ++i)
	{
		Lines.Add(FString::Printf(TEXT("설계도[%d] %s  (%s)"),
			i, *WeaponLabel(Blueprints[i]), *UEnum::GetValueAsString(RunInventory->CanCraft(Blueprints[i]))));
	}

	const TArray<FUT1OwnedWeapon>& Owned = RunInventory->GetOwnedWeapons();
	for (int32 i = 0; i < Owned.Num(); ++i)
	{
		const FUT1OwnedWeapon& Weapon = Owned[i];
		const bool bEquipped = (Weapon.WeaponData == RunInventory->GetEquippedWeapon());
		Lines.Add(FString::Printf(TEXT("무기[%d] %s%s  공격력 %d / 공속 %d / 범위 %d / 치명 %d"),
			i, *WeaponLabel(Weapon.WeaponData), bEquipped ? TEXT(" [장착]") : TEXT(""),
			Weapon.GetStatLevel(EUT1WeaponStat::AttackPower),
			Weapon.GetStatLevel(EUT1WeaponStat::AttackSpeed),
			Weapon.GetStatLevel(EUT1WeaponStat::AttackRange),
			Weapon.GetStatLevel(EUT1WeaponStat::CritChance)));
	}

	for (const FString& Line : Lines)
	{
		UE_LOG(LogUT1, Log, TEXT("%s"), *Line);
	}
	if (GEngine != nullptr)
	{
		for (int32 i = Lines.Num() - 1; i >= 0; --i)
		{
			GEngine->AddOnScreenDebugMessage(-1, 8.0f, FColor::White, Lines[i]);
		}
	}
}
