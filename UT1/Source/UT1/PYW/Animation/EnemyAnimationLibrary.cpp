#include "PYW/Animation/EnemyAnimationLibrary.h"

#include "Animation/BlendSpace.h"

int32 UEnemyAnimationLibrary::RebuildBlendSpace(UBlendSpace* BlendSpace)
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
