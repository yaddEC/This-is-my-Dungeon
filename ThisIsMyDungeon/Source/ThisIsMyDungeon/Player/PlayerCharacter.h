// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "PlayerCharacter.generated.h"

UCLASS()
class THISISMYDUNGEON_API APlayerCharacter : public ACharacter
{
private:
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FollowCamera;

	class UStaticMeshComponent* ProjectileStart;

	FTransform SpawnTransform;
	
	FTransform MeshRelativeTransform;

protected:
	virtual void BeginPlay() override;

	void MoveForward(float Value);

	void MoveRight(float Value);

	void TurnAtRate(float Rate);

	void LookUpAtRate(float Rate);

	void OnJump();

	void OnShoot();

	void ApplyDamage(int Damage);

	void Respawn();

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
		float BaseTurnRate;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
		float BaseLookUpRate;

	// Life
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Life)
		int MaxLife = 100;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Life)
		int CurrentLife = 0;

	// Power
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Power)
		int MaxPower = 2000;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Power)
		int CurrentPower = 0;

	UPROPERTY(EditAnywhere, Category = Power)
		int StartingPower = 800;

	// Projectiles
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		TSubclassOf<class AFireBall> ProjectileClass;

	APlayerCharacter();

	virtual void Tick(float DeltaTime) override;


};
