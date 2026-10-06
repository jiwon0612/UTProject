// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CJW/Entities/UT1Entity.h"
#include "UT1Player.generated.h"

struct FInputActionValue;
struct FComboStep;
class UUT1WeaponData;
class AUT1Weapon;
class UUT1RunInventoryComponent;
class UUT1MaterialData;
class UUT1DodgeComponent;

// 지금 E 를 누르면 실행될 대상이 바뀌었다. 대상이 없으면 nullptr.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUT1FocusedInteractableChanged, AActor*, NewTarget);

/**
 * 
 */
UCLASS()
class UT1_API AUT1Player : public AUT1Entity
{
	GENERATED_BODY()
public:
	AUT1Player();
public:
	virtual void NotifyControllerChanged() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void Tick(float DeltaTime) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 상호작용 대상(IUT1Interactable) 범위 판정. 캡슐이 대상의 콜리전과 겹치면 불린다.
	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;
	virtual void NotifyActorEndOverlap(AActor* OtherActor) override;

public:
	void ComboAttack();

	void PlayComboStep();
	void ComboReset();

	int32 ComboIndex = 0;
	bool bIsAttacking = false;
	bool bComboQueued = false;

	// 현재 장착 무기. 콤보 시퀀스와 장착 메시가 모두 여기서 나온다.
	// UPROPERTY 가 없으면 리플렉션에 잡히지 않아 GC 가 수거해 갈 수 있다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<UUT1WeaponData> CurrentWeaponData;

	// 무기를 장착한다. nullptr 을 넘기면 해제한다.
	// 진행 중이던 콤보는 끊고 초기화한다.
	// 게임 흐름에서는 직접 부르지 말고 RunInventory->EquipWeapon 을 쓴다.
	// 그래야 보관함의 "장착 중" 표시와 손에 든 무기가 어긋나지 않는다.
	void EquipWeaponData(UUT1WeaponData* NewWeaponData);

	UUT1RunInventoryComponent* GetRunInventory() const { return RunInventory; }

	// --- 공격 판정 ---
	// AnimNotifyState_UT1AttackTrace 가 몽타주 구간에 맞춰 불러 준다.
	// 장착된 무기 전부에 전달하므로 쌍수 무기도 그대로 동작한다.
	void StartWeaponTrace();
	void TickWeaponTrace();
	void StopWeaponTrace();

	UFUNCTION()
	void OnMontageEnd(UAnimMontage* Montage, bool bInterrupted);

	// 재생 중인 콤보 몽타주를 멈추고 콤보 상태를 처음으로 되돌린다.
	// 무기 교체, 사망, 회피 캔슬처럼 공격을 외부에서 끊어야 할 때 쓴다.
	void CancelCombo();
	
	void RotateToCursor();

protected:
	// 죽는 순간 휘두르던 판정과 콤보 상태를 정리한다.
	// 안 하면 쓰러진 채로 칼이 계속 적을 벤다.
	virtual void HandleDeath(AActor* Killer) override;

public:

private:
	// 콤보를 다음 단계로 넘긴다. 재생 중인 몽타주를 덮어쓰므로,
	// 호출하는 쪽이 콤보 윈도우 검사를 끝낸 뒤에 부른다.
	void AdvanceCombo();

	// 현재 단계의 콤보 정보. 인덱스를 벗어나면 nullptr.
	const FComboStep* GetCurrentComboStep() const;

	// 다음 단계가 실제로 존재하는지. 마지막 단계에서 선입력을 받지 않기 위해 쓴다.
	bool HasNextComboStep() const;

	// BaseDamage x 현재 단계의 DamageMultiplier.
	float GetCurrentAttackDamage() const;

	// 재생 중인 콤보 몽타주의 경과 시간(초). 재생 중이 아니면 false.
	bool GetComboMontagePosition(float& OutPosition) const;

	// 재생 위치를 물어보려면 대상 몽타주를 알아야 해서 따로 들고 있는다.
	UPROPERTY()
	TObjectPtr<UAnimMontage> CurrentComboMontage;

	// EquipMeshes 항목마다 스폰해서 소켓에 붙인 무기 액터.
	// 부착된 액터는 부모가 파괴돼도 따라 사라지지 않으므로 직접 정리한다.
	UPROPERTY()
	TArray<TObjectPtr<AUT1Weapon>> EquippedWeapons;

	void ClearEquippedWeapons();

	// 인벤토리가 장착 변경을 알리면 실제 무기 액터를 교체한다.
	// 델리게이트에 바인딩하려면 UFUNCTION 이어야 한다.
	UFUNCTION()
	void HandleEquippedWeaponChanged(UUT1WeaponData* NewWeapon);

	UFUNCTION()
	void HandleInventoryChanged();

	// 공격 범위 강화 = 무기 액터 크기. 장착 직후와 강화 직후에 다시 맞춘다.
	void ApplyWeaponRangeScale();

protected:
	// 이번 런의 재료, 설계도, 보유 무기와 강화 상태.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Crafting")
	TObjectPtr<UUT1RunInventoryComponent> RunInventory;

	// 회피 거리/시간/무적/쿨다운/몽타주는 BP_Player 의 이 컴포넌트에서 조절한다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<UUT1DodgeComponent> DodgeComponent;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<class USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<class UCameraComponent> Camera;

public:
	void Input_Move(const FInputActionValue& InputValue);
	void Input_Interact();
	void Input_Dodge();

	// 공격 버튼: 누른 순간(Started)과 누르고 있는 동안(Triggered, 매 프레임).
	void Input_AttackPressed();
	void Input_AttackHeld();

protected:
	// 꾹 누르고 있으면 콤보 마지막 타 뒤에 1타부터 다시 시작한다.
	// 끄면 꾹 누르기는 한 콤보를 끝까지 잇는 데까지만 동작한다.
	UPROPERTY(EditAnywhere, Category = "Combat|Hold Attack")
	bool bRepeatComboWhileHeld = true;

	// 이 시간 이상 누르고 있어야 "꾹 누르기"로 본다. Triggered 는 누른 첫 프레임에도
	// 발생하므로, 기준이 없으면 한 번 클릭만 해도 다음 타가 예약돼 버린다.
	UPROPERTY(EditAnywhere, Category = "Combat|Hold Attack", meta = (ClampMin = "0.0"))
	float HoldAttackThreshold = 0.2f;

private:
	float AttackPressedTime = 0.0f;

private:
	// 2D 입력을 카메라 기준 월드 방향으로 바꾼다. 이동과 회피가 같은 기준을 써야
	// 누른 방향과 구르는 방향이 어긋나지 않는다.
	FVector GetCameraRelativeDirection(const FVector2D& Input) const;

public:
	// E 를 누르면 실행될 대상. 안내 UI 와 실제 입력이 같은 함수로 대상을 고르므로
	// "안내는 A 인데 B 가 열리는" 어긋남이 생기지 않는다.
	AActor* GetFocusedInteractable() const;

	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FUT1FocusedInteractableChanged OnFocusedInteractableChanged;

private:
	// 범위 안의 상호작용 대상들. 대상이 사라져도 댕글링 포인터가 되지 않게 약한 참조로 둔다.
	// 여러 개가 겹치면 가장 나중에 들어온 대상을 쓴다.
	TArray<TWeakObjectPtr<AActor>> NearbyInteractables;

	// 대상이 바뀌었을 때만 OnFocusedInteractableChanged 를 알린다.
	// 겹침 시작/끝, 사망처럼 대상이 바뀔 수 있는 곳에서 부른다.
	void RefreshFocusedInteractable();

	// 마지막으로 알린 대상. 같은 대상을 반복해서 알리지 않으려고 기억한다.
	TWeakObjectPtr<AActor> LastFocusedInteractable;

protected:
	UPROPERTY(EditAnywhere, Category = Input)
	TObjectPtr<class UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditAnywhere, Category = Input)
	TObjectPtr<class UInputAction> MoveAction;

	UPROPERTY(EditAnywhere, Category = Input)
	TObjectPtr<class UInputAction> AttackAction;

	// IA_Interact (E). 비어 있으면 상호작용 입력을 바인딩하지 않는다.
	UPROPERTY(EditAnywhere, Category = Input)
	TObjectPtr<class UInputAction> InteractAction;

	// IA_Dodge (Space). 비어 있으면 회피 입력을 바인딩하지 않는다.
	UPROPERTY(EditAnywhere, Category = Input)
	TObjectPtr<class UInputAction> DodgeAction;

protected:
	// 시작 무기. 보관함에 보유 상태로 등록하고 바로 장착한다.
	UPROPERTY(EditAnywhere, Category = Test)
	TObjectPtr<class UUT1WeaponData> TestWeaponData;

	// --- 작업대 UI 가 생기기 전까지 쓰는 테스트 명령 ---
	// Exec 함수는 빙의된 Pawn 에 있으면 콘솔(~)에서 바로 호출된다.
	// UI 가 생긴 뒤에도 밸런스 확인용으로 남겨 둘 만하다.

	// UT1_GiveTestLoot 에서 지급할 재료와 설계도.
	UPROPERTY(EditAnywhere, Category = Test)
	TArray<TObjectPtr<UUT1MaterialData>> TestMaterials;

	UPROPERTY(EditAnywhere, Category = Test)
	TArray<TObjectPtr<UUT1WeaponData>> TestBlueprints;

public:
	// TestMaterials 각각 Count 개, TestBlueprints 전부 해금.
	UFUNCTION(Exec)
	void UT1_GiveTestLoot(int32 Count = 10);

	// 해금한 설계도 목록의 Index 번째 무기를 제작.
	UFUNCTION(Exec)
	void UT1_Craft(int32 BlueprintIndex);

	// 보유 무기 목록의 Index 번째 무기의 스탯을 강화. 예) UT1_Enhance 0 AttackPower
	UFUNCTION(Exec)
	void UT1_Enhance(int32 OwnedIndex, const FString& StatName);

	UFUNCTION(Exec)
	void UT1_Equip(int32 OwnedIndex);

	UFUNCTION(Exec)
	void UT1_Dismantle(int32 OwnedIndex);

	UFUNCTION(Exec)
	void UT1_PrintInventory();
};
