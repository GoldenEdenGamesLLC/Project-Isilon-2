#pragma once

#include "CoreMinimal.h"
#include "Engine/HitResult.h"

struct FSphereInteractionParams
{
	UPrimitiveComponent* OverlappedComponent = nullptr;
	AActor* OtherActor = nullptr;
	UPrimitiveComponent* OtherComp = nullptr;
	int32 OtherBodyIndex = INDEX_NONE;
	bool bFromSweep = false;
	FHitResult SweepResult;
};