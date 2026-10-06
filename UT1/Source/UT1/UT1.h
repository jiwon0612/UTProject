// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/** Main log category used across the project */
DECLARE_LOG_CATEGORY_EXTERN(LogUT1, Log, All);

/**
 * 무기 타격 판정용 커스텀 트레이스 채널.
 *
 * DefaultEngine.ini 의 [/Script/Engine.CollisionProfile] 에 "Weapon" 이라는
 * 이름으로 선언돼 있다. 엔진은 채널을 인덱스로만 알기 때문에, ini 의
 * ECC_GameTraceChannel1 과 이 정의가 어긋나면 엉뚱한 채널로 트레이스하게 된다.
 * 채널을 추가하거나 순서를 바꿀 때 둘을 같이 고칠 것.
 */
#define UT1_TRACE_CHANNEL_WEAPON ECC_GameTraceChannel1
