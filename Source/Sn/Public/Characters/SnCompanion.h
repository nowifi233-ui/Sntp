// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "SnCompanion.generated.h"

class UAbilitySystemComponent;
class UGameplayAbility;
class UGameplayEffect;
class USnAttributeSet;

/**
 * Summoned pet. Like the enemy, the ASC and attribute set live here as
 * subobjects (standard GAS pattern for AI). Spawned and owned by the summon
 * ability (USnGA_SummonCompanion), which SetOwner()s it to the player after
 * spawning.
 *
 * Lifecycle rules:
 * - Despawned by the summon GA: its EndAbility (cancel / unequip removal)
 *   destroys the companion it spawned. Each GA only manages its own summon.
 * - Dies when the owner dies (binds the owner's AttributeSet OnDeath).
 * - Dies (Destroy, no death sequence) when killed itself.
 *
 * The attack skill is the SAME configurable GA slot type as the player's
 * (ASnPlayerState::AttackAbilityClass): a skill-family asset
 * (USnGA_ProjectileAttack / one-shot sphere blueprints - the projectile
 * family is the natural fit, the AI controller only pulls the trigger when
 * a shoot target exists) granted to the companion's own ASC on init and
 * activated by the AI controller by class when a target is in range. Damage
 * resolves from this ASC's attributes (GE_CompanionInit), the same way the
 * player's skills resolve from theirs.
 */
UCLASS()
class SN_API ASnCompanion : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ASnCompanion();

	//~ IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	//~ End of IAbilitySystemInterface

	UFUNCTION(BlueprintPure, Category = "Companion")
	USnAttributeSet* GetAttributeSet() const { return AttributeSet; }

	/** The attack skill the AI controller activates when a shoot target
	 *  exists (same GA slot type as the player's AttackAbilityClass). */
	UFUNCTION(BlueprintPure, Category = "Companion")
	TSubclassOf<UGameplayAbility> GetAttackAbilityClass() const { return AttackAbilityClass; }

	/** The player's live companion (owner match), or null if none. Pure lookup
	 *  helper - ownership and despawn belong to the summoning GA. */
	UFUNCTION(BlueprintCallable, Category = "Companion", meta = (WorldContext = "WorldContextObject"))
	static ASnCompanion* GetActiveCompanion(UObject* WorldContextObject, AActor* InOwner);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Initializes the ASC (owner = avatar = self) and applies default attributes. */
	void InitAbilityActorInfo();

	/** Binds our own OnDeath (killed in combat -> Destroy, no death sequence). */
	void BindAttributeCallbacks();

	/** Binds the owner's OnDeath (owner dies -> self-destruct). Returns true
	 *  once bound. */
	bool BindOwnerDeath();

	/** The owner's attribute set (null until the owner is set). GetAttributeSet
	 *  returns a const set in 5.7; we own the bind, so casting away const is
	 *  safe. */
	class USnAttributeSet* GetOwnerAttributeSet() const;

	/** Retries BindOwnerDeath: the summon ability SetOwner()s us after spawn,
	 *  so the owner may not exist yet at BeginPlay. Stops once bound -
	 *  despawning is the summon GA's job, nothing left to poll for. */
	void HandleOwnerBindTimer();

	/** Killed in combat: destroy immediately (no death fall, no loot). */
	UFUNCTION()
	void HandleCompanionDeath(AActor* Killer);

	/** The owner died: destroy alongside them. */
	UFUNCTION()
	void HandleOwnerDied(AActor* InInstigator);

	UPROPERTY(VisibleAnywhere, Category = "AbilitySystem")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, Category = "AbilitySystem")
	TObjectPtr<USnAttributeSet> AttributeSet;

	/** Applied on begin play to set up default attribute values (e.g. GE_CompanionInit). */
	UPROPERTY(EditDefaultsOnly, Category = "AbilitySystem")
	TSubclassOf<UGameplayEffect> DefaultAttributesEffect;

	/** The attack skill asset (same family as the player's AttackAbilityClass
	 *  - e.g. a projectile-family blueprint). Granted on init; the AI
	 *  controller activates it by class when a shoot target exists. Keep it
	 *  out of GrantedAbilities - it is granted from this slot only. */
	UPROPERTY(EditDefaultsOnly, Category = "AbilitySystem")
	TSubclassOf<UGameplayAbility> AttackAbilityClass;

	/** Extra abilities granted on init (passives, etc.), separate from
	 *  AttackAbilityClass - the class-activated attack must not be granted a
	 *  second time here. */
	UPROPERTY(EditDefaultsOnly, Category = "AbilitySystem")
	TArray<TSubclassOf<UGameplayAbility>> GrantedAbilities;

private:
	FTimerHandle OwnerBindTimerHandle;

	/** Set once the owner's death delegate is bound; stops the retry timer. */
	bool bOwnerBound = false;
};
