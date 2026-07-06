// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/EquipmentComponent/EquipmentComponent.h"

#include "Characters/SntpPlayerCharacter.h"
#include "Data/ItemDefinition.h"
#include "Components/InventoryComponent.h"


UEquipmentComponent::UEquipmentComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UEquipmentComponent::BeginPlay()
{
    Super::BeginPlay();

    InitializeSlots();
}

void UEquipmentComponent::InitializeSlots()
{
    EquipmentSlots.Empty();

    auto AddSlot = [&](EEquipmentSlot Slot)
    {
        FEquipmentSlotData Data;
        Data.Slot = Slot;
        EquipmentSlots.Add(Data);
    };

    AddSlot(EEquipmentSlot::Weapon);

    AddSlot(EEquipmentSlot::Head);
    AddSlot(EEquipmentSlot::Chest);
    AddSlot(EEquipmentSlot::Legs);
    AddSlot(EEquipmentSlot::Feet);

    AddSlot(EEquipmentSlot::Ring1);
    AddSlot(EEquipmentSlot::Ring2);

    AddSlot(EEquipmentSlot::Necklace);
}

FEquipmentSlotData* UEquipmentComponent::FindSlot(EEquipmentSlot Slot)
{
    for (FEquipmentSlotData& Data : EquipmentSlots)
    {
        if (Data.Slot == Slot)
        {
            return &Data;
        }
    }

    return nullptr;
}

const FEquipmentSlotData* UEquipmentComponent::FindSlot(EEquipmentSlot Slot) const
{
    for (const FEquipmentSlotData& Data : EquipmentSlots)
    {
        if (Data.Slot == Slot)
        {
            return &Data;
        }
    }

    return nullptr;
}

UItemDefinition* UEquipmentComponent::GetEquipment(EEquipmentSlot Slot) const
{
    const FEquipmentSlotData* Data = FindSlot(Slot);

    return Data ? Data->ItemDef : nullptr;
}

bool UEquipmentComponent::IsSlotOccupied(EEquipmentSlot Slot) const
{
    const FEquipmentSlotData* Data = FindSlot(Slot);

    return Data && Data->ItemDef != nullptr;
}

void UEquipmentComponent::BroadCastOnEquipmentChanged()
{
    OnEquipmentChanged.Broadcast();
}

bool UEquipmentComponent::EquipItem(UInventoryComponent* Inventory, int32 InventoryIndex)
{
    if (!Inventory)
    {
        return false;
    }

    if (!Inventory->Items.IsValidIndex(InventoryIndex))
    {
        return false;
    }

    FItemInstance& Item = Inventory->Items[InventoryIndex];

    if (Item.IsEmpty())
    {
        return false;
    }

    if (!Item.ItemDef->bEquipable)
    {
        return false;
    }

    FEquipmentSlotData* Slot = FindSlot(Item.ItemDef->EquipSlot);

    if (!Slot)
    {
        return false;
    }

    // 已有装备，先放回背包
    if (Slot->ItemDef)
    {
        Inventory->AddItem(Slot->ItemDef, 1);
        RemoveEquipEffect(*Slot);
        Slot->ItemDef = nullptr;
    }

    Slot->ItemDef = Item.ItemDef;

    ApplyEquipEffect(Item.ItemDef, *Slot);

    Inventory->RemoveItemByIndex(InventoryIndex, 1);

    OnEquipmentChanged.Broadcast();

    return true;
}

bool UEquipmentComponent::Unequip(EEquipmentSlot SlotType, UInventoryComponent* Inventory)
{
    if (!Inventory)
    {
        return false;
    }

    FEquipmentSlotData* Slot = FindSlot(SlotType);

    if (!Slot)
    {
        return false;
    }

    if (!Slot->ItemDef)
    {
        return false;
    }

    Inventory->AddItem(Slot->ItemDef, 1);
    RemoveEquipEffect(*Slot);
    Slot->ItemDef = nullptr;
    OnEquipmentChanged.Broadcast();

    return true;
}

bool UEquipmentComponent::ApplyEquipEffect(UItemDefinition* Item, FEquipmentSlotData& SlotData)
{
    // 下一步实现 GAS
    ASntpPlayerCharacter* Character = GetOwner<ASntpPlayerCharacter>();
    if (!Character)
    {
        return false;
    }
    UAbilitySystemComponent* ASC = Character->GetAbilitySystemComponent();
    if (!ASC)
    {
        return false;
    }
    
    if (!Item || !Item->UseEffect)
    {
        return false;
    }
    SlotData.EffectHandle = ASC->ApplyGameplayEffectSpecToSelf(*ASC->MakeOutgoingSpec(Item->UseEffect, 1.f, ASC->MakeEffectContext()).Data);
    
    return SlotData.EffectHandle.IsValid();
}

void UEquipmentComponent::RemoveEquipEffect(FEquipmentSlotData& SlotData)
{
    // 下一步实现 GAS
    ASntpPlayerCharacter* Character = Cast<ASntpPlayerCharacter>(GetOwner());

    if (!Character)
    {
        return;
    }

    UAbilitySystemComponent* ASC = Character->GetAbilitySystemComponent();

    if (!ASC)
    {
        return;
    }

    if (SlotData.EffectHandle.IsValid())
    {
        ASC->RemoveActiveGameplayEffect(SlotData.EffectHandle);

        SlotData.EffectHandle.Invalidate();
    }
}

