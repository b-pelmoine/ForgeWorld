// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "FlyingCamera.generated.h"

UCLASS()
class SANDBOX2018_API AFlyingCamera : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	AFlyingCamera();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	class UPhysicsHandleComponent* m_handle;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	class UCameraComponent* m_cam;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FVector MovementInput;
	FVector2D CameraInput;
	float ZoomFactor;
	bool bZoomingIn;

	UPROPERTY(EditAnywhere)
	float CameraBaseSpeed;
	UPROPERTY(EditAnywhere)
	float CameraFastSpeedMultiplier;
	float SpeedFactor;
	float bFastSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float grabDistance;
	float holdDistance;
	UPROPERTY(EditAnywhere)
	float throwForce;
	float grabTime;
	AActor* grabbedActor;
	UPROPERTY(BlueprintReadOnly)
	TArray<FVector> trajectoryPoints;
	UPROPERTY(EditAnywhere)
	unsigned int trajectoryLength;
	UPROPERTY(EditAnywhere)
	float trajectoryTimeInterpolation;

	void MoveForward(float axisValue);
	void MoveRight(float axisValue);
	void MoveUp(float AxisValue);
	void PitchCamera(float axisValue);
	void YawCamera(float axisValue);
	void ChangeSpeed(float axisValue);
	void ZoomIn();
	void ZoomOut();
	void ActivateFastSpeed();
	void DeactivateFastSpeed();
	void Grabbing();
	void Releasing();
	UFUNCTION(BlueprintImplementableEvent)
	void OnGrab(AActor* actor);
	void Grab();
	UFUNCTION(BlueprintImplementableEvent)
	void OnRelease(AActor* actor);
	void Release();
	UFUNCTION(BlueprintImplementableEvent)
	void OnPushRelease(AActor* actor);
	void PushRelease();
	UFUNCTION(BlueprintImplementableEvent)
	void OnSweepHit(FVector Offset, int index);

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	
	
};
