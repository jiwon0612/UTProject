// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "UT1MaterialData.generated.h"

class UTexture2D;

/**
 * 재료 한 종류의 정의. 재료 하나 = 에셋 하나 (예: DA_Mat_Scrap).
 *
 * 에셋 자체를 재료의 ID 로 쓴다. 문자열 키와 달리 오타가 날 수 없고,
 * 비용 목록에서 드롭다운으로 고를 수 있으며, 재료를 추가할 때
 * C++ 를 다시 컴파일할 필요가 없다 (enum 방식과의 차이).
 */
UCLASS(BlueprintType)
class UT1_API UUT1MaterialData : public UDataAsset
{
	GENERATED_BODY()

public:
	// DisplayName 이 비어 있으면 에셋 이름을 쓴다. 데이터를 덜 채워도 UI 가 빈칸이 되지 않게.
	UFUNCTION(BlueprintPure, Category = "Info")
	FText GetDisplayText() const
	{
		return DisplayName.IsEmpty() ? FText::FromString(GetName()) : DisplayName;
	}

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Info")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Info", meta = (MultiLine = "true"))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Info")
	TObjectPtr<UTexture2D> Icon;
};
