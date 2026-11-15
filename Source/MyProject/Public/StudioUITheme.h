#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Components/Button.h"
#include "Fonts/SlateFontInfo.h"
#include "StudioUITheme.generated.h"

class UFontFace;
class UTexture2D;

/** Approved UI art only. Editing this Data Asset changes native screens without C++. */
UCLASS(BlueprintType)
class MYPROJECT_API UStudioUITheme : public UDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Art") TMap<FName,TObjectPtr<UTexture2D>> Art;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Fonts") TObjectPtr<UFontFace> HeadingFace;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Palette") FLinearColor Navy=FLinearColor(.009,.014,.035,1);
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Palette") FLinearColor Gold=FLinearColor(.95,.64,.13,1);
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Palette") FLinearColor Cream=FLinearColor(.93,.85,.65,1);
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Palette") FLinearColor Red=FLinearColor(.72,.045,.055,1);
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Hub",meta=(MultiLine=true)) FText HubDescription;
    /** Optional supplied movie cover. The approved brand logo is used when no cover was supplied. */
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Hub") TObjectPtr<UTexture2D> MovieCover;
    UFUNCTION(BlueprintPure,Category="UI") UTexture2D* Texture(FName Id) const;
    UFUNCTION(BlueprintPure,Category="UI") FSlateFontInfo Heading(int32 Size=22) const;
    UFUNCTION(BlueprintPure,Category="UI") FSlateBrush Brush(FName Id,FMargin Margin) const;
    UFUNCTION(BlueprintCallable,Category="UI") void StyleButton(UButton* Button,FName Kind=NAME_None,bool bSelected=false) const;
    static UStudioUITheme* LoadDefault();
private:
    mutable TSharedPtr<const FCompositeFont> HeadingFamily;
};

/** Slate already selects the supplied hover/pressed art. Tiny feedback needs no per-frame UI tick. */
UCLASS()
class MYPROJECT_API UStudioArtButton : public UButton
{
    GENERATED_BODY()
public:
    UStudioArtButton();
private:
    void ScaleVisual(float Scale);
    UFUNCTION() void Hover();
    UFUNCTION() void Leave();
    UFUNCTION() void Press();
    UFUNCTION() void Release();
};
