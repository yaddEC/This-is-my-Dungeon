// Fill out your copyright notice in the Description page of Project Settings.

#include "PlayerCharacter.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "Materials/Material.h"
#include "../DebugString.hpp"
#include "DrawDebugHelpers.h"
#include "FireBall.h"
#include "NavigationData.h"
#include "Kismet/KismetMathLibrary.h"
#include "ThisIsMyDungeon/DungeonGameMode.h"
#include "DrawDebugHelpers.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem/Public/NavigationPath.h"
#include "Runtime/Engine/Classes/Kismet/GameplayStatics.h"
#include "Kismet/GameplayStatics.h"
#include "../GenericTrap.h"
#include "../Enemy/Enemy.h"
#include "Runtime/Engine/Public/TimerManager.h"

// Sets default values
APlayerCharacter::APlayerCharacter()
{
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->AirControl = 0.2f;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->AddLocalOffset(FVector(0, 0, 100));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 300.0f;
	CameraBoom->bUsePawnControlRotation = true;

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	ProjectileStart = nullptr;
	CurrentTrap = nullptr;
}

void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	auto children = GetMesh()->GetChildComponent(0);
	if (Cast<UStaticMeshComponent>(children))
		ProjectileStart = Cast<UStaticMeshComponent>(children);
	CurrentLife = MaxLife;
	CurrentPower = StartingPower;
	MeshRelativeTransform = GetMesh()->GetRelativeTransform();
	SpawnTransform = GetActorTransform();
	hit = FHitResult(ForceInit);
	//Get Treasure Location
	TArray<AActor*> treasure;
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), TEXT("Treasure"), treasure);
	if (treasure.Num() > 0)
	{
		TreasureLoc = treasure[0]->GetActorLocation();
	}
	//GetSpawnersLocation
	TSubclassOf<ASpawner> ClassToFind;
	ClassToFind = ASpawner::StaticClass();
	TArray<AActor*> TempSpawner;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ClassToFind, TempSpawner);

	if (TempSpawner.Num() > 0)
	{
		for (int i = 0; i <= TempSpawner.Num() - 1; i++)
		{
			SpawnLoc.Add(TempSpawner[i]->GetActorLocation());
		}
	}
	PlayerSpawn = GetActorLocation();

}

void APlayerCharacter::OnJump()
{
}

int APlayerCharacter::GetCurrentTrapIndex()
{
	return CurrentTrap->Index;
}

void APlayerCharacter::OnShoot()
{
	if (!ProjectileStart || !ProjectileClass)
		return;

	// Raycast Point to find hit point.
	FHitResult Hit;

	RaycastFromCamera(&Hit, 600000);
	if (Hit.bBlockingHit) {
		GetWorld()->SpawnActor<AFireBall>(ProjectileClass, ProjectileStart->GetComponentLocation(), (Hit.Location - ProjectileStart->GetComponentLocation()).ToOrientationRotator());
	}
	else
		GetWorld()->SpawnActor<AFireBall>(ProjectileClass, ProjectileStart->GetComponentLocation(), FollowCamera->GetForwardVector().ToOrientationRotator());
}

void APlayerCharacter::ApplyDamage(int Damage)
{
	CurrentLife -= Damage;
	if (CurrentLife <= 0)
	{
		CurrentLife = 0;

		// Enable Ragdoll
		UCapsuleComponent* CapsuleComp = GetCapsuleComponent();
		DisableInput(Cast<APlayerController>(GetController()));
		GetMesh()->SetAllBodiesSimulatePhysics(true);

		FTimerHandle _;
		GetWorldTimerManager().SetTimer(_, this, &APlayerCharacter::Respawn, 5.f, false);
	}
}

void APlayerCharacter::AddPower(int add)
{
	CurrentPower += add;
}

void APlayerCharacter::Respawn()
{
	// Disable Ragdoll
	EnableInput(Cast<APlayerController>(GetController()));
	GetMesh()->SetAllBodiesSimulatePhysics(false);
	GetMesh()->AttachTo(GetCapsuleComponent());
	GetMesh()->SetRelativeLocationAndRotation(MeshRelativeTransform.GetLocation(), MeshRelativeTransform.GetRotation());
	this->SetActorTransform(SpawnTransform);
	CurrentLife = MaxLife;
}

void APlayerCharacter::StartWave()
{
	ADungeonGameMode* GM = Cast<ADungeonGameMode>(UGameplayStatics::GetGameMode(this));

	GM->StartWaveGM();
}

void APlayerCharacter::MoveForward(float Value)
{
	if ((Controller != nullptr) && (Value != 0.0f))
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void APlayerCharacter::MoveRight(float Value)
{
	if ((Controller != nullptr) && (Value != 0.0f))
	{
		// find out which way is right
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get right vector 
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		// add movement in that direction
		AddMovementInput(Direction, Value);
	}
}

void APlayerCharacter::TurnAtRate(float Rate)
{
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void APlayerCharacter::LookUpAtRate(float Rate)
{
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

// Called every frame
void APlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (CurrentTrap && !CurrentTrap->Placed)
	{
		if (RaycastFromCamera(&hit))
		{
			FVector snappedPos = hit.Location;
			float wholePart;
			float gridSize = modff(CurrentTrap->size.X / 100.f, &wholePart) * 100.f;


			if (fabsf(hit.Normal.X) < 0.9f)
			{
				snappedPos.X -= modff(snappedPos.X / 100.f, &wholePart) * 100.f;
				snappedPos.X += hit.Location.X > 0 ? gridSize : -gridSize;
			}
			if (fabsf(hit.Normal.Y) < 0.9f)
			{
				snappedPos.Y -= modff(snappedPos.Y / 100.f, &wholePart) * 100.f;
				snappedPos.Y += hit.Location.Y > 0 ? gridSize : -gridSize;
			}
			if (fabsf(hit.Normal.Z) < 0.9f)
			{
				snappedPos.Z -= modff(snappedPos.Z / 100.f, &wholePart) * 100.f;
				snappedPos.Z += hit.Location.Z > 0 ? gridSize : -gridSize;
			}


			// TO CHANGE
			FVector res = snappedPos + hit.Normal * CurrentTrap->size.Z;
			CurrentTrap->SetActorLocation(res);
			FRotator rot = FRotationMatrix::MakeFromZ(hit.Normal).Rotator();
			CurrentTrap->SetActorRotation(rot);
		}
		else
		{
			CurrentTrap->SetActorLocation(FVector(0,0,0));
		}
		

	}
	this->SetActorRotation(UKismetMathLibrary::RInterpTo(GetActorRotation(), FRotator::MakeFromEuler(FVector(GetActorRotation().Euler().X, GetActorRotation().Euler().Y, FollowCamera->GetComponentRotation().Euler().Z)), DeltaTime, 5.f)); 

	if (_currentTime >= 5.f)
	{
		AddPower(10);
		_currentTime = 0;
	}
	_currentTime += DeltaTime;
}


// Called to bind functionality to input
void APlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	check(PlayerInputComponent);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAction("Shoot", IE_Pressed, this, &APlayerCharacter::OnShoot);

	PlayerInputComponent->BindAction("SetUpTrap", IE_Pressed, this, &APlayerCharacter::OnTrapSetUp);
	PlayerInputComponent->BindAction("CancelTrap", IE_Pressed, this, &APlayerCharacter::OnCancelTrap);

	PlayerInputComponent->BindAction("ChooseTrap1", IE_Pressed, this, &APlayerCharacter::OnTrap1);
	PlayerInputComponent->BindAction("ChooseTrap2", IE_Pressed, this, &APlayerCharacter::OnTrap2);
	PlayerInputComponent->BindAction("ChooseTrap3", IE_Pressed, this, &APlayerCharacter::OnTrap3);
	PlayerInputComponent->BindAction("ChooseTrap4", IE_Pressed, this, &APlayerCharacter::OnTrap4);

	PlayerInputComponent->BindAction("StartWave", IE_Pressed, this, &APlayerCharacter::StartWave);

	PlayerInputComponent->BindAxis("MoveForward", this, &APlayerCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &APlayerCharacter::MoveRight);

	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &APlayerCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &APlayerCharacter::LookUpAtRate);

}

bool APlayerCharacter::RaycastFromCamera(FHitResult* RV_Hit, float MaxDistance)
{
	if (Controller == NULL) { return false; }

	FCollisionQueryParams RV_TraceParams = FCollisionQueryParams(FName(TEXT("RV_Trace")), true, this);
	RV_TraceParams.bTraceComplex = true;
	RV_TraceParams.bReturnPhysicalMaterial = true;

	RV_TraceParams.AddIgnoredActor(this);

	FVector pos;
	FVector dir;
	FVector2D screenSize;
	GetWorld()->GetGameViewport()->GetViewportSize(screenSize);
	screenSize = screenSize / 2;
	Cast<APlayerController>(this->GetController())->DeprojectScreenPositionToWorld(screenSize.X, screenSize.Y, pos, dir);

	// Raycast Point to find hit point.
	FHitResult Hit;
	auto StartLocation = pos;
	auto EndLocation = StartLocation + (FollowCamera->GetForwardVector() * MaxDistance);
	if (GetWorld()->LineTraceSingleByChannel(*RV_Hit, StartLocation, EndLocation, ECollisionChannel::ECC_WorldStatic, RV_TraceParams))
	{
		if (AEnemy* Character = Cast<AEnemy>(RV_Hit->Actor))
		{
			Debug("Enemy");
			return false;
		}
		else
		{
			return true;
		}
		}
	return false;
	}
	

void APlayerCharacter::OnTrap1()
{
	SelectTrap(0);
}

void APlayerCharacter::OnTrap2()
{
	SelectTrap(1);
}

void APlayerCharacter::OnTrap3()
{
	SelectTrap(2);
}

void APlayerCharacter::OnTrap4()
{
	SelectTrap(3);
}

void APlayerCharacter::SelectTrap(int index)
{
	if (TrapsBlueprint.Num() <= index)
		return;
	if (CurrentTrap)
		CurrentTrap->Destroy();
	if (!TrapsBlueprint[index])
	{
		DebugError("Missing Trap Blueprint %d", index);
		return;
	}

	CurrentTrap = Cast<AGenericTrap>(GetWorld()->SpawnActor(TrapsBlueprint[index]));
	if (CurrentTrap)
		Debug("Choose trap %d", index + 1);
}
void APlayerCharacter::CheckPath()
{
	for (int i = 0; i <= SpawnLoc.Num() - 1; i++)
	{
		UNavigationPath* NavPath = UNavigationSystemV1::FindPathToLocationSynchronously(GetWorld(), TreasureLoc, SpawnLoc[i], NULL);
		if (NavPath)
		{
			if (NavPath->IsPartial())
			{
				lastTrap->Destroy();
				CurrentPower += lastTrap->Cost;
				Debug("Blocked");
			}
		}
	}
	
}

void APlayerCharacter::OnTrapSetUp()
{
	FVector normal = hit.Normal;
	//Debug("%.2f, %.2f, %.2f", normal.X, normal.Y, normal.Z);
	if (!CurrentTrap) return;
	if (!CurrentTrap->CanBePlaced) return;

	if (normal == FVector(0, 0, 0))
	{
		// raycast hit nothing
		Debug("None");

		return;
	}

	//CurrentPower -= CurrentTrap->Cost;
	if (normal.Z > 0.9f)
	{
		// raycast hit the ground
		Debug("Ground");
		lastTrap = GetWorld()->SpawnActor<AGenericTrap>(CurrentTrap->GetClass(), CurrentTrap->GetActorLocation(), CurrentTrap->GetActorRotation());
		FTimerHandle Handle;
		lastTrap->SetUp();
		GetWorld()->GetTimerManager().SetTimer(Handle, this, &APlayerCharacter::CheckPath, 0.1f);
		
	}
	else
	{
		// raycast hit a wall
		Debug("Wall");
		auto trap = GetWorld()->SpawnActor<AGenericTrap>(CurrentTrap->GetClass(), CurrentTrap->GetActorLocation(), CurrentTrap->GetActorRotation());
		trap->SetUp();
	}
	CurrentTrap->Destroy();
	CurrentTrap = nullptr;
}

void APlayerCharacter::OnCancelTrap()
{
	if (!CurrentTrap) return;
	CurrentTrap->Destroy();
	CurrentTrap = nullptr;
}

