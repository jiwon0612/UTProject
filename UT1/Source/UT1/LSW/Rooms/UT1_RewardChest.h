#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CJW/Interaction/UT1Interactable.h"
#include "UT1_RewardChest.generated.h"

class AUT1Player;
class AUT1LootPickup;
class USphereComponent;
class UStaticMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UUT1MaterialData;
class UUT1WeaponData;

struct FUT1RewardPickupFlight
{
	TWeakObjectPtr<AUT1LootPickup> Pickup;
	FVector StartLocation = FVector::ZeroVector;
	FVector EndLocation = FVector::ZeroVector;
	float Elapsed = 0.0f;
	float Duration = 0.8f;
	float ArcHeight = 180.0f;
};

USTRUCT(BlueprintType)
struct FUT1RewardChestEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward")
	TObjectPtr<UUT1MaterialData> Material = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward")
	TObjectPtr<UUT1WeaponData> Blueprint = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward", meta = (ClampMin = "1"))
	int32 Count = 1;
};

/** A one-use chest that randomly selects and tosses several configured rewards. */
UCLASS()
class UT1_API AUT1_RewardChest : public AActor, public IUT1Interactable
{
	GENERATED_BODY()

public:
	AUT1_RewardChest();

	virtual void Tick(float DeltaSeconds) override;
	virtual void Interact_Implementation(AUT1Player* Interactor) override;

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward")
	TArray<FUT1RewardChestEntry> Rewards;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward")
	TSubclassOf<AUT1LootPickup> PickupClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward", meta = (ClampMin = "1"))
	int32 MinRewardCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward", meta = (ClampMin = "1"))
	int32 MaxRewardCount = 3;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Chest")
	TObjectPtr<USphereComponent> InteractRange;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Chest")
	TObjectPtr<UStaticMeshComponent> BodyMesh;

private:
	void StartChestFade();
	void UpdateChestFade(float DeltaSeconds);

	bool bOpened = false;
	bool bFadingOut = false;
	float FadeElapsed = 0.0f;
	float FadeDuration = 1.0f;
	FName FadeOpacityParameter = TEXT("ChestOpacity");

	UPROPERTY()
	TArray<TObjectPtr<UMaterialInstanceDynamic>> FadeMaterials;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> ChestFadeMaterial;

	TArray<FUT1RewardPickupFlight> TossingPickups;
};
