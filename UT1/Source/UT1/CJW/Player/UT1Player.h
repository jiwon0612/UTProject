// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CJW/Entities/UT1Entity.h"
#include "UT1Player.generated.h"

struct FInputActionValue;
struct FComboStep;

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

public:
	void ComboAttack();

	void PlayComboStep();
	void ComboReset();

	int32 ComboIndex = 0;
	bool bIsAttacking = false;
	bool bComboQueued = false;

	UFUNCTION()
	void OnMontageEnd(UAnimMontage* Montage, bool bInterrupted);
	
	void RotateToCursor();

private:
	// 콤보를 다음 단계로 넘긴다. 재생 중인 몽타주를 덮어쓰므로,
	// 호출하는 쪽이 콤보 윈도우 검사를 끝낸 뒤에 부른다.
	void AdvanceCombo();

	// 현재 단계의 콤보 정보. 인덱스를 벗어나면 nullptr.
	const FComboStep* GetCurrentComboStep() const;

	// 다음 단계가 실제로 존재하는지. 마지막 단계에서 선입력을 받지 않기 위해 쓴다.
	bool HasNextComboStep() const;

	// 재생 중인 콤보 몽타주의 경과 시간(초). 재생 중이 아니면 false.
	bool GetComboMontagePosition(float& OutPosition) const;

	// 재생 위치를 물어보려면 대상 몽타주를 알아야 해서 따로 들고 있는다.
	UPROPERTY()
	TObjectPtr<UAnimMontage> CurrentComboMontage;

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
