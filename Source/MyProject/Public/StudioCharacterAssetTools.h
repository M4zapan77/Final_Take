#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "StudioCharacterAssetTools.generated.h"
class USkeletalMesh;
class UStaticMesh;
class UAnimSequence;
class UMaterial;
class UMaterialInterface;

/** Editor build helpers; generated assets stay in the customization folder, source art is read-only. */
UCLASS()
class MYPROJECT_API UStudioCharacterAssetTools : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="Studio|Editor")
    static bool ExportArtThumbnail(UObject* Asset,const FString& Filename,float OrbitYaw=-45.f);
    UFUNCTION(BlueprintCallable,Category="Studio|Editor")
    static bool ExportOutfitThumbnail(USkeletalMesh* Mesh,UMaterialInterface* Material,const FString& Filename);
    UFUNCTION(BlueprintPure,Category="Studio|Editor")
    static FTransform ReferenceBoneTransform(USkeletalMesh* Mesh,FName Bone,const FTransform& MeshToActor);
    UFUNCTION(BlueprintCallable, Category="Studio|Editor")
    static bool NormalizeArtMesh(UStaticMesh* Mesh,float LongestDimension=100.f,int32 MaximumTriangles=60000);
    UFUNCTION(BlueprintCallable, Category = "Studio|Editor")
    static bool ResetCustomizationMaterial(UMaterial* Material);
    UFUNCTION(BlueprintCallable, Category = "Studio|Editor")
    static FString DescribeMesh(USkeletalMesh* Mesh);
    UFUNCTION(BlueprintCallable, Category = "Studio|Editor")
    static TArray<UObject*> RetargetAnimations(const TArray<UObject*>& Assets, USkeletalMesh* Source,
                                               USkeletalMesh* Target, UObject* Retargeter, const FString& Destination);
    UFUNCTION(BlueprintCallable, Category = "Studio|Editor")
    static UStaticMesh* BuildFaceSurface(USkeletalMesh* Mesh, const FString& PackagePath);
    UFUNCTION(BlueprintCallable, Category = "Studio|Editor")
    static UObject* CreateLocomotionBlueprint(USkeletalMesh* Mesh, UAnimSequence* Idle, UAnimSequence* Walk,
                                              UAnimSequence* Run, UAnimSequence* Jump, UAnimSequence* Fall,
                                              const FString& PackagePath);
    UFUNCTION(BlueprintCallable, Category = "Studio|Editor")
    static UAnimSequence* CreatePlaceholderGesture(USkeletalMesh* Mesh, UAnimSequence* Template, FName Gesture,
                                                   const FString& PackagePath);
};
