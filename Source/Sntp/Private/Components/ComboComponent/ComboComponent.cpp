// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/ComboComponent/ComboComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Characters/SntpPlayerCharacter.h"
#include "Components/ComboComponent/ComboDataAsset.h"

// Sets default values for this component's properties
UComboComponent::UComboComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}

void UComboComponent::Init(UAbilitySystemComponent* InASC)
{
	ASC = InASC;
	CurrentNode = ComboData? ComboData->StartNode : NAME_None;
}

void UComboComponent::OnInput(EInputType Input)
{
	if (!bIsInCombo)
	{
		StartCombo(Input);
		UE_LOG(LogTemp, Log, TEXT("StartCombo"));
		return;
	}
	
	if (bComboWindowOpen)
	{
		PendingInput.InputType = Input;
		PendingInput.bIsValid = true;
		TryConsumeInput();
		return;
	}
	
	// Record pre input
	if (!bBufferWindowOpen)
	{
		return;
	}
	
	if (PendingInput.bIsValid)
	{
		if (GetPriority(Input) < GetPriority(PendingInput.InputType))
		{
			return;
		}
	}
	PendingInput.InputType = Input;
	PendingInput.bIsValid = true;
}

void UComboComponent::OpenComboWindow()
{
	bComboWindowOpen = true;
	UE_LOG(LogTemp, Warning, TEXT("Open Combo Window: Try Consume Input: "));
	TryConsumeInput();
}

void UComboComponent::CloseComboWindow()
{
	bComboWindowOpen = false;
	PendingInput.Clear();
}

const FComboNode* UComboComponent::GetNode(FName NodeID) const
{
	if (!ComboData) return nullptr;
	return ComboData->Nodes.Find(NodeID);
}

float UComboComponent::GetCurrentInputWindow() const
{
	if (const FComboNode* Node = GetNode(CurrentNode))
	{
		return Node->InputWindow;
	}
	return 0.3f;
}

void UComboComponent::ExecuteInput(EInputType Input)
{
	const FComboNode* Node = GetNode(CurrentNode);
	if (!Node)
	{
		ResetCombo();
		return;
	}
	
	const FName* NextNode = Node->NextNodes.Find(Input);
	if (!NextNode)
	{
		return;
	}
	CurrentNode = *NextNode;
	
	const FComboNode* NewNode = GetNode(CurrentNode);
	if (!NewNode)
	{
		ResetCombo();
		return;
	}
	UE_LOG(LogTemp, Log, TEXT("Enter Node: %s"), *NewNode->NodeID.ToString());
	
	if (ASC && NewNode->Ability)
	{
		bIsInCombo = true;
		ASC->TryActivateAbilityByClass(NewNode->Ability);
		bComboWindowOpen = false;
	}
	
	bComboWindowOpen = false;
}

void UComboComponent::ResetCombo()
{
	bIsInCombo = false;
	CurrentNode = ComboData? ComboData->StartNode : NAME_None;
	PendingInput.Clear();
}

void UComboComponent::TryConsumeInput()
{
	if (!PendingInput.bIsValid) return;
	if (bComboWindowOpen == false) return;
	UE_LOG(LogTemp, Warning, TEXT("Consume Success!"));
	ExecuteInput(PendingInput.InputType);
	PendingInput.Clear();
}

int32 UComboComponent::GetPriority(EInputType Input) const
{
	switch (Input)
	{
		case EInputType::Skill: return 3;
		case EInputType::Heavy: return 2;
		case EInputType::Attack: return 1;
		default: return 0;
	}
}

void UComboComponent::StartCombo(EInputType Input)
{
	bIsInCombo = true;
	CurrentNode = ComboData? ComboData->StartNode : NAME_None;
	ExecuteInput(Input);
}

void UComboComponent::Interrupt()
{
	if (ASC)
	{
		FGameplayTagContainer Tags;
		Tags.AddTag(FGameplayTag::RequestGameplayTag("Ability.Attack"));
		ASC->CancelAbilities(&Tags);
	}
	ResetCombo();
	
}

void UComboComponent::HitScan()
{
	USkeletalMeshComponent* Weapon = GetOwner<ASntpPlayerCharacter>()->GetWeaponMesh();
	if (!Weapon) return;
	
	FVector CurrentWeaponStart = Weapon->GetSocketLocation(FName("Start"));
	FVector CurrentWeaponMid = Weapon->GetSocketLocation(FName("Mid"));
	FVector CurrentWeaponEnd = Weapon->GetSocketLocation(FName("End"));
	
	if (!bHasLastFrame)
	{
		LastWeaponStart = CurrentWeaponStart;
		LastWeaponMid = CurrentWeaponMid;
		LastWeaponEnd = CurrentWeaponEnd;
		bHasLastFrame = true;
		return;
	}
	
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(GetOwner());
	Params.bTraceComplex = false;
	
	float Radius = 50.f;
	
	TArray<FHitResult> Hits;
	
	GetWorld()->SweepMultiByChannel(
		Hits,
		LastWeaponStart,
		CurrentWeaponStart,
		FQuat::Identity,
		ECC_Pawn,
		FCollisionShape::MakeSphere(Radius),
		Params);
	
	GetWorld()->SweepMultiByChannel(
		Hits,
		LastWeaponMid,
		CurrentWeaponMid,
		FQuat::Identity,
		ECC_Pawn,
		FCollisionShape::MakeSphere(Radius),
		Params);
	
	GetWorld()->SweepMultiByChannel(
		Hits,
		LastWeaponEnd,
		CurrentWeaponEnd,
		FQuat::Identity,
		ECC_Pawn,
		FCollisionShape::MakeSphere(Radius),
		Params
	);
	
	
	// Debug
	DrawDebugLine(GetWorld(), LastWeaponStart, CurrentWeaponStart, FColor::Red, false, 1.f, 0, 2);
	DrawDebugLine(GetWorld(), LastWeaponMid, CurrentWeaponMid, FColor::Red, false, 1.f, 0, 2);
	DrawDebugLine(GetWorld(), LastWeaponEnd, CurrentWeaponEnd, FColor::Red, false, 1.f, 0, 2);


	for (const FHitResult& Hit : Hits)
	{
		AActor* HitActor = Hit.GetActor();
		if (!HitActor) continue;

		if (HitActors.Contains(HitActor))
		{
			continue;
		}
		HitActors.AddUnique(HitActor);
		
		// Give the projectile a Gameplay Effect to Cause Damage;
		if (GetOwner()->HasAuthority())
		{
			if (UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitActor))
			{
				if (!WeaponTraceEffectHandle.Data)
				{
					return;
				}
				TargetASC->ApplyGameplayEffectSpecToSelf(*WeaponTraceEffectHandle.Data.Get());
			}
		}
	}
	LastWeaponStart = CurrentWeaponStart;
	LastWeaponMid = CurrentWeaponMid;
	LastWeaponEnd = CurrentWeaponEnd;
}

void UComboComponent::HitScanStart(FGameplayEffectSpecHandle DamageEffectSpecHandle)
{
	USkeletalMeshComponent* Weapon = GetOwner<ASntpPlayerCharacter>()->GetWeaponMesh();
	if (!Weapon) return;
	
	WeaponTraceEffectHandle = DamageEffectSpecHandle;
	if (!WeaponTraceEffectHandle.Data)
	{
		return;
	}
	HitActors.Reset();
	
	LastWeaponStart = Weapon->GetSocketLocation(FName("Start"));
	LastWeaponMid = Weapon->GetSocketLocation(FName("Mid"));
	LastWeaponEnd = Weapon->GetSocketLocation(FName("End"));
	bHasLastFrame = false;
	
	if (!GetWorld()) return;

	GetWorld()->GetTimerManager().SetTimer(
		WeaponHitTimer,
		this,
		&UComboComponent::HitScan,
		1.f / 60.f,
		true
	);
}

void UComboComponent::HitScanEnd()
{
	GetWorld()->GetTimerManager().ClearTimer(WeaponHitTimer);
	bHasLastFrame = false;
}









