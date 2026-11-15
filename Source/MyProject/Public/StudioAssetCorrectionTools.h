#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "StudioAssetCorrectionTools.generated.h"
class UStaticMesh;
class UPrimitiveComponent;

/** Editor-only authoring helpers; originals remain untouched. */
UCLASS()
class MYPROJECT_API UStudioAssetCorrectionTools : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="Studio|Editor")
    static FString DescribeStaticMesh(UStaticMesh* Mesh);
    UFUNCTION(BlueprintCallable, Category="Studio|Editor")
    static UStaticMesh* CreateCorrectedMesh(UStaticMesh* Source,const TArray<FVector>& Positions,const FString& PackagePath);
    UFUNCTION(BlueprintCallable, Category="Studio|Editor")
    static FVector TraceSurface(UPrimitiveComponent* Surface,FVector Start,FVector End);
};
