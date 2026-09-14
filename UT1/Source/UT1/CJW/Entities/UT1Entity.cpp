// Fill out your copyright notice in the Description page of Project Settings.


#include "CJW/Entities/UT1Entity.h"

// Sets default values
AUT1Entity::AUT1Entity()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AUT1Entity::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AUT1Entity::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AUT1Entity::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

