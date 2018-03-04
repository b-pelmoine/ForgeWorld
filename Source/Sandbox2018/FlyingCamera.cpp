// Fill out your copyright notice in the Description page of Project Settings.

#include "FlyingCamera.h"

#include "Classes/GameFramework/SpringArmComponent.h"
#include "Classes/Camera/CameraComponent.h"
#include "Classes/Components/InputComponent.h"
#include "Classes/Engine/World.h"
#include "Classes/PhysicsEngine/PhysicsHandleComponent.h"
#include "Classes/Components/PrimitiveComponent.h"
#include "Classes/Components/LineBatchComponent.h"
#include "Classes/Kismet/GameplayStatics.h"
#include "Classes/Engine/CollisionProfile.h"
#include "Classes/Components/SphereComponent.h"

// Sets default values
AFlyingCamera::AFlyingCamera()
{
	PrimaryActorTick.bCanEverTick = true;
	
	USphereComponent* collider = CreateDefaultSubobject<USphereComponent>(TEXT("RootComponent"));
	RootComponent = collider;
	collider->InitSphereRadius(32.0f);
	collider->SetCollisionProfileName(UCollisionProfile::Pawn_ProfileName);

	m_cam = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	m_cam->SetupAttachment(RootComponent);
	CameraBaseSpeed = 800.0f;
	CameraFastSpeedMultiplier = 3.0f;

	m_handle = CreateDefaultSubobject<UPhysicsHandleComponent >(TEXT("PhysicsHandle"));
	grabTime = 0.0f;
	grabDistance = 5000.0f;
	holdDistance = 0.0f;
	grabbedActor = nullptr;
	throwForce = 5000.0f;
	trajectoryLength = 20;
	trajectoryTimeInterpolation = 0.1f;

	AutoPossessPlayer = EAutoReceiveInput::Player0;
}

// Called when the game starts or when spawned
void AFlyingCamera::BeginPlay()
{
	Super::BeginPlay();
	trajectoryPoints.Init(FVector::ZeroVector, trajectoryLength);
}

// Called every frame
void AFlyingCamera::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	{
		if (bZoomingIn)
		{
			ZoomFactor += DeltaTime / 0.1f;         //Zoom in over half a second
		}
		else
		{
			ZoomFactor -= DeltaTime / 0.25f;        //Zoom out over a quarter of a second
		}
		ZoomFactor = FMath::Clamp<float>(ZoomFactor, 0.0f, 1.0f);
		//Blend our camera's FOV and our SpringArm's length based on ZoomFactor
		m_cam->FieldOfView = FMath::Lerp<float>(90.0f, 30.0f, ZoomFactor);
		//m_arm->TargetArmLength = FMath::Lerp<float>(400.0f, 300.0f, ZoomFactor);
	}
	
	{
		FRotator NewRotation = GetActorRotation();
		NewRotation.Yaw += CameraInput.X;
		NewRotation.Pitch = FMath::Clamp(NewRotation.Pitch + CameraInput.Y, -89.5f, 89.5f);
		NewRotation.Roll = 0;
		SetActorRotation(NewRotation);
	}

	{
		if (bFastSpeed)
		{
			SpeedFactor += DeltaTime / 0.25f;         //Zoom in over half a second
		}
		else
		{
			SpeedFactor -= DeltaTime / 0.1f;        //Zoom out over a quarter of a second
		}
		SpeedFactor = FMath::Clamp<float>(SpeedFactor, 1.0f, CameraFastSpeedMultiplier);
		if (!MovementInput.IsZero())
		{
			MovementInput = MovementInput.GetSafeNormal();

			FVector MovementOffset = FVector::ZeroVector;
			MovementOffset += m_cam->GetForwardVector() * MovementInput.X;
			MovementOffset += m_cam->GetRightVector() * MovementInput.Y;
			MovementOffset += m_cam->GetUpVector() * MovementInput.Z;

			FVector offset = FVector::ZeroVector;
			offset = MovementOffset.GetSafeNormal() * SpeedFactor * CameraBaseSpeed;
			//OnSweepHit(offset, 0);
			FHitResult hit;
			FCollisionQueryParams params;
			params.AddIgnoredActor(this);
			if (GetWorld()->SweepSingleByChannel(
				hit,
				GetActorLocation(),
				GetActorLocation() + (MovementOffset.GetSafeNormal() * 128.0f),
				FQuat::Identity,
				ECollisionChannel::ECC_Visibility,
				FCollisionShape::MakeSphere(32.0f),
				params
			))
			{
				FVector hitNorm = hit.Normal;
				offset = hitNorm * (1 - FMath::Clamp(FMath::Abs(FVector::DotProduct(hitNorm, MovementOffset.GetSafeNormal())), 0.0f, 1.0f)) * DeltaTime
				+ (FVector::CrossProduct(MovementOffset.RotateAngleAxis(90.0f, hitNorm), hitNorm)).GetSafeNormal() * SpeedFactor * CameraBaseSpeed;
				OnSweepHit(offset, 0);
			}
			
			if(!SetActorLocation(GetActorLocation() + offset * DeltaTime, true, &hit))
			{
				FVector hitNorm = hit.Normal;
				offset = hitNorm * (1 - FMath::Abs(FVector::DotProduct(hit.Normal, MovementOffset.GetSafeNormal())));
				OnSweepHit(offset, 1);

				if (!SetActorLocation(GetActorLocation() + offset * DeltaTime, true, &hit))
				{
					offset = hit.Normal *  ( 1 - FMath::Abs( FVector::DotProduct(hit.Normal, MovementOffset.GetSafeNormal())));
					OnSweepHit(offset, 2);
					SetActorLocation(GetActorLocation() + offset * DeltaTime, true, &hit);
				}
			}
		}
	}

	{
		const FVector handlePosition = GetActorLocation() + GetActorForwardVector() * holdDistance;
		m_handle->SetTargetLocation(handlePosition);
		if (grabbedActor != nullptr)
		{
			for (int32 Index = 0; Index != trajectoryPoints.Num(); ++Index)
			{
				const float time = (trajectoryTimeInterpolation * Index);
				const FVector positionAtTime = handlePosition
					+ (GetActorForwardVector() * throwForce * time
						+ (FVector(0, 0, grabbedActor->GetWorldSettings()->GetGravityZ()) *( time * time)) / 2);
				trajectoryPoints[Index] = positionAtTime;
			}
		}
	}

}

// Called to bind functionality to input
void AFlyingCamera::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	InputComponent->BindAction("ZoomIn", IE_Pressed, this, &AFlyingCamera::ZoomIn);
	InputComponent->BindAction("ZoomIn", IE_Released, this, &AFlyingCamera::ZoomOut);
	InputComponent->BindAction("MoveFast", IE_Pressed, this, &AFlyingCamera::ActivateFastSpeed);
	InputComponent->BindAction("MoveFast", IE_Released, this, &AFlyingCamera::DeactivateFastSpeed);
	InputComponent->BindAction("Grabbing", IE_Pressed, this, &AFlyingCamera::Grabbing);
	InputComponent->BindAction("Grabbing", IE_Released, this, &AFlyingCamera::Releasing);

	InputComponent->BindAxis("MoveForward", this, &AFlyingCamera::MoveForward);
	InputComponent->BindAxis("MoveRight", this, &AFlyingCamera::MoveRight);
	InputComponent->BindAxis("MoveUp", this, &AFlyingCamera::MoveUp);
	InputComponent->BindAxis("CameraPitch", this, &AFlyingCamera::PitchCamera);
	InputComponent->BindAxis("CameraYaw", this, &AFlyingCamera::YawCamera);
	InputComponent->BindAxis("ChangeSpeed", this, &AFlyingCamera::ChangeSpeed);
}

void AFlyingCamera::MoveForward(float AxisValue)
{
	MovementInput.X = FMath::Clamp<float>(AxisValue, -1.0f, 1.0f);
}

void AFlyingCamera::MoveRight(float AxisValue)
{
	MovementInput.Y = FMath::Clamp<float>(AxisValue, -1.0f, 1.0f);
}

void AFlyingCamera::MoveUp(float AxisValue)
{
	MovementInput.Z = FMath::Clamp<float>(AxisValue, -1.0f, 1.0f);
}

void AFlyingCamera::PitchCamera(float AxisValue)
{
	CameraInput.Y = AxisValue;
}

void AFlyingCamera::YawCamera(float AxisValue)
{
	CameraInput.X = AxisValue;
}

void AFlyingCamera::ChangeSpeed(float axisValue)
{
	if (m_handle->GetGrabbedComponent() != NULL)
		holdDistance = FMath::Clamp(holdDistance + axisValue * (.1f * holdDistance + 5.0f), 0.0f, 10000.0f);
	else
		CameraBaseSpeed = FMath::Clamp(CameraBaseSpeed + axisValue * (.1f * CameraBaseSpeed + 5.0f), 0.0f, 10000.0f);
}

void AFlyingCamera::ZoomIn()
{
	bZoomingIn = true;
}

void AFlyingCamera::ZoomOut()
{
	bZoomingIn = false;
}

void AFlyingCamera::ActivateFastSpeed()
{
	bFastSpeed = true;
}

void AFlyingCamera::DeactivateFastSpeed()
{
	bFastSpeed = false;
}

void AFlyingCamera::Grabbing()
{
	if (m_handle->GetGrabbedComponent() != NULL)
		PushRelease();
	else
		Grab();
}

void AFlyingCamera::Releasing()
{
	if (UGameplayStatics::GetRealTimeSeconds(GetWorld()) - grabTime > .25f && grabbedActor != nullptr)
	{
		Release();
	}
	else
	{
		OnGrab(grabbedActor);
	}
}

void AFlyingCamera::Grab()
{
	FCollisionQueryParams params;
	params.AddIgnoredActor(this);
	FHitResult hit;
	if (GetWorld()->LineTraceSingleByChannel(
		hit,
		GetActorLocation(),
		GetActorLocation() + GetActorForwardVector() * grabDistance,
		ECollisionChannel::ECC_PhysicsBody,
		params
	))
	{
		grabbedActor = hit.GetActor();
		UPrimitiveComponent* component = Cast<UPrimitiveComponent>(grabbedActor->GetRootComponent());
		if (component != nullptr)
		{
			const FName profile = component->GetCollisionProfileName();
			if (profile.IsEqual(UCollisionProfile::PhysicsActor_ProfileName) && grabbedActor->IsRootComponentMovable())
			{
				m_handle->GrabComponentAtLocation(Cast<UPrimitiveComponent>(grabbedActor->GetRootComponent()), TEXT(""), hit.Location);
				holdDistance = hit.Distance;
				grabTime = UGameplayStatics::GetRealTimeSeconds(GetWorld());
			}
			else
			{
				grabbedActor = nullptr;
			}
		}
	}
}

void AFlyingCamera::Release()
{
	OnRelease(grabbedActor);
	grabbedActor = nullptr;
	m_handle->ReleaseComponent();
}

void AFlyingCamera::PushRelease()
{
	UPrimitiveComponent* component = m_handle->GetGrabbedComponent();
	OnPushRelease(grabbedActor);
	grabbedActor = nullptr;
	m_handle->ReleaseComponent();
	component->SetPhysicsLinearVelocity(FVector::ZeroVector);
	component->AddImpulse(GetActorForwardVector() * throwForce, NAME_None, true);
}
