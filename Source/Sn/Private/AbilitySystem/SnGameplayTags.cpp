// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/SnGameplayTags.h"

namespace SnGameplayTags
{
	UE_DEFINE_GAMEPLAY_TAG(Attribute_Health, "Attribute.Health");
	UE_DEFINE_GAMEPLAY_TAG(Attribute_MaxHealth, "Attribute.MaxHealth");
	UE_DEFINE_GAMEPLAY_TAG(Attribute_AttackPower, "Attribute.AttackPower");
	UE_DEFINE_GAMEPLAY_TAG(Attribute_XP, "Attribute.XP");
	UE_DEFINE_GAMEPLAY_TAG(Attribute_Level, "Attribute.Level");
	UE_DEFINE_GAMEPLAY_TAG(Attribute_Capacity, "Attribute.Capacity");

	UE_DEFINE_GAMEPLAY_TAG(Attribute_MP, "Attribute.MP");
	UE_DEFINE_GAMEPLAY_TAG(Attribute_MaxMP, "Attribute.MaxMP");
	UE_DEFINE_GAMEPLAY_TAG(Attribute_MPRegen, "Attribute.MPRegen");
	UE_DEFINE_GAMEPLAY_TAG(Attribute_HealthRegen, "Attribute.HealthRegen");
	UE_DEFINE_GAMEPLAY_TAG(Attribute_Shield, "Attribute.Shield");

	UE_DEFINE_GAMEPLAY_TAG(Attribute_PhysicalDamage, "Attribute.PhysicalDamage");
	UE_DEFINE_GAMEPLAY_TAG(Attribute_PhysicalDamagePercent, "Attribute.PhysicalDamagePercent");
	UE_DEFINE_GAMEPLAY_TAG(Attribute_FireDamage, "Attribute.FireDamage");
	UE_DEFINE_GAMEPLAY_TAG(Attribute_IceDamage, "Attribute.IceDamage");
	UE_DEFINE_GAMEPLAY_TAG(Attribute_LightningDamage, "Attribute.LightningDamage");
	UE_DEFINE_GAMEPLAY_TAG(Attribute_FireDamagePercent, "Attribute.FireDamagePercent");
	UE_DEFINE_GAMEPLAY_TAG(Attribute_IceDamagePercent, "Attribute.IceDamagePercent");
	UE_DEFINE_GAMEPLAY_TAG(Attribute_LightningDamagePercent, "Attribute.LightningDamagePercent");
	UE_DEFINE_GAMEPLAY_TAG(Attribute_ArmorPenetration, "Attribute.ArmorPenetration");
	UE_DEFINE_GAMEPLAY_TAG(Attribute_AttackSpeed, "Attribute.AttackSpeed");
	UE_DEFINE_GAMEPLAY_TAG(Attribute_MoveSpeed, "Attribute.MoveSpeed");
	UE_DEFINE_GAMEPLAY_TAG(Attribute_Defense, "Attribute.Defense");
	UE_DEFINE_GAMEPLAY_TAG(Attribute_Toughness, "Attribute.Toughness");
	UE_DEFINE_GAMEPLAY_TAG(Attribute_Dodge, "Attribute.Dodge");
	UE_DEFINE_GAMEPLAY_TAG(Attribute_FireResistance, "Attribute.FireResistance");
	UE_DEFINE_GAMEPLAY_TAG(Attribute_IceResistance, "Attribute.IceResistance");
	UE_DEFINE_GAMEPLAY_TAG(Attribute_LightningResistance, "Attribute.LightningResistance");
	UE_DEFINE_GAMEPLAY_TAG(Attribute_CritChance, "Attribute.CritChance");
	UE_DEFINE_GAMEPLAY_TAG(Attribute_CritDamage, "Attribute.CritDamage");

	UE_DEFINE_GAMEPLAY_TAG(Ability_Attack, "Ability.Attack");

	UE_DEFINE_GAMEPLAY_TAG(Difficulty_Applied, "Difficulty.Applied");

	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Attack, "Cooldown.Attack");
	UE_DEFINE_GAMEPLAY_TAG(Cooldown_EnemyAttack, "Cooldown.EnemyAttack");
	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Use, "Cooldown.Use");

	UE_DEFINE_GAMEPLAY_TAG(Event_AttackHit, "Event.AttackHit");

	UE_DEFINE_GAMEPLAY_TAG(Event_AttackWindow, "Event.AttackWindow");

	UE_DEFINE_GAMEPLAY_TAG(Event_DamageTaken, "Event.DamageTaken");
	UE_DEFINE_GAMEPLAY_TAG(Event_Kill, "Event.Kill");

	UE_DEFINE_GAMEPLAY_TAG(Event_AttributeGained_Health, "Event.AttributeGained.Health");
	UE_DEFINE_GAMEPLAY_TAG(Event_AttributeGained_Shield, "Event.AttributeGained.Shield");
	UE_DEFINE_GAMEPLAY_TAG(Event_AttributeGained_MP, "Event.AttributeGained.MP");

	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_DamageNumber, "GameplayCue.DamageNumber");

	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_EnemyDeathFall, "GameplayCue.EnemyDeathFall");

	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_AuraFrostBurst, "GameplayCue.Aura.FrostBurst");

	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Swing_Melee, "GameplayCue.Swing.Melee");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Hit_Melee, "GameplayCue.Hit.Melee");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Swing_Fire, "GameplayCue.Swing.Fire");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Hit_Fire, "GameplayCue.Hit.Fire");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Hit_FireProjectile, "GameplayCue.Hit.FireProjectile");

	UE_DEFINE_GAMEPLAY_TAG(Companion_Summon, "Companion.Summon");

	UE_DEFINE_GAMEPLAY_TAG(Combat_Team_Enemy, "Combat.Team.Enemy");

	UE_DEFINE_GAMEPLAY_TAG(Combat_Team_Player, "Combat.Team.Player");

	UE_DEFINE_GAMEPLAY_TAG(Status_Burning, "Status.Burning");

	UE_DEFINE_GAMEPLAY_TAG(SetByCaller_Damage_Physical, "SetByCaller.Damage.Physical");
	UE_DEFINE_GAMEPLAY_TAG(SetByCaller_Damage_Fire, "SetByCaller.Damage.Fire");
	UE_DEFINE_GAMEPLAY_TAG(SetByCaller_Damage_Ice, "SetByCaller.Damage.Ice");
	UE_DEFINE_GAMEPLAY_TAG(SetByCaller_Damage_Lightning, "SetByCaller.Damage.Lightning");
	UE_DEFINE_GAMEPLAY_TAG(SetByCaller_Damage_Penetration, "SetByCaller.Damage.Penetration");
	UE_DEFINE_GAMEPLAY_TAG(SetByCaller_Damage_Critical, "SetByCaller.Damage.Critical");

	UE_DEFINE_GAMEPLAY_TAG(SetByCaller_XP, "SetByCaller.XP");
	UE_DEFINE_GAMEPLAY_TAG(SetByCaller_UseCD, "SetByCaller.UseCD");
	UE_DEFINE_GAMEPLAY_TAG(SetByCaller_BurnDuration, "SetByCaller.BurnDuration");

	UE_DEFINE_GAMEPLAY_TAG(SetByCaller_Health, "SetByCaller.Health");
	UE_DEFINE_GAMEPLAY_TAG(SetByCaller_MaxHealth, "SetByCaller.MaxHealth");
	UE_DEFINE_GAMEPLAY_TAG(SetByCaller_AttackPower, "SetByCaller.AttackPower");
	UE_DEFINE_GAMEPLAY_TAG(SetByCaller_Capacity, "SetByCaller.Capacity");

	UE_DEFINE_GAMEPLAY_TAG(SetByCaller_MP, "SetByCaller.MP");
	UE_DEFINE_GAMEPLAY_TAG(SetByCaller_MaxMP, "SetByCaller.MaxMP");
	UE_DEFINE_GAMEPLAY_TAG(SetByCaller_MPRegen, "SetByCaller.MPRegen");
	UE_DEFINE_GAMEPLAY_TAG(SetByCaller_HealthRegen, "SetByCaller.HealthRegen");
	UE_DEFINE_GAMEPLAY_TAG(SetByCaller_PhysicalDamage, "SetByCaller.PhysicalDamage");
	UE_DEFINE_GAMEPLAY_TAG(SetByCaller_PhysicalDamagePercent, "SetByCaller.PhysicalDamagePercent");
	UE_DEFINE_GAMEPLAY_TAG(SetByCaller_FireDamage, "SetByCaller.FireDamage");
	UE_DEFINE_GAMEPLAY_TAG(SetByCaller_IceDamage, "SetByCaller.IceDamage");
	UE_DEFINE_GAMEPLAY_TAG(SetByCaller_LightningDamage, "SetByCaller.LightningDamage");
	UE_DEFINE_GAMEPLAY_TAG(SetByCaller_FireDamagePercent, "SetByCaller.FireDamagePercent");
	UE_DEFINE_GAMEPLAY_TAG(SetByCaller_IceDamagePercent, "SetByCaller.IceDamagePercent");
	UE_DEFINE_GAMEPLAY_TAG(SetByCaller_LightningDamagePercent, "SetByCaller.LightningDamagePercent");
	UE_DEFINE_GAMEPLAY_TAG(SetByCaller_ArmorPenetration, "SetByCaller.ArmorPenetration");
	UE_DEFINE_GAMEPLAY_TAG(SetByCaller_AttackSpeed, "SetByCaller.AttackSpeed");
	UE_DEFINE_GAMEPLAY_TAG(SetByCaller_MoveSpeed, "SetByCaller.MoveSpeed");
	UE_DEFINE_GAMEPLAY_TAG(SetByCaller_Defense, "SetByCaller.Defense");
	UE_DEFINE_GAMEPLAY_TAG(SetByCaller_Toughness, "SetByCaller.Toughness");
	UE_DEFINE_GAMEPLAY_TAG(SetByCaller_Dodge, "SetByCaller.Dodge");
	UE_DEFINE_GAMEPLAY_TAG(SetByCaller_FireResistance, "SetByCaller.FireResistance");
	UE_DEFINE_GAMEPLAY_TAG(SetByCaller_IceResistance, "SetByCaller.IceResistance");
	UE_DEFINE_GAMEPLAY_TAG(SetByCaller_LightningResistance, "SetByCaller.LightningResistance");
	UE_DEFINE_GAMEPLAY_TAG(SetByCaller_CritChance, "SetByCaller.CritChance");
	UE_DEFINE_GAMEPLAY_TAG(SetByCaller_CritDamage, "SetByCaller.CritDamage");
	UE_DEFINE_GAMEPLAY_TAG(SetByCaller_CooldownReduction, "SetByCaller.CooldownReduction");
	UE_DEFINE_GAMEPLAY_TAG(SetByCaller_Shield, "SetByCaller.Shield");

	UE_DEFINE_GAMEPLAY_TAG(Item_Type_Equipment, "Item.Type.Equipment");
	UE_DEFINE_GAMEPLAY_TAG(Item_Type_Consumable, "Item.Type.Consumable");
	UE_DEFINE_GAMEPLAY_TAG(Item_Type_Material, "Item.Type.Material");
	UE_DEFINE_GAMEPLAY_TAG(Item_Type_Rune, "Item.Type.Rune");

	UE_DEFINE_GAMEPLAY_TAG(Item_Bond_Food, "Item.Bond.Food");
	UE_DEFINE_GAMEPLAY_TAG(Item_Bond_Pet, "Item.Bond.Pet");
	UE_DEFINE_GAMEPLAY_TAG(Item_Bond_Precision, "Item.Bond.Precision");
	UE_DEFINE_GAMEPLAY_TAG(Item_Bond_Burning, "Item.Bond.Burning");
	UE_DEFINE_GAMEPLAY_TAG(Item_Bond_Frozen, "Item.Bond.Frozen");
	UE_DEFINE_GAMEPLAY_TAG(Item_Bond_Lightning, "Item.Bond.Lightning");

	UE_DEFINE_GAMEPLAY_TAG(Item_Source_Bag, "Item.Source.Bag");
	UE_DEFINE_GAMEPLAY_TAG(Item_Source_Equipment, "Item.Source.Equipment");
	UE_DEFINE_GAMEPLAY_TAG(Item_Source_Storage, "Item.Source.Storage");
}
