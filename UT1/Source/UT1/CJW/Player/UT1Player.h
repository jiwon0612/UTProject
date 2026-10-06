// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CJW/Entities/UT1Entity.h"
#include "UT1Player.generated.h"

struct FInputActionValue;
struct FComboStep;
class UUT1WeaponData;
class AUT1Weapon;

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
	void EquipWeaponData(UUT1WeaponData* NewWeaponData);

	// --- 공격 판정 ---
	// AnimNotifyState_UT1AttackTrace 가 몽타주 구간에 맞춰 불러 준다.
	// 장착된 무기 전부에 전달하므로 쌍수 무기도 그대로 동작한다.
	void StartWeaponTrace();
	void TickWeaponTrace();
	void StopWeaponTrace();

	UFUNCTION()
	void OnMontageEnd(UAnimMontage* Montage, bool bInterrupted);
	
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

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<class USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<class UCameraComponent> Camera;

public:
	void Input_Move(const FInputActionValue& InputValue);

protected:
	UPROPERTY(EditAnywhere, Category = Input)
	TObjectPtr<class UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditAnywhere, Category = Input)
	TObjectPtr<class UInputAction> MoveAction;

	UPROPERTY(EditAnywhere, Category = Input)
	TObjectPtr<class UInputAction> AttackAction;

protected:
	UPROPERTY(EditAnywhere, Category = Test)
	TObjectPtr<class UUT1WeaponData> TestWeaponData;
};
