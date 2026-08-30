#include "Core/W11ArenaBounds.h"

#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

float FW11ArenaBounds::ResolveHalfExtent(const int32 ChapterIndex, const int32 StageIndex)
{
	// Chapter 1 owns the expanded 3200 x 3200 floor. Authored later maps still
	// use the original 2000 x 2000 footprint until their individual art passes.
	return ChapterIndex == 1 || (ChapterIndex <= 0 && StageIndex == 1) ? 1500.0f : 950.0f;
}

FVector FW11ArenaBounds::ClampLocation(
	const FVector& Location,
	const float CapsuleRadius,
	const float HalfExtent)
{
	const float SafeExtent = FMath::Max(0.0f, HalfExtent - FMath::Max(0.0f, CapsuleRadius));
	return FVector(
		FMath::Clamp(Location.X, -SafeExtent, SafeExtent),
		FMath::Clamp(Location.Y, -SafeExtent, SafeExtent),
		Location.Z);
}

bool FW11ArenaBounds::ConstrainCharacter(ACharacter& Character, const float HalfExtent)
{
	const UCapsuleComponent* Capsule = Character.GetCapsuleComponent();
	const float CapsuleRadius = Capsule ? Capsule->GetScaledCapsuleRadius() : 0.0f;
	const FVector CurrentLocation = Character.GetActorLocation();
	const FVector ClampedLocation = ClampLocation(CurrentLocation, CapsuleRadius, HalfExtent);
	if (CurrentLocation.Equals(ClampedLocation, 0.01f))
	{
		return false;
	}

	Character.SetActorLocation(ClampedLocation, false, nullptr, ETeleportType::TeleportPhysics);
	if (UCharacterMovementComponent* Movement = Character.GetCharacterMovement())
	{
		FVector Velocity = Movement->Velocity;
		if (!FMath::IsNearlyEqual(CurrentLocation.X, ClampedLocation.X))
		{
			Velocity.X = 0.0f;
		}
		if (!FMath::IsNearlyEqual(CurrentLocation.Y, ClampedLocation.Y))
		{
			Velocity.Y = 0.0f;
		}
		Movement->Velocity = Velocity;
	}
	return true;
}
