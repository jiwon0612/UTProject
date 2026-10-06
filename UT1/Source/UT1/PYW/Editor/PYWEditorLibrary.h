#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PYWEditorLibrary.generated.h"

/** PYW 에디터 스크립트(Python)에서 쓰는 에셋 보조 함수임. Python에 노출되지 않은 엔진 기능을 감쌈 */
UCLASS()
class UT1_API UPYWEditorLibrary : public UBlueprintFunctionLibrary
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

	/** 이벤트 그래프의 노드와 실행 연결을 문자열로 돌려줌. 수정 전 확인용임 */
	UFUNCTION(BlueprintCallable, Category = "PYW|Blueprint", meta = (DevelopmentOnly))
	static TArray<FString> DescribeEventGraphs(class UBlueprint* Blueprint);

	/**
	 * 삭제된 Input Action을 가리키는 Enhanced Input 이벤트 노드와, 그 노드에서만 실행되는
	 * 하위 실행 체인을 지움. 지운 노드 이름을 돌려줌. 다른 이벤트와 공유하는 노드는 남김.
	 */
	UFUNCTION(BlueprintCallable, Category = "PYW|Blueprint", meta = (DevelopmentOnly))
	static TArray<FString> RemoveBrokenInputActionEvents(class UBlueprint* Blueprint);

	/**
	 * Mapping Context 핀이 비어 있는 Add Mapping Context 호출과, 그 호출에만 쓰이는 상위 노드
	 * (서브시스템 조회, Is Valid 분기 등)를 지움. 이벤트 노드는 남김. 지운 노드 이름을 돌려줌.
	 */
	UFUNCTION(BlueprintCallable, Category = "PYW|Blueprint", meta = (DevelopmentOnly))
	static TArray<FString> RemoveNullMappingContextCalls(class UBlueprint* Blueprint);

	/** Niagara 시스템에 들어 있는 이미터 핸들 이름을 돌려줌. 런타임에 특정 이미터를 끌 때 이름을 확인하는 용도임 */
	UFUNCTION(BlueprintCallable, Category = "PYW|VFX", meta = (DevelopmentOnly))
	static TArray<FString> GetNiagaraEmitterNames(class UNiagaraSystem* System);};
