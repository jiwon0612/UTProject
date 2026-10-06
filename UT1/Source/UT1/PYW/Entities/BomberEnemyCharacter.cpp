#include "PYW/Entities/BomberEnemyCharacter.h"

#include "Animation/AnimSequence.h"
#include "Components/CapsuleComponent.h"
#include "UObject/ConstructorHelpers.h"

ABomberEnemyCharacter::ABomberEnemyCharacter()
{
	GetCapsuleComponent()->SetRelativeScale3D(FVector(0.8f));
	CombatMovement = EEnemyCombatMovement::HoldGround;
	MaxHealth = 40.0f;
	AttackRange = 60.0f;
	WalkSpeed = 200.0f;
	ChaseSpeed = 520.0f;
	DetectionRange = 1600.0f;
	ReactionTime = 0.2f;
	// 한두 번만 맞혀도 경직되어 점화가 끊기게 함. 대응 수단을 주기 위함
	PoiseThreshold = 12.0f;
	StaggerDuration = 0.7f;

	static ConstructorHelpers::FObjectFinder<UAnimSequence> ChargedAttack(
		TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_ChargedAttack"));
	AttackAnimation = ChargedAttack.Object;
	AttackPatterns.Reset();

	FEnemyAttackPattern& SelfDestruct = AttackPatterns.AddDefaulted_GetRef();
	SelfDestruct.Name = TEXT("BomberSelfDestruct");
	SelfDestruct.Animation = ChargedAttack.Object;
	SelfDestruct.Damage = 45.0f;
	SelfDestruct.Cooldown = 1.0f;
	SelfDestruct.ImpactDelay = 1.1f;
	SelfDestruct.AreaRadius = 300.0f;
	SelfDestruct.KnockbackStrength = 700.0f;
	SelfDestruct.KnockbackLift = 300.0f;
	SelfDestruct.bConsumesSelf = true;
}
