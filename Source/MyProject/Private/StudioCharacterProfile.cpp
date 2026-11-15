#include "StudioCharacterProfile.h"
#include "ActingComponent.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequenceBase.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/Character.h"

bool UStudioCharacterProfile::ApplyModel(ACharacter* Character) const
{
    if (!Character || !BodyMesh || !AnimationClass || !MeshTransform.IsValid())
    {
        return false;
    }
    USkeletalMeshComponent* Mesh = Character->GetMesh();
    if (!Mesh)
    {
        return false;
    }
    if (Mesh->GetSkeletalMeshAsset() == BodyMesh && Mesh->GetAnimClass() == AnimationClass)
    {
        return true;
    }
    Mesh->SetSkeletalMesh(BodyMesh);
    Mesh->EmptyOverrideMaterials();
    FTransform Fit = MeshTransform;
    if (bFitFeetToCapsule && Character->GetCapsuleComponent())
    {
        FVector Location = Fit.GetLocation();
        Location.Z = -Character->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() -
                     ReferenceFloorHeight * Fit.GetScale3D().Z;
        Fit.SetLocation(Location);
    }
    Mesh->SetRelativeTransform(Fit);
    Mesh->SetAnimInstanceClass(AnimationClass);
    Mesh->SetSimulatePhysics(false);
    if (SkinMaterial)
    {
        Mesh->SetMaterial(0, SkinMaterial);
    }
    return true;
}
void UStudioCharacterProfile::ConfigureActing(UActingComponent* Acting) const
{
    if (!Acting || !BodyMesh)
    {
        return;
    }
    for (FActingGestureDefinition& Gesture : Acting->Gestures)
    {
        if (Gesture.Animation && Gesture.Animation->GetSkeleton() != BodyMesh->GetSkeleton())
        {
            if (UAnimSequenceBase* const* Retargeted = RetargetedAnimations.Find(Gesture.Animation))
            {
                Gesture.Animation = *Retargeted;
            }
            else
            {
                Gesture.Animation = nullptr;
            }
        }
        if (Gesture.Montage && Gesture.Montage->GetSkeleton() != BodyMesh->GetSkeleton())
        {
            if (UAnimSequenceBase* const* Retargeted = RetargetedAnimations.Find(Gesture.Montage))
            {
                Gesture.Montage = Cast<UAnimMontage>(*Retargeted);
            }
            else
            {
                Gesture.Montage = nullptr;
            }
        }
        if (!Gesture.Animation && !Gesture.Montage)
        {
            if (const auto* Default = DefaultGestures.FindByPredicate(
                    [&](const auto& Item) { return Item.Identifier == Gesture.Identifier; }))
            {
                Gesture = *Default;
            }
        }
    }
}
void UStudioCharacterProfile::ConfigureCatalog(TArray<FCosmeticDefinition>& Catalog) const
{
    for (auto& Item : Catalog)
    {
        if (Item.Category == ECosmeticCategory::Body && SkinMaterial && !Item.StaticMesh && !Item.SkeletalMesh &&
            (Item.Materials.IsEmpty() || !Item.Materials[0] ||
             Item.Materials[0]->GetPathName().StartsWith(TEXT("/Engine/BasicShapes/"))))
        {
            Item.Materials = {SkinMaterial};
        }
        if (!Item.StaticMesh || !Item.StaticMesh->GetPathName().StartsWith(TEXT("/Engine/BasicShapes/")))
        {
            continue;
        }
        if (Item.Id == TEXT("Head.PoliceCap"))
        {
            Item.ActorRelativeTransform = FTransform(FQuat::Identity, FVector(0, 0, 90), FVector(.78, .78, .12));
        }
        else if (Item.Id == TEXT("Head.Crown"))
        {
            Item.ActorRelativeTransform = FTransform(FQuat::Identity, FVector(0, 0, 96), FVector(.70, .70, .26));
        }
        else if (Item.Id == TEXT("Head.RobberMask"))
        {
            Item.ActorRelativeTransform = FTransform(FQuat::Identity, FVector(29, 0, 37), FVector(.10, .62, .10));
        }
        else if (Item.Id == TEXT("Head.Chicken"))
        {
            Item.ActorRelativeTransform = FTransform(FQuat::Identity, FVector(0, 0, 65), FVector(.78, .78, .58));
        }
        else if (Item.Id == TEXT("Face.Glasses"))
        {
            Item.ActorRelativeTransform = FTransform(FQuat::Identity, FVector(31, 0, 46), FVector(.04, .53, .09));
        }
        else if (Item.Id == TEXT("Face.Mustache"))
        {
            Item.ActorRelativeTransform = FTransform(FQuat::Identity, FVector(31, 0, 30), FVector(.04, .28, .05));
        }
    }
}
