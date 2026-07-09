// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ChatType.h"
#include "Subsystems/WorldSubsystem.h"
#include "UChatManagerSubsystem.generated.h"

class UChatComponent;
/**
 * 
 */
UCLASS()
class SNTP_API UUChatManagerSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
	
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;	
	
public:
	/**/
	void HandleWorldMessage(AActor* Sender, const FString& Context);
	
	void HandlePrivateMessage(AActor* Sender, const FString& TargetName, const FString& Context);
	
	void BroadcastSystemMessage(const FString& Content);
	
private:
	void GetAllChatComponents(TArray<UChatComponent*>& Components);
	
	FChatMessage MakeMessage(AActor* Sender, const FString& Context, EChatChannel Channel);
	
	
};
