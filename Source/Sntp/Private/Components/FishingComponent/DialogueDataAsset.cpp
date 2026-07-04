// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/FishingComponent/DialogueDataAsset.h"

FDialogueNode UDialogueDataAsset::GetNodeByID(FName NodeID)
{
	for (FDialogueNode Node : Nodes)
	{
		if (Node.NodeID == NodeID)
		{
			return Node;
		}
	}
	return FDialogueNode();
}
