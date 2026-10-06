#include "PYW/Entities/ArtilleryEnemyCharacter.h"

#include "Animation/AnimSequence.h"
#include "Components/CapsuleComponent.h"
#include "UObject/ConstructorHelpers.h"

AArtilleryEnemyCharacter::AArtilleryEnemyCharacter()
{
	GetCapsuleComponent()->SetRelativeScale3D(FVector(0.95f));
	CombatType = EEnemyCombatType::Ranged;
	CombatMovement = EEnemyCombatMovement::Kite;
	MaxHealth = 35.0f;
	AttackRange = 1300.0f;
	RetreatDistance = 450.0f;
	DetectionRange = 1800.0f;
	WalkSpeed = 150.0f;
	ChaseSpeed = 280.0f;
	StrafeSpeed = 200.0f;
	ReactionTime = 0.7f;
	PoiseThreshold = 15.0f;
	StaggerDuration = 0.7f;
	// 보라색 포탄과 같은 색으로 터짐
	AreaFlashColor = FLinearColor(1.0f, 0.2f, 1.6f);

	// 포탄은 투사체 대신 '표시된 원에 시간차로 떨어지는 범위 판정'임 (bAreaAtTarget).
	// 제자리에서 도는 검술 모션은 포격과 어울리지 않아 단순한 동작을 씀.
	// LobThrow는 힘을 모았다 내지르는 모션(1.83s), Barrage는 손을 모았다 내미는 시전 구간, CloseBlast 0.48s
	static ConstructorHelpers::FObjectFinder<UAnimSequence> LobThrowAnim(
		TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_ChargedAttack"));
	// 연속 포격은 원거리 시전자와 같은 손 모으기 구간(MM_Pistol_Equip 0.70s~)을 쓰고, 모은 손을 내민 자세로 연달아 쏘아 올림
	static ConstructorHelpers::FObjectFinder<UAnimSequence> BarrageAnim(
		TEXT("/Game/Characters/Mannequins/Anims/Pistol/MM_Pistol_Equip"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> CloseBlastAnim(
		TEXT("/Game/CJW/Assets/hackNSlash/Animations/Combo_10/Anim_Combo_10_Br_4"));
	AttackAnimation = LobThrowAnim.Object;
	AttackPatterns.Reset();

	// 던진 뒤 약 0.7초 날아가 떨어짐. 원이 보인 순간부터 1.6초 안에 빠져나오면 피함
	FEnemyAttackPattern& Shell = AttackPatterns.AddDefaulted_GetRef();
	Shell.Name = TEXT("ArtilleryShell");
	Shell.Animation = LobThrowAnim.Object;
	Shell.Damage = 6.3f;
	Shell.Cooldown = 1.2f;
	Shell.ImpactDelay = 1.6f;
	// 팔을 내지르는 순간(0.75s) 던져 0.85s 동안 포물선으로 날아감
	Shell.LobLaunchTime = 0.75f;
	Shell.MinRange = 250.0f;
	Shell.AreaRadius = 200.0f;
	Shell.bAreaAtTarget = true;
	Shell.KnockbackLift = 200.0f;
	Shell.Weight = 3.0f;

	// 대상 주변에 여러 발을 흩뿌림. 첫 발은 발밑이라 일단 움직여야 하고, 나머지 원 사이로 빠져나가야 함
	FEnemyAttackPattern& Barrage = AttackPatterns.AddDefaulted_GetRef();
	Barrage.Name = TEXT("ArtilleryBarrage");
	Barrage.Animation = BarrageAnim.Object;
	Barrage.AnimationStartTime = 0.70f;
	Barrage.AnimationDuration = 1.5f;
	Barrage.Damage = 4.2f;
	Barrage.Cooldown = 1.5f;
	Barrage.ImpactDelay = 1.4f;
	// 조준 자세에서 0.3s 간격으로 연달아 쏘아 올림. 각 포탄은 1.0s 날아감
	Barrage.LobLaunchTime = 0.4f;
	Barrage.HitCount = 4;
	Barrage.HitInterval = 0.3f;
	Barrage.MinRange = 350.0f;
	Barrage.AreaRadius = 160.0f;
	Barrage.bAreaAtTarget = true;
	Barrage.AreaScatter = 320.0f;
	Barrage.PatternCooldown = 7.0f;
	Barrage.Weight = 2.0f;

	// 붙어 온 대상을 밀어내고 다시 포격 거리를 벌리기 위한 근거리 폭발임
	FEnemyAttackPattern& CloseBlast = AttackPatterns.AddDefaulted_GetRef();
	CloseBlast.Name = TEXT("ArtilleryCloseBlast");
	CloseBlast.Animation = CloseBlastAnim.Object;
	CloseBlast.Damage = 2.8f;
	CloseBlast.Cooldown = 0.8f;
	CloseBlast.ImpactDelay = 0.3f;
	CloseBlast.MaxRange = 200.0f;
	CloseBlast.AreaRadius = 220.0f;
	CloseBlast.KnockbackStrength = 700.0f;
	CloseBlast.KnockbackLift = 150.0f;
	CloseBlast.PatternCooldown = 5.0f;
	CloseBlast.Weight = 4.0f;
	CloseBlast.bSuperArmor = true;
}
