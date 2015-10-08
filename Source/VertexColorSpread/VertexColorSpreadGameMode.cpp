// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "VertexColorSpread.h"
#include "VertexColorSpreadGameMode.h"
#include "VertexColorSpreadHUD.h"
#include "VertexColorSpreadCharacter.h"

AVertexColorSpreadGameMode::AVertexColorSpreadGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPersonCPP/Blueprints/FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

	// use our custom HUD class
	HUDClass = AVertexColorSpreadHUD::StaticClass();
}
