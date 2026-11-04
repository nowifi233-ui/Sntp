// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/SnTutorialZone.h"

#include "Components/SphereComponent.h"
#include "Dialogue/FSnDialogueRow.h"
#include "Game/SnSettings.h"
#include "Game/USnGameInstance.h"
#include "Player/SnPlayerController.h"

ASnTutorialZone::ASnTutorialZone()
{
	PrimaryActorTick.bCanEverTick = false;

	// Query-only overlap sphere, Pawn channel only. Enemies and companions are
	// pawns too - who acts on the overlap is filtered in HandleOverlap, not
	// here.
	TriggerSphere = CreateDefaultSubobject<USphereComponent>(TEXT("TriggerSphere"));
	SetRootComponent(TriggerSphere);
	TriggerSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerSphere->SetCollisionObjectType(ECC_WorldDynamic);
	TriggerSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	TriggerSphere->SetHiddenInGame(true);
	TriggerSphere->OnComponentBeginOverlap.AddDynamic(this, &ASnTutorialZone::HandleOverlap);

	ApplyRadius();
}

void ASnTutorialZone::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyRadius();
}

void ASnTutorialZone::ApplyRadius()
{
	if (TriggerSphere)
	{
		TriggerSphere->SetSphereRadius(TriggerRadius);
	}
}

#if WITH_EDITOR
void ASnTutorialZone::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(ASnTutorialZone, TriggerRadius))
	{
		ApplyRadius();
	}
}
#endif

void ASnTutorialZone::HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// Real players only: AI-driven companions and summons also walk around
	// (they carry the "Player" aggro tag), but they must never open a
	// tutorial. IsPlayerControlled means an actual player state - the safe
	// test here.
	APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn || !Pawn->IsPlayerControlled())
	{
		return;
	}

	// Single-player discipline (same as the hub intro): the popup locks game
	// input (UI-only mode) - it never fires in a LAN session.
	if (GetNetMode() != NM_Standalone)
	{
		return;
	}

	USnGameInstance* SnGI = Cast<USnGameInstance>(GetGameInstance());
	if (!SnGI)
	{
		return;
	}

	// Once ever, persisted in the save (the milestone IS the tutorial).
	if (SnGI->HasMilestone(TutorialRow))
	{
		return;
	}

	// The row must exist before the milestone is written: a zone with a
	// missing row logs, stays live, and re-triggers once the config is fixed
	// (a burned milestone would leave it dead forever). Deliberate dev-error
	// path - the warning is the fix prompt.
	UDataTable* Table = USnSettings::Get()->GetDialogueDataTable();
	if (TutorialRow.IsNone() || !Table || !Table->FindRow<FSnDialogueRow>(TutorialRow, TEXT("ASnTutorialZone::HandleOverlap")))
	{
		UE_LOG(LogTemp, Warning, TEXT("ASnTutorialZone %s: TutorialRow '%s' 不在对话表（DT_Dialogue）里 - 先修配置"), *GetName(), *TutorialRow.ToString());
		return;
	}

	// Triggered = taught. Written before the popup opens, so skipping with
	// Escape still counts; the zone then disables itself for good (double
	// insurance - a standing player never re-overlaps anyway).
	SnGI->AddMilestone(TutorialRow);
	SetActorEnableCollision(false);

	if (ASnPlayerController* SnPC = Cast<ASnPlayerController>(Pawn->GetController()))
	{
		// Tutorial: pause behind the popup (single-player only - released by
		// the PC when the popup closes). The world freezing behind a tutorial
		// also keeps enemies from killing the player mid-read.
		SnPC->PlayDialogue(TutorialRow, /*bPauseGame=*/true);
	}
}
