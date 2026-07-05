// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/EquipmentComponent/EquipmentComponent.h"

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
        if (Inventory->AddItem(Slot->ItemDef, 1) < 1)
        {
            return false;
        }

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

    if (Inventory->AddItem(Slot->ItemDef, 1) < 1)
    {
        return false;
    }

    RemoveEquipEffect(*Slot);

    Slot->ItemDef = nullptr;

    OnEquipmentChanged.Broadcast();

    return true;
}

bool UEquipmentComponent::ApplyEquipEffect(UItemDefinition* Item, FEquipmentSlotData& SlotData)
{
    // 下一步实现 GAS
    return true;
}

void UEquipmentComponent::RemoveEquipEffect(FEquipmentSlotData& SlotData)
{
    // 下一步实现 GAS
}

