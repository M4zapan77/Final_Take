#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ActingTypes.h"
#include "StudioFlowTypes.h"
#include "MovieResultsTypes.h"
#include "StudioAudioProfile.generated.h"
class UActingComponent;

/** One editable set of supplied assets. Global loops never enter performance data. */
UCLASS(BlueprintType)
class MYPROJECT_API UStudioAudioProfile : public UDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Global") FStudioFlowSettings FlowAudio;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Global") FMoviePremiereSettings PremiereAudio;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Performance") TArray<FActingSoundDefinition> Soundboard;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Performance") TArray<FActingSoundDefinition> Footsteps;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Performance") TObjectPtr<USoundAttenuation> ActorAttenuation;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Performance") TArray<TObjectPtr<USoundBase>> FaintImpacts;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Props") TArray<TObjectPtr<USoundBase>> MoneyPickups;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Props") TObjectPtr<USoundBase> DonutBite;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Global") TObjectPtr<USoundBase> Applause;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Alternates") TArray<TObjectPtr<USoundBase>> AlternateStings;
    void ConfigureActing(UActingComponent* Acting) const;
    void ConfigureFlow(FStudioFlowSettings& Settings) const;
    void ConfigurePremiere(FMoviePremiereSettings& Settings) const;
    void ConfigureCatalog(TArray<FCosmeticDefinition>& Catalog) const;
};
