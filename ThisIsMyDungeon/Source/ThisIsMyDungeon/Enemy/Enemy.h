// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Enemy.generated.h"


UCLASS()
class THISISMYDUNGEON_API AEnemy : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AEnemy();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	UPROPERTY(EditAnyWhere, BlueprintReadOnly, Category = "AI")
		class UBehaviorTree* TreeAsset;

	class UWidgetComponent* widgetComp = nullptr;

	UPROPERTY(EditAnywhere)
		FVector TreasureLoc;

	UPROPERTY(EditAnywhere, Category = "Life")
		int Health = 50;
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	void ApplyDamage(int Damage);

	FName GetClosestSocket(FVector pos);

	UFUNCTION(BlueprintImplementableEvent)
	void NewWidgetAnim(FVector pos, int Damage);
};
