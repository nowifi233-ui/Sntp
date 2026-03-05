// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/GameplayAbilities/SntpProjectileSpell.h"

#include "Actors/SntpProjectile.h"
#include "EntitySystem/MovieSceneEntitySystemRunner.h"
#include "Interaction/CombatInterface.h"

void USntpProjectileSpell::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                           const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
                                           const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	
	SpawnProjectile();
}

void USntpProjectileSpell::SpawnProjectile()
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
		
		// TODO: Set Rotation
		
		ASntpProjectile* SntpProjectile = GetWorld()->SpawnActorDeferred<ASntpProjectile>(
			ProjectileClass,
			SpawnTransform,
			GetAvatarActorFromActorInfo(),
			Cast<APawn>(GetAvatarActorFromActorInfo()),
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		
		// TODO: Give the projectile a Gameplay Effect to Cause Damage;
		
		SntpProjectile->FinishSpawning(SpawnTransform);
	}
}
