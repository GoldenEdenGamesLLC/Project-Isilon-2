// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CharSelectionPlayerController.h"
#include "CharacterSelectionGameMode.generated.h"

/**
 * 
 */
UCLASS()
class PROJECT_ISILON_2_API ACharacterSelectionGameMode : public AGameModeBase
{
	GENERATED_BODY()
	
public:
	ACharacterSelectionGameMode();

	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;

	virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;
	
	void SpawnSelectedCharacter(ACharSelectionPlayerController* Player, EPlayableCharacter PlayerChar);

protected:
	UPROPERTY(EditDefaultsOnly, Category="Playable Characters")
	TSubclassOf<APawn> BarbarianClass;

	UPROPERTY(EditDefaultsOnly, Category="Playable Characters")
	TSubclassOf<APawn> WizardClass;
	
	UPROPERTY(EditDefaultsOnly, Category="Playable Characters")
	TSubclassOf<APawn> RangerClass;
};
