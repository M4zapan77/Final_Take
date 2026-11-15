#include "MovieResultsTypes.h"

FMovieResultsSettings::FMovieResultsSettings()
{
    MovieTitle = NSLOCTEXT("MovieResults", "DefaultTitle", "THE WORST BANK HEIST");
    Tagline = NSLOCTEXT("MovieResults", "Tagline", "One actor. Three roles. No budget.");
    const FText Names[] = {NSLOCTEXT("FinalTakeUI","MovieResultsTypes.Names.87f728c6db","Cinematic Disaster"), NSLOCTEXT("FinalTakeUI","MovieResultsTypes.Names.3df1f5354c","So Bad It's Good"), NSLOCTEXT("FinalTakeUI","MovieResultsTypes.Names.a05884f104","Cult Classic"), NSLOCTEXT("FinalTakeUI","MovieResultsTypes.Names.78495d48a2","Box Office Hit"), NSLOCTEXT("FinalTakeUI","MovieResultsTypes.Names.99d1185feb","Masterpiece")};
    const FText Reviews[] = {
        NSLOCTEXT("FinalTakeUI","MovieResultsTypes.Reviews.c68afe625f","The audience asked for a refund. Admission was free."),
        NSLOCTEXT("FinalTakeUI","MovieResultsTypes.Reviews.607343385f","The plot escaped. We respect its decision."),
        NSLOCTEXT("FinalTakeUI","MovieResultsTypes.Reviews.98d1b10680","It makes no sense. We are watching it again."),
        NSLOCTEXT("FinalTakeUI","MovieResultsTypes.Reviews.58fed0bf71","A hit! The budget was recovered in imaginary money."),
        NSLOCTEXT("FinalTakeUI","MovieResultsTypes.Reviews.0191a7cb19","One actor. Three legends. Absolutely no budget.")};
    for (int32 Index=0; Index<5; ++Index)
    {
        auto& Text = RatingTexts.AddDefaulted_GetRef();
        Text.Stars = Index+1; Text.Name = Names[Index]; Text.AudienceReview = Reviews[Index];
    }
    const FText AwardNames[] = {NSLOCTEXT("FinalTakeUI","MovieResultsTypes.AwardNames.5d77e3f247","Best Actor"), NSLOCTEXT("FinalTakeUI","MovieResultsTypes.AwardNames.8db29e0d61","Director's Cut"), NSLOCTEXT("FinalTakeUI","MovieResultsTypes.AwardNames.fd4aeb565a","Dancing Legend"), NSLOCTEXT("FinalTakeUI","MovieResultsTypes.AwardNames.7424b26e9d","Sound Chaos"), NSLOCTEXT("FinalTakeUI","MovieResultsTypes.AwardNames.db3f3db03a","Costume Disaster")};
    const FText Descriptions[] = {NSLOCTEXT("FinalTakeUI","MovieResultsTypes.Descriptions.fdccd253d9","The strongest performance. The competition looked strangely familiar."),
        NSLOCTEXT("FinalTakeUI","MovieResultsTypes.Descriptions.0eee26b266","Every acting challenge completed. We can finally afford the word 'vision'."),
        NSLOCTEXT("FinalTakeUI","MovieResultsTypes.Descriptions.0c96993300","The most recorded dances. The floor has filed a complaint."),
        NSLOCTEXT("FinalTakeUI","MovieResultsTypes.Descriptions.6934517c64","The widest soundboard range. Subtlety was unavailable."),
        NSLOCTEXT("FinalTakeUI","MovieResultsTypes.Descriptions.be727e30e2","A costume combination the wardrobe department refuses to explain.")};
    for (int32 Index=0; Index<5; ++Index)
    {
        auto& Style = AwardStyles.AddDefaulted_GetRef(); Style.Type = static_cast<EMovieAwardType>(Index);
        Style.Name = AwardNames[Index]; Style.Description = Descriptions[Index];
    }
    auto& Rule = CostumeDisasterRules.AddDefaulted_GetRef();
    Rule.Id = TEXT("ChickenOnDuty"); Rule.RequiredTags = {TEXT("Costume.AnimalHead"), TEXT("Costume.OfficialUniform")};
}

bool FMoviePremiereSettings::IsValid() const
{
    return FMath::IsFinite(TitleFadeInSeconds) && TitleFadeInSeconds>=0
        && FMath::IsFinite(TitleHoldSeconds) && TitleHoldSeconds>=0
        && FMath::IsFinite(TitleFadeOutSeconds) && TitleFadeOutSeconds>=0
        && FMath::IsFinite(GetTitleDuration())
        && FMath::IsFinite(ResultsFadeInSeconds) && ResultsFadeInSeconds>=0
        && FMath::IsFinite(LetterboxHeightFraction) && LetterboxHeightFraction>=0 && LetterboxHeightFraction<=.25f
        && FMath::IsFinite(MusicVolume) && MusicVolume>=0 && MusicVolume<=2
        && FMath::IsFinite(EffectsVolume) && EffectsVolume>=0 && EffectsVolume<=2;
}
