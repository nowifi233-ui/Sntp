// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/GameplayAbilities/SntpProjectileSpell.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "SntpGameplayTags.h"
#include "Actors/SntpProjectile.h"
#include "EntitySystem/MovieSceneEntitySystemRunner.h"
#include "Interaction/CombatInterface.h"

void USntpProjectileSpell::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                           const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
                                           const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	
}

void USntpProjectileSpell::SpawnProjectile(const FVector& ProjectileTargetLocation)
{
	const FGameplayAbilityActivationInfo ActivationInfo = GetAvatarActorFromActorInfo();
	const bool bIsServer = HasAuthority(&ActivationInfo);
	if (!bIsServer)
	{
		return;
	}
	
	ICombatInterface* CombatInterface = Cast<ICombatInterface>(GetAvatarActorFromActorInfo());
	if (CombatInterface)
	{	
		FTransform SpawnTransform;
		const FVector SocketLocation = CombatInterface->GetCombatSocketLocation();
		SpawnTransform.SetLocation(SocketLocation);
		
		// Set Rotation
		FRotator Rotation = (ProjectileTargetLocation - SocketLocation).Rotation();
		Rotation.Pitch = 0.f;
		SpawnTransform.SetRotation(Rotation.Quaternion());
		
		ASntpProjectile* SntpProjectile = GetWorld()->SpawnActorDeferred<ASntpProjectile>(
			ProjectileClass,
			SpawnTransform,
			GetAvatarActorFromActorInfo(),
			Cast<APawn>(GetAvatarActorFromActorInfo()),
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		
		// Give the projectile a Gameplay Effect to Cause Damage;
		const UAbilitySystemComponent* SourceASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetAvatarActorFromActorInfo());
		const FGameplayEffectContextHandle EffectContext = SourceASC->MakeEffectContext();
		const FGameplayEffectSpecHandle EffectSpecHandle = SourceASC->MakeOutgoingSpec(DamageEffectClass, GetAbilityLevel(), EffectContext);

		const FSntpGameplayTags GameplayTags = FSntpGameplayTags::Get();
		const float ScaledDamage = Damage.GetValueAtLevel(GetAbilityLevel());
		
		UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(EffectSpecHandle, GameplayTags.Damage, ScaledDamage);
		SntpProjectile->DamageEffectSpecHandle = EffectSpecHandle;
		SntpProjectile->FinishSpawning(SpawnTransform);
	}
}
