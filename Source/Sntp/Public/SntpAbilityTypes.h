#pragma once

#include "GameplayEffectTypes.h"
#include "SntpAbilityTypes.generated.h"

USTRUCT(BlueprintType)
struct FSntpGameplayEffectContext : public FGameplayEffectContext
{
	GENERATED_BODY()
public:
	bool IsCritical() const {return bIsCritical; }
	
	void SetIsCritical(bool bInIsCritical) { bIsCritical = bInIsCritical; }
	
	/** Returns the actual struct used for serialization, subclasses must override this! */
	virtual UScriptStruct* GetScriptStruct() const override
	{
		return StaticStruct();
	}
	
	virtual FSntpGameplayEffectContext* Duplicate() const override
	{
		FSntpGameplayEffectContext* NewContext = new FSntpGameplayEffectContext();
		*NewContext = *this;
		if (GetHitResult())
		{
			// Does a deep copy of the hit result
			NewContext->AddHitResult(*GetHitResult(), true);
		}
		return NewContext;
	}
	
	/** Custom serialization, subclasses must override this */
	virtual bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess) override;
	
protected:
	UPROPERTY()
	bool bIsCritical = false;
};

template<>
struct TStructOpsTypeTraits<FSntpGameplayEffectContext> : public TStructOpsTypeTraitsBase2<FSntpGameplayEffectContext>
{
	enum
	{
		WithNetSerializer = true,
		WithCopy = true
	};
};