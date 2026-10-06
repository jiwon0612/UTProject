#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "EnemyAnimationLibrary.generated.h"

/** PYW 에디터 스크립트(Python)에서 쓰는 애니메이션 에셋 보조 함수임 */
UCLASS()
class UT1_API UEnemyAnimationLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * BlendSpace의 런타임 삼각분할을 다시 만들고 유효한 샘플 수를 돌려줌.
	 * Python으로 SampleData만 바꾸면 ResampleData가 불리지 않아 평가할 삼각형이 없고,
	 * 재생 시 레퍼런스 포즈(T포즈)가 나옴. 에디터 빌드에서만 동작함.
	 */
	UFUNCTION(BlueprintCallable, Category = "PYW|Animation", meta = (DevelopmentOnly))
	static int32 RebuildBlendSpace(class UBlendSpace* BlendSpace);
};
