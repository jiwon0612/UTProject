#include "PYW/Editor/PYWEditorLibrary.h"

#include "Animation/BlendSpace.h"
#include "Engine/Blueprint.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"

int32 UPYWEditorLibrary::RebuildBlendSpace(UBlendSpace* BlendSpace)
{
	int32 ValidSamples = 0;
#if WITH_EDITOR
	if (!BlendSpace) return 0;
	BlendSpace->Modify();
	BlendSpace->ResampleData();
	for (const FBlendSample& Sample : BlendSpace->GetBlendSamples())
	{
		if (Sample.bIsValid) ++ValidSamples;
	}
	BlendSpace->MarkPackageDirty();
#endif
	return ValidSamples;
}

#if WITH_EDITOR
namespace
{
	// BlueprintGraph(에디터 모듈)에 의존하지 않도록 핀 분류와 노드 클래스를 이름으로 비교함
	const FName ExecPinCategory(TEXT("exec"));

	TArray<UEdGraph*> GetEventGraphs(UBlueprint* Blueprint)
	{
		TArray<UEdGraph*> Graphs;
		for (UEdGraph* Graph : Blueprint->UbergraphPages)
		{
			if (Graph) Graphs.Add(Graph);
		}
		return Graphs;
	}

	bool IsBrokenInputActionEvent(const UEdGraphNode* Node)
	{
		if (Node->GetClass()->GetName() != TEXT("K2Node_EnhancedInputAction")) return false;
		const FObjectPropertyBase* Property = FindFProperty<FObjectPropertyBase>(Node->GetClass(), TEXT("InputAction"));
		return Property && Property->GetObjectPropertyValue_InContainer(Node) == nullptr;
	}

	// 지정된 IMC가 삭제되어 MappingContext 핀이 비어 있고 연결도 없는 Add Mapping Context 호출임
	bool IsNullMappingContextCall(const UEdGraphNode* Node)
	{
		if (Node->GetClass()->GetName() != TEXT("K2Node_CallFunction")) return false;
		const UEdGraphPin* const* Pin = Node->Pins.FindByPredicate([](const UEdGraphPin* Candidate)
		{
			return Candidate && Candidate->Direction == EGPD_Input && Candidate->PinName == TEXT("MappingContext");
		});
		return Pin && (*Pin)->LinkedTo.IsEmpty() && (*Pin)->DefaultObject == nullptr;
	}

	// 분기·리루트처럼 부작용 없이 흐름만 잇는 노드임. 출력이 모두 지워질 노드로만 가면 함께 지움
	bool IsFlowOnlyNode(const UEdGraphNode* Node)
	{
		const FString ClassName = Node->GetClass()->GetName();
		return ClassName == TEXT("K2Node_IfThenElse") || ClassName == TEXT("K2Node_Knot") || ClassName == TEXT("K2Node_MacroInstance");
	}

	bool HasExecPins(const UEdGraphNode* Node)
	{
		return Node->Pins.ContainsByPredicate([](const UEdGraphPin* Pin) { return Pin && Pin->PinType.PinCategory == ExecPinCategory; });
	}

	// 실행 입력이 모두 제거 대상에서만 들어오면 그 노드도 해당 이벤트 전용으로 봄
	bool IsOnlyReachedFrom(const UEdGraphNode* Node, const TSet<UEdGraphNode*>& Removed)
	{
		bool bHasExecInput = false;
		for (const UEdGraphPin* Pin : Node->Pins)
		{
			if (!Pin || Pin->Direction != EGPD_Input || Pin->PinType.PinCategory != ExecPinCategory) continue;
			for (const UEdGraphPin* Linked : Pin->LinkedTo)
			{
				bHasExecInput = true;
				if (!Linked || !Removed.Contains(Linked->GetOwningNode())) return false;
			}
		}
		return bHasExecInput;
	}

	// 실행 핀이 없는 순수 노드는 모든 출력이 제거 대상으로만 이어질 때 함께 지움
	bool OnlyFeeds(const UEdGraphNode* Node, const TSet<UEdGraphNode*>& Removed)
	{
		bool bHasOutput = false;
		for (const UEdGraphPin* Pin : Node->Pins)
		{
			if (!Pin || Pin->Direction != EGPD_Output) continue;
			for (const UEdGraphPin* Linked : Pin->LinkedTo)
			{
				bHasOutput = true;
				if (!Linked || !Removed.Contains(Linked->GetOwningNode())) return false;
			}
		}
		return bHasOutput;
	}

	/** Seed 노드와, 그 노드에서만 실행되는 하위 체인, 그 노드에만 값을 주거나 흐름을 잇는 상위 노드를 지움 */
	TArray<FString> RemoveDedicatedNodes(UBlueprint* Blueprint, TFunctionRef<bool(const UEdGraphNode*)> IsSeed)
	{
		TArray<FString> RemovedNames;
		for (UEdGraph* Graph : GetEventGraphs(Blueprint))
		{
			TSet<UEdGraphNode*> Removed;
			for (UEdGraphNode* Node : Graph->Nodes)
			{
				if (Node && IsSeed(Node)) Removed.Add(Node);
			}
			if (Removed.IsEmpty()) continue;

			// 고정점에 도달할 때까지 모음. 이벤트 노드는 실행 입력이 없어서 지워지지 않음
			bool bChanged = true;
			while (bChanged)
			{
				bChanged = false;
				for (UEdGraphNode* Node : Graph->Nodes)
				{
					if (!Node || Removed.Contains(Node)) continue;
					const bool bDedicated = HasExecPins(Node)
						? IsOnlyReachedFrom(Node, Removed) || (IsFlowOnlyNode(Node) && OnlyFeeds(Node, Removed))
						: OnlyFeeds(Node, Removed);
					if (bDedicated)
					{
						Removed.Add(Node);
						bChanged = true;
					}
				}
			}

			Blueprint->Modify();
			Graph->Modify();
			for (UEdGraphNode* Node : Removed)
			{
				RemovedNames.Add(FString::Printf(TEXT("%s \"%s\""), *Node->GetName(), *Node->GetNodeTitle(ENodeTitleType::ListView).ToString()));
				Node->BreakAllNodeLinks();
				Graph->RemoveNode(Node);
			}
		}
		if (!RemovedNames.IsEmpty()) Blueprint->MarkPackageDirty();
		return RemovedNames;
	}
}
#endif

TArray<FString> UPYWEditorLibrary::DescribeEventGraphs(UBlueprint* Blueprint)
{
	TArray<FString> Lines;
#if WITH_EDITOR
	if (!Blueprint) return Lines;
	for (UEdGraph* Graph : GetEventGraphs(Blueprint))
	{
		for (UEdGraphNode* Node : Graph->Nodes)
		{
			if (!Node) continue;
			TArray<FString> Targets;
			for (const UEdGraphPin* Pin : Node->Pins)
			{
				if (!Pin || Pin->Direction != EGPD_Output || Pin->PinType.PinCategory != ExecPinCategory) continue;
				for (const UEdGraphPin* Linked : Pin->LinkedTo)
				{
					if (Linked) Targets.Add(FString::Printf(TEXT("%s->%s"), *Pin->PinName.ToString(), *Linked->GetOwningNode()->GetName()));
				}
			}
			Lines.Add(FString::Printf(TEXT("%s | %s | %s | \"%s\"%s | %s"), *Graph->GetName(), *Node->GetName(),
				*Node->GetClass()->GetName(), *Node->GetNodeTitle(ENodeTitleType::ListView).ToString(),
				IsBrokenInputActionEvent(Node) || IsNullMappingContextCall(Node) ? TEXT(" BROKEN") : TEXT(""), *FString::Join(Targets, TEXT(", "))));
		}
	}
#endif
	return Lines;
}

TArray<FString> UPYWEditorLibrary::RemoveBrokenInputActionEvents(UBlueprint* Blueprint)
{
#if WITH_EDITOR
	if (Blueprint) return RemoveDedicatedNodes(Blueprint, IsBrokenInputActionEvent);
#endif
	return {};
}

TArray<FString> UPYWEditorLibrary::RemoveNullMappingContextCalls(UBlueprint* Blueprint)
{
#if WITH_EDITOR
	if (Blueprint) return RemoveDedicatedNodes(Blueprint, IsNullMappingContextCall);
#endif
	return {};
}
