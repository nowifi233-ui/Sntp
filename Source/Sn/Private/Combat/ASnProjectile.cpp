// Fill out your copyright notice in the Description page of Project Settings.

#include "Combat/ASnProjectile.h"

#include "Abilities/GameplayAbilityTypes.h"
#include "AbilitySystem/SnGameplayEffectContext.h"
#include "AbilitySystem/SnGameplayTags.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Combat/ASnDestructible.h"
#include "Combat/USnCombatLibrary.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/EngineTypes.h"
#include "Engine/World.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "TimerManager.h"

ASnProjectile::ASnProjectile()
{
	// No actor tick: the ProjectileMovement drives the flight.
	PrimaryActorTick.bCanEverTick = false;

	// Spawned on the server by the firing ability; movement replication
	// carries the flight to every client (bReplicateMovement is off by
	// default on AActor - without it clients would only get the spawn
	// transform and the missile would hang in the air).
	SetReplicates(true);
	SetReplicateMovement(true);

	// Sweep body: blocks the world (a wall/ground hit stops the flight and
	// the server destroys the missile), overlaps pawns (the enemy hit test
	// filters there - IsEnemy reads the ASC tag, so allies pass through).
	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	CollisionSphere->InitSphereRadius(12.f);
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionSphere->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionSphere->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	CollisionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CollisionSphere->SetGenerateOverlapEvents(true);
	RootComponent = CollisionSphere;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(CollisionSphere);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	// (UpdatedComponent auto-wires to the actor root - the collision sphere -
	// at startup; no manual assignment needed.)
	// The initial direction is resolved from the SPAWN ROTATION at
	// InitialSpeed (bInitialVelocityInLocalSpace defaults true), so the
	// launcher only aims the spawn transform - no velocity code. Speed /
	// gravity / turn rate are numbers on the blueprint CDO (one-place rule);
	// the defaults below are a usable straight fireball for unconfigured
	// tests, overridden by any real asset.
	Movement->InitialSpeed = 2000.f;
	Movement->MaxSpeed = 2000.f;
	Movement->bRotationFollowsVelocity = true;
	Movement->bShouldBounce = false;
	// Straight-line default: full gravity would arc a flat shot into the
	// ground within meters (drop = 0.5 x 980 x t^2 - a 20 m shot drops
	// ~5 m and dies on the ground before reaching its target). A lobbed /
	// arcing skill opts in by raising ProjectileGravityScale on its own
	// projectile blueprint.
	Movement->ProjectileGravityScale = 0.f;
}

void ASnProjectile::InitializeProjectile(FGameplayEffectSpecHandle InDamageSpec, AActor* InAttacker, UAbilitySystemComponent* InSourceASC, FGameplayTag InHitCueTag)
{
	DamageSpecHandle = InDamageSpec;
	Attacker = InAttacker;
	SourceASC = InSourceASC;
	HitCueTag = InHitCueTag;
}

void ASnProjectile::SetHomingTarget(USceneComponent* TargetComponent)
{
	if (Movement && TargetComponent)
	{
		Movement->HomingTargetComponent = TargetComponent;
		Movement->bIsHomingProjectile = true;
	}
}

void ASnProjectile::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		// Server-only gameplay wiring. 5.7's ProjectileMovementComponent has
		// no built-in lifetime (MaxLifeTime is gone), so the server recycles
		// a missile that hit nothing after LifetimeSeconds - a miss never
		// flies on, and the replicated destroy tells every client to drop
		// its copy.
		GetWorldTimerManager().SetTimer(LifetimeHandle, this, &ASnProjectile::HandleLifetimeExpired, LifetimeSeconds, /*bLoop=*/false);
		CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &ASnProjectile::OnSphereOverlap);
		Movement->OnProjectileStop.AddDynamic(this, &ASnProjectile::OnProjectileStopped);
	}
}

void ASnProjectile::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority() || bHasHit || !OtherActor)
	{
		return;
	}

	// A destructible hit resolves the missile too: no ASC, so IsEnemy is
	// false for it - one impact counts one hit toward breaking, pops the
	// impact cue, and the missile dies (the enemy path's single-hit rule).
	// The cue params mirror the enemy block below (see its comment).
	if (ASnDestructible* Destructible = USnCombatLibrary::FindDestructible(OtherActor))
	{
		bHasHit = true;
		Destructible->RegisterHit();
		if (HitCueTag.IsValid() && SourceASC.IsValid())
		{
			FGameplayCueParameters HitParams;
			HitParams.Location = bFromSweep ? FVector(SweepResult.ImpactPoint) : OtherActor->GetActorLocation();
			HitParams.Normal = bFromSweep ? FVector(SweepResult.Normal) : FVector::UpVector;
			HitParams.Instigator = Attacker.Get();
			SourceASC->ExecuteGameplayCue(HitCueTag, HitParams);
		}
		Destroy();
		return;
	}

	// The same hostile-tag source as every combat query (Combat.Team.Enemy):
	// allies, companions and neutrals simply pass through. Known asymmetry:
	// with no ASC on the other side (e.g. a door) there is nothing to hit -
	// the overlap is ignored and the missile flies on.
	if (!USnCombatLibrary::IsEnemy(OtherActor))
	{
		return;
	}

	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherActor);
	if (!TargetASC)
	{
		return;
	}

	// Single hit: the first enemy contact resolves the missile (no pierce -
	// the approved scope). The flag guards the same-frame overlap batch:
	// without it two enemies touched in one movement tick could both take
	// the hit before the destroy lands.
	bHasHit = true;

	// The delayed spec applied NOW, on the server, through the same
	// application call the instant path uses (ApplyDamageToTarget applies
	// the very same spec immediately). Kill credit and mitigation work off
	// the spec's context (instigator + element), set by the launcher.
	if (DamageSpecHandle.IsValid() && SourceASC.IsValid())
	{
		SourceASC->ApplyGameplayEffectSpecToTarget(*DamageSpecHandle.Data.Get(), TargetASC);

		// Attack-hit event, fired once per resolved hit like the sphere
		// family's: a projectile impact is a PRIMARY hit - enchantment GAs
		// append on it and bond passives (the ignite) filter by element, so a
		// fire missile can ignite and every element enchant sees the missile's
		// own hit. The event's context carries the missile's element, read
		// from the delayed spec (set by the launcher at fire time). Secondary
		// damage (enchantment appends) and DoT ticks never fire the event -
		// that discipline lives with the listeners (see SnGameplayTags).
		const FSnGameplayEffectContext* HitContext = static_cast<const FSnGameplayEffectContext*>(DamageSpecHandle.Data->GetContext().Get());
		const EDamageType HitElement = HitContext ? HitContext->GetDamageType() : EDamageType::Physical;

		// Event magnitude = the missile's damage, read back from the delayed
		// spec (the launcher wrote it at fire time, negative under the
		// element's set-by-caller key), so the proc family's "share of the
		// triggering amount" payload works on a missile hit too. Attacker-side
		// number - pre-mitigation, see the sphere family's send site.
		FGameplayEventData EventData;
		EventData.Instigator = Attacker.Get();
		EventData.Target = OtherActor;
		EventData.EventMagnitude = FMath::Abs(DamageSpecHandle.Data->GetSetByCallerMagnitude(USnCombatLibrary::GetDamageSetByCallerKey(HitElement)));
		FGameplayEffectContextHandle EventContext = SourceASC->MakeEffectContext();
		if (FSnGameplayEffectContext* SnEventContext = static_cast<FSnGameplayEffectContext*>(EventContext.Get()))
		{
			SnEventContext->SetDamageType(HitElement);
		}
		EventData.ContextHandle = EventContext;
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Attacker.Get(), SnGameplayTags::Event_AttackHit, EventData);
	}

	// Impact cue at the true hit point: the sweep's impact data when the
	// overlap came from the flight sweep (always true for a flying missile),
	// the victim's location otherwise. Executed on the source ASC - the
	// empty-key multicast plays once on every machine (the notify carries
	// the location/normal/instigator to each side).
	if (HitCueTag.IsValid() && SourceASC.IsValid())
	{
		FGameplayCueParameters HitParams;
		// Wrap the sweep fields in FVector first: the hit result's location and
		// normal are the quantized network types (FVector_NetQuantize[-Normal]),
		// so the ternary's arms would have no common type.
		HitParams.Location = bFromSweep ? FVector(SweepResult.ImpactPoint) : OtherActor->GetActorLocation();
		HitParams.Normal = bFromSweep ? FVector(SweepResult.Normal) : FVector::UpVector;
		HitParams.Instigator = Attacker.Get();
		SourceASC->ExecuteGameplayCue(HitCueTag, HitParams);
	}

	Destroy();
}

void ASnProjectile::OnProjectileStopped(const FHitResult& ImpactResult)
{
	// The flight was blocked by the world (wall / ground - pawns overlap, so
	// they never stop it): the missile dies there, no bounce. Client copies
	// stop locally too, but only the server destroys - its replicated
	// destroy removes every copy.
	if (HasAuthority() && !bHasHit)
	{
		Destroy();
	}
}

void ASnProjectile::HandleLifetimeExpired()
{
	if (HasAuthority())
	{
		Destroy();
	}
}
