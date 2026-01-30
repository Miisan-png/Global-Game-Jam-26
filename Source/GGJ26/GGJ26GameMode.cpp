// Copyright Epic Games, Inc. All Rights Reserved.

#include "GGJ26GameMode.h"
#include "GGJ26Character.h"
#include "UObject/ConstructorHelpers.h"

AGGJ26GameMode::AGGJ26GameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

}
