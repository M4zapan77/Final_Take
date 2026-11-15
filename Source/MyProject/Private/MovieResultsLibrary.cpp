#include "MovieResultsLibrary.h"

int32 UMovieResultsLibrary::GetStarsForScore(double Score)
{
    if (!FMath::IsFinite(Score)) { return 1; }
    if (Score>=90.0) { return 5; }
    if (Score>=75.0) { return 4; }
    if (Score>=50.0) { return 3; }
    if (Score>=25.0) { return 2; }
    return 1;
}

FMovieResults UMovieResultsLibrary::EvaluateMovie(const TArray<FMoviePerformance>& Performances, const TArray<FMovieRoleDefinition>& Definitions,
    const FMovieScoreSummary& Scores, const FMovieResultsSettings& Settings)
{
    FMovieResults Result;
    if (Performances.Num()!=3 || Scores.RoleScores.Num()!=3 || !FMath::IsFinite(Scores.FinalMovieScore)) { return Result; }
    TSet<FName> SeenRoles;
    int32 TotalScore=0, TotalObjectives=0, CompletedConfigured=0;
    for (const auto& Performance : Performances)
    {
        const auto* SavedScore=Scores.RoleScores.FindByPredicate([&](const FMovieRoleScore& Score) { return Score.RoleId==Performance.RoleId; });
        if (Performance.RoleId.IsNone() || SeenRoles.Contains(Performance.RoleId) || !SavedScore || !Performance.Recording.IsValid()
            || Performance.ObjectiveResult.Score<0 || Performance.ObjectiveResult.Score>100 || SavedScore->Score!=Performance.ObjectiveResult.Score) { return FMovieResults(); }
        SeenRoles.Add(Performance.RoleId); TotalScore+=SavedScore->Score;
        auto& RoleResult=Result.Roles.AddDefaulted_GetRef();
        RoleResult.RoleId=Performance.RoleId; RoleResult.Score=SavedScore->Score;
        const auto* Definition=Definitions.FindByPredicate([&](const FMovieRoleDefinition& Role) { return Role.RoleId==Performance.RoleId; });
        RoleResult.RoleName=Definition && !Definition->DisplayName.IsEmpty()?Definition->DisplayName:FText::FromName(Performance.RoleId);
        TSet<FName> Configured;
        if (Definition)
        {
            for (const auto& Objective : Definition->Objectives)
            {
                if (Objective.Id.IsNone() || Configured.Contains(Objective.Id)) { continue; }
                Configured.Add(Objective.Id);
                if (Performance.ObjectiveResult.CompletedObjectiveIds.Contains(Objective.Id)) { ++CompletedConfigured; }
            }
        }
        RoleResult.ObjectiveCount=Configured.Num(); TotalObjectives+=Configured.Num();
        for (FName Id : Performance.ObjectiveResult.CompletedObjectiveIds)
        {
            if (Id.IsNone() || RoleResult.CompletedObjectiveIds.Contains(Id)) { continue; }
            RoleResult.CompletedObjectiveIds.Add(Id);
            const FMovieObjectiveDefinition* Objective=Definition?Definition->Objectives.FindByPredicate([Id](const FMovieObjectiveDefinition& Item) { return Item.Id==Id; }):nullptr;
            RoleResult.CompletedObjectiveNames.Add(Objective && !Objective->DisplayName.IsEmpty()?Objective->DisplayName:FText::FromName(Id));
        }
        TSet<FName> Sounds, Tags;
        for (const auto& Event : Performance.Recording.Events)
        {
            if (Event.Type==EActingEventType::Gesture && Event.Identifier==TEXT("Dance")) { ++RoleResult.DanceEvents; }
            if (Event.Type==EActingEventType::Sound && !Event.Identifier.IsNone()) { Sounds.Add(Event.Identifier); }
        }
        for (const auto& Item : Performance.Recording.Appearance.Items)
        { for (FName Tag : Item.CosmeticTags) { if (!Tag.IsNone()) { Tags.Add(Tag); } } }
        RoleResult.DistinctSoundIds=Sounds.Array(); RoleResult.CostumeTags=Tags.Array();
        RoleResult.DistinctSoundIds.Sort(FNameLexicalLess()); RoleResult.CostumeTags.Sort(FNameLexicalLess());
    }
    if(Scores.bUseStoryScoring)
    {
        const auto& S=Scores.StoryScore;
        if(S.Story<0 || S.Story>40 || S.Interactions<0 || S.Interactions>30 || S.Creativity<0 || S.Creativity>20 || S.PropsAndNPCs<0 || S.PropsAndNPCs>10
            || !FMath::IsNearlyEqual(Scores.FinalMovieScore,static_cast<double>(S.Total()),1.e-8)){return FMovieResults();}
    }
    else if (!FMath::IsNearlyEqual(Scores.FinalMovieScore,TotalScore/3.0,1.e-8)) { return FMovieResults(); }
    Result.bAvailable=true; Result.MovieTitle=Settings.MovieTitle; Result.Tagline=Settings.Tagline;
    Result.FinalScore=Scores.FinalMovieScore; Result.Stars=GetStarsForScore(Result.FinalScore);
    const FMovieResultsSettings Defaults;
    const auto* Rating=Settings.RatingTexts.FindByPredicate([&](const FMovieRatingText& Item) { return Item.Stars==Result.Stars; });
    if (!Rating) { Rating=&Defaults.RatingTexts[Result.Stars-1]; }
    Result.RatingName=Rating->Name; Result.AudienceReview=Rating->AudienceReview;
    const auto AddAward=[&](EMovieAwardType Type, const TArray<FName>& Winners, int32 Statistic, bool bWholeMovie=false)
    {
        if (Winners.IsEmpty()) { return; }
        const auto* Style=Settings.AwardStyles.FindByPredicate([Type](const FMovieAwardStyle& Item) { return Item.Type==Type; });
        if (!Style) { Style=&Defaults.AwardStyles[static_cast<int32>(Type)]; }
        auto& Award=Result.Awards.AddDefaulted_GetRef(); Award.Type=Type; Award.Name=Style->Name; Award.Description=Style->Description;
        Award.Icon=Style->Icon; Award.WinnerRoleIds=Winners; Award.Statistic=Statistic; Award.bWholeMovie=bWholeMovie;
    };
    const auto Leaders=[&](auto Statistic, bool bRequirePositive, EMovieAwardType Type)
    {
        int32 Maximum=0; for (const auto& RoleResult : Result.Roles) { Maximum=FMath::Max(Maximum,Statistic(RoleResult)); }
        if (bRequirePositive && Maximum==0) { return; }
        TArray<FName> Winners; for (const auto& RoleResult : Result.Roles) { if (Statistic(RoleResult)==Maximum) { Winners.Add(RoleResult.RoleId); } }
        AddAward(Type,Winners,Maximum);
    };
    Leaders([](const FMovieRoleResult& RoleResult) { return RoleResult.Score; },false,EMovieAwardType::BestActor);
    Result.bAllObjectivesCompleted=TotalObjectives>0 && CompletedConfigured==TotalObjectives;
    if (Result.bAllObjectivesCompleted)
    {
        TArray<FName> Cast; for (const auto& RoleResult : Result.Roles) { Cast.Add(RoleResult.RoleId); }
        AddAward(EMovieAwardType::DirectorsCut,Cast,TotalObjectives,true);
    }
    Leaders([](const FMovieRoleResult& RoleResult) { return RoleResult.DanceEvents; },true,EMovieAwardType::DancingLegend);
    Leaders([](const FMovieRoleResult& RoleResult) { return RoleResult.DistinctSoundIds.Num(); },true,EMovieAwardType::SoundChaos);
    TArray<FName> CostumeWinners;
    for (const auto& RoleResult : Result.Roles)
    {
        for (const auto& Rule : Settings.CostumeDisasterRules)
        {
            if (Rule.Id.IsNone() || Rule.RequiredTags.IsEmpty()) { continue; }
            bool bMatches=true;
            for (FName Tag : Rule.RequiredTags) { bMatches &= !Tag.IsNone() && RoleResult.CostumeTags.Contains(Tag); }
            for (FName Tag : Rule.ForbiddenTags) { if (!Tag.IsNone() && RoleResult.CostumeTags.Contains(Tag)) { bMatches=false; } }
            if (bMatches) { CostumeWinners.AddUnique(RoleResult.RoleId); break; }
        }
    }
    AddAward(EMovieAwardType::CostumeDisaster,CostumeWinners,CostumeWinners.Num());
    return Result;
}
