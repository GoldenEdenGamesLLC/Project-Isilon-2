// Fill out your copyright notice in the Description page of Project Settings.


#include "CharacterSelectionGameMode.h"
#include "CharSelectionPlayerController.h"

ACharacterSelectionGameMode::ACharacterSelectionGameMode()
{
    PlayerControllerClass = ACharSelectionPlayerController::StaticClass();

    DefaultPawnClass = nullptr;
}

void ACharacterSelectionGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{

}

UClass* ACharacterSelectionGameMode::GetDefaultPawnClassForController_Implementation(AController* InController)
{
    ACharSelectionPlayerController* Player = Cast<ACharSelectionPlayerController>(InController);

    if(!Player || !Player->bHasSelectedCharacter)
    {
        return nullptr;
    }

    switch(Player->SelectedCharacter)
    {
        case EPlayableCharacter::Barbarian:
            return BarbarianClass.Get();
        case EPlayableCharacter::Wizard:
            return WizardClass.Get();
        case EPlayableCharacter::Ranger:
            return RangerClass.Get();
        default:
            return nullptr;
    }
}

void ACharacterSelectionGameMode::SpawnSelectedCharacter(ACharSelectionPlayerController* Player, EPlayableCharacter PlayerChar)
{
    if(!HasAuthority() || !IsValid(Player))
    {
        return;
    }

    if(Player->bHasSelectedCharacter)
    {
        return;
    }

    TSubclassOf<APawn> ClassToSpawn;

    switch(PlayerChar)
    {
        case EPlayableCharacter::Barbarian:
            ClassToSpawn = BarbarianClass;
            break;
        case EPlayableCharacter::Wizard:
            ClassToSpawn = WizardClass;
            break;
        case EPlayableCharacter::Ranger:
            ClassToSpawn = RangerClass;
            break;
        default:
            return;
    }

    if(!ClassToSpawn)
    {
        UE_LOG(LogTemp, Error, TEXT("[CHARACTER SELECTION] No Blueprint class configured."));
        return;
    }

    Player->SelectedCharacter = PlayerChar;
    Player->bHasSelectedCharacter = true;

    RestartPlayer(Player);

    if(Player->GetPawn())
    {
        Player->ClientFinishCharacterSelection();
        UE_LOG(LogTemp, Warning, TEXT("[CHARACTER SELECTION] Spawned: %s"), *GetNameSafe(Player->GetPawn()));
    }
    else
    {
        Player->bHasSelectedCharacter = false;
        UE_LOG(LogTemp, Error, TEXT("[CHARACTER SELECTION] Failed to spawn selected pawn."));
    }
}