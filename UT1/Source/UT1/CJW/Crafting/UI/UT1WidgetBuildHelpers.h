// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Layout/Margin.h"

class UWidgetTree;
class UTextBlock;
class UButton;

/**
 * C++ 로 기본 레이아웃을 조립할 때 쓰는 작은 도우미.
 *
 * 작업대 위젯들은 WBP 레이아웃이 비어 있으면(=C++ 클래스를 그대로 쓰거나,
 * WBP 를 만들었지만 아직 아무것도 배치하지 않았으면) 스스로 기본 화면을 만든다.
 * 위젯마다 같은 "텍스트 만들기 / 버튼 만들기" 코드가 반복되지 않도록 모았다.
 */
namespace UT1WidgetBuild
{
	UTextBlock* MakeText(UWidgetTree* Tree, FName Name, const FText& Text, int32 FontSize = 14);

	// 가운데에 글자가 들어간 버튼.
	UButton* MakeButton(UWidgetTree* Tree, FName Name, const FText& Label, int32 FontSize = 14);
}
