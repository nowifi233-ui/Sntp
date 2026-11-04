// Copyright Epic Games, Inc. All Rights Reserved.

#include "Sn.h"
#include "AbilitySystemGlobals.h"
#include "Modules/ModuleManager.h"

class FSnModule : public FDefaultGameModuleImpl
{
public:
	virtual void StartupModule() override
	{
		// Instantiate the config-selected AbilitySystemGlobals subclass
		// (USnAbilitySystemGlobals, set in DefaultGame.ini): every context
		// allocated from now on is an FSnGameplayEffectContext, which carries
		// the element / crit / miss flags the damage ExecCalc and the attribute
		// set exchange. Must run before the first effect is created -
		// InitGlobalData is idempotent.
		UAbilitySystemGlobals::Get().InitGlobalData();
	}
};

IMPLEMENT_PRIMARY_GAME_MODULE( FSnModule, Sn, "Sn" );
