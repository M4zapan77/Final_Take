#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "StudioLanguageSubsystem.generated.h"
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FStudioLanguageChanged,const FString&,Language);

/** Local UI preference. FText/locres refreshes presentation without restarting a scene. */
UCLASS()
class MYPROJECT_API UStudioLanguageSubsystem:public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    UFUNCTION(BlueprintCallable,Category="Studio|Language") bool SetLanguage(const FString& Language,bool bSave=true);
    UFUNCTION(BlueprintPure,Category="Studio|Language") FString GetLanguage() const;
    UPROPERTY(BlueprintAssignable,Category="Studio|Language") FStudioLanguageChanged OnLanguageChanged;
    UFUNCTION(BlueprintPure,Category="Studio|Language",meta=(WorldContext="Context")) static UStudioLanguageSubsystem* Find(const UObject* Context);
    UFUNCTION(BlueprintPure,Category="Studio|Language") static FText ActingEventName(FName Identifier);
private:
    FString OriginalEditorCulture;
    FString OriginalPreviewCulture;
    bool bOriginalPreview=false;
};
