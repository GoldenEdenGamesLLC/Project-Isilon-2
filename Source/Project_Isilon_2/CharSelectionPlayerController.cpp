// Fill out your copyright notice in the Description page of Project Settings.


#include "CharSelectionPlayerController.h"
#include "CharacterSelectionGameMode.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Engine/World.h"

void ACharSelectionPlayerController::BeginPlay()
{
    Super::BeginPlay();

    if(IsLocalController())
    {
        ShowCharacterSelection();
    }
}

void ACharSelectionPlayerController::ShowCharacterSelection()
{
    if(!IsLocalController() || !CharacterSelectionWidgetClass)
    {
        return;
    }

    CharacterSelectionWidget = CreateWidget<UUserWidget>(this, CharacterSelectionWidgetClass);

    if(CharacterSelectionWidget)
    {
        CharacterSelectionWidget->AddToViewport();

        bShowMouseCursor = true;

        UWidgetBlueprintLibrary::SetInputMode_UIOnlyEx(this, CharacterSelectionWidget);
    }
}

void ACharSelectionPlayerController::SelectCharacter(EPlayableCharacter PlayerChar)
{
    ServerSelectCharacter(PlayerChar);
}

void ACharSelectionPlayerController::ServerSelectCharacter_Implementation(EPlayableCharacter PlayerChar)
{
    if(bHasSelectedCharacter)
    {
        return;
    }

    ACharacterSelectionGameMode* GM = GetWorld()->GetAuthGameMode<ACharacterSelectionGameMode>();

    if(!GM)
    {
        return;
    }

    GM->SpawnSelectedCharacter(this, PlayerChar);
}

void ACharSelectionPlayerController::ClientFinishCharacterSelection_Implementation()
{
    if(CharacterSelectionWidget)
    {
        CharacterSelectionWidget->RemoveFromParent();
        CharacterSelectionWidget = nullptr;
    }

    bShowMouseCursor = false;

    UWidgetBlueprintLibrary::SetInputMode_GameOnly(this);
}
