// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilityTask/TargetDataUnderMouse.h"

#include "Dataflow/DataflowInstance.h"
#include "Kismet/GameplayStatics.h"

UTargetDataUnderMouse* UTargetDataUnderMouse::CreateTargetDataUnderMouse(UGameplayAbility* OwningAbility)
{
	UTargetDataUnderMouse* MyObj = NewAbilityTask<UTargetDataUnderMouse>(OwningAbility);
	
	
	return MyObj;
}

void UTargetDataUnderMouse::Activate()
{
	Super::Activate();

	const APlayerController* PlayerController = Ability->GetCurrentActorInfo()->PlayerController.Get();
	FHitResult HitResult;
	PlayerController->GetHitResultUnderCursor(ECC_Visibility, false, HitResult);
	ValidData.Broadcast(HitResult.Location);
}
