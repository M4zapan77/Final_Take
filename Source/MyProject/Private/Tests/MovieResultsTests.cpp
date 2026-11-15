#if WITH_DEV_AUTOMATION_TESTS
#include "MovieResultsLibrary.h"
#include "Misc/AutomationTest.h"
#include <limits>

namespace MovieResultsTests
{
    struct FFilm
    {
        TArray<FMoviePerformance> Performances;
        TArray<FMovieRoleDefinition> Roles;
        FMovieScoreSummary Scores;
        FMovieResultsSettings Settings;
        FFilm()
        {
            const FName Ids[]={TEXT("Robber"),TEXT("PoliceOfficer"),TEXT("Customer")};
            for (int32 Index=0;Index<3;++Index)
            {
                auto& RoleDefinition=Roles.AddDefaulted_GetRef(); RoleDefinition.RoleId=Ids[Index]; RoleDefinition.DisplayName=FText::FromName(Ids[Index]);
                auto& Objective=RoleDefinition.Objectives.AddDefaulted_GetRef();
                Objective.Id=FName(*(Ids[Index].ToString()+TEXT(".Challenge"))); Objective.DisplayName=FText::FromString(TEXT("Complete the acting challenge")); Objective.Points=100;
                auto& Performance=Performances.AddDefaulted_GetRef(); Performance.RoleId=Ids[Index];
                Performance.Recording.Samples.AddDefaulted(); Performance.Recording.Samples.AddDefaulted_GetRef().TimeSeconds=1.0;
                Performance.ObjectiveResult.Score=100; Performance.ObjectiveResult.CompletedObjectiveIds.Add(Objective.Id);
            }
            UpdateScores();
        }
        void UpdateScores()
        {
            Scores=FMovieScoreSummary();
            for (const auto& Performance:Performances) { auto& Item=Scores.RoleScores.AddDefaulted_GetRef(); Item.RoleId=Performance.RoleId; Item.Score=Performance.ObjectiveResult.Score; Scores.FinalMovieScore+=Item.Score/3.0; }
        }
        void Event(int32 Index,EActingEventType Type,FName Identifier)
        {
            auto& Item=Performances[Index].Recording.Events.AddDefaulted_GetRef(); Item.TimeSeconds=.1; Item.Type=Type; Item.Identifier=Identifier;
            if (Type==EActingEventType::UseProp) { Item.PropId=TEXT("Prop.Donut"); Item.PropVisual.UseGestureId=TEXT("Dance"); }
        }
        void ChickenUniform(int32 Index)
        {
            auto& Outfit=Performances[Index].Recording.Appearance; Outfit.bHasAppearance=true;
            Outfit.Selection.HeadId=TEXT("Head.Chicken"); Outfit.Selection.BodyId=TEXT("Body.Uniform");
            for (const auto& Item:WardrobeDefaults::MakeCatalog()) { if (Item.Id==Outfit.Selection.HeadId || Item.Id==Outfit.Selection.BodyId) { Outfit.Items.Add(Item); } }
        }
        FMovieResults Evaluate() const { return UMovieResultsLibrary::EvaluateMovie(Performances,Roles,Scores,Settings); }
    };
    const FMovieAward* Award(const FMovieResults& Results,EMovieAwardType Type)
    { return Results.Awards.FindByPredicate([Type](const FMovieAward& Item) { return Item.Type==Type; }); }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMovieRatingThresholdTest,"OneManStudio.Ratings.ScoreThresholds",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMovieRatingThresholdTest::RunTest(const FString& Parameters)
{
    const double Scores[]={0,24.999999,25,49.999999,50,74.999999,75,89.999999,90,100};
    const int32 Stars[]={1,1,2,2,3,3,4,4,5,5};
    for (int32 Index=0;Index<UE_ARRAY_COUNT(Scores);++Index)
    { TestEqual(*FString::Printf(TEXT("Unrounded score %.6f"),Scores[Index]),UMovieResultsLibrary::GetStarsForScore(Scores[Index]),Stars[Index]); }
    TestEqual(TEXT("Nonfinite input safely receives one star"),UMovieResultsLibrary::GetStarsForScore(std::numeric_limits<double>::quiet_NaN()),1);
    MovieResultsTests::FFilm Film; Film.Performances[0].ObjectiveResult.Score=89; Film.Performances[1].ObjectiveResult.Score=90; Film.Performances[2].ObjectiveResult.Score=90; Film.UpdateScores();
    const auto Result=Film.Evaluate();
    TestTrue(TEXT("Average retains fractions"),FMath::IsNearlyEqual(Result.FinalScore,269.0/3.0,1.e-8));
    TestEqual(TEXT("An average that rounds to 90 is still four stars"),Result.Stars,4);
    TestEqual(TEXT("Default label matches four stars"),Result.RatingName.ToString(),FString(TEXT("Box Office Hit")));
    TestFalse(TEXT("Audience review supplied"),Result.AudienceReview.IsEmpty());
    Film.Scores.FinalMovieScore=100;
    TestFalse(TEXT("Inconsistent retained scores are rejected"),Film.Evaluate().bAvailable);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMovieAwardsTieTest,"OneManStudio.Ratings.AwardsAndTies",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMovieAwardsTieTest::RunTest(const FString& Parameters)
{
    MovieResultsTests::FFilm Film;
    for (int32 Count=0;Count<3;++Count) { Film.Event(0,EActingEventType::Gesture,TEXT("Dance")); Film.Event(1,EActingEventType::Gesture,TEXT("Dance")); }
    Film.Event(2,EActingEventType::Gesture,TEXT("Dance"));
    for (int32 Count=0;Count<4;++Count) { Film.Event(0,EActingEventType::Sound,TEXT("EvilLaugh")); }
    Film.Event(0,EActingEventType::Sound,TEXT("Scream")); Film.Event(1,EActingEventType::Sound,TEXT("PoliceSiren")); Film.Event(1,EActingEventType::Sound,TEXT("Cry"));
    Film.ChickenUniform(2);
    const auto Result=Film.Evaluate();
    TestTrue(TEXT("Retained film is evaluable"),Result.bAvailable);
    TestEqual(TEXT("All five conditions produce awards"),Result.Awards.Num(),5);
    const auto* Best=MovieResultsTests::Award(Result,EMovieAwardType::BestActor);
    const auto* Dance=MovieResultsTests::Award(Result,EMovieAwardType::DancingLegend);
    const auto* Sound=MovieResultsTests::Award(Result,EMovieAwardType::SoundChaos);
    const auto* Costume=MovieResultsTests::Award(Result,EMovieAwardType::CostumeDisaster);
    const auto* Director=MovieResultsTests::Award(Result,EMovieAwardType::DirectorsCut);
    if (!TestNotNull(TEXT("Best Actor"),Best) || !TestNotNull(TEXT("Dance"),Dance) || !TestNotNull(TEXT("Sound"),Sound) || !TestNotNull(TEXT("Costume"),Costume) || !TestNotNull(TEXT("Director"),Director)) { return false; }
    TestEqual(TEXT("All highest scoring roles share Best Actor"),Best->WinnerRoleIds.Num(),3);
    TestTrue(TEXT("Dance tie includes both leaders"),Dance->WinnerRoleIds.Num()==2 && Dance->Statistic==3 && Dance->WinnerRoleIds.Contains(TEXT("Robber")) && Dance->WinnerRoleIds.Contains(TEXT("PoliceOfficer")));
    TestTrue(TEXT("Repeated sounds do not increase distinct count; tied leaders both win"),Sound->WinnerRoleIds.Num()==2 && Sound->Statistic==2);
    TestTrue(TEXT("Costume award uses the recorded combination"),Costume->WinnerRoleIds.Num()==1 && Costume->WinnerRoleIds[0]==TEXT("Customer"));
    TestTrue(TEXT("Director's Cut is for the complete movie"),Result.bAllObjectivesCompleted && Director->bWholeMovie && Director->Statistic==3);
    TestEqual(TEXT("Completed objective display name retained"),Result.Roles[0].CompletedObjectiveNames[0].ToString(),FString(TEXT("Complete the acting challenge")));
    Film.Performances[2].Recording.Appearance.Items.Reset();
    TestEqual(TEXT("Already calculated award data is a value snapshot"),Costume->WinnerRoleIds.Num(),1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMovieConditionalAwardTest,"OneManStudio.Ratings.ConditionalAwards",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMovieConditionalAwardTest::RunTest(const FString& Parameters)
{
    MovieResultsTests::FFilm Film;
    for(int32 Role=0;Role<3;++Role){for(int32 Step=0;Step<5;++Step){Film.Event(Role,EActingEventType::Footstep,FName(*FString::Printf(TEXT("Footstep.%d"),Step)));}}
    const auto WithSteps=Film.Evaluate();
    TestTrue(TEXT("Recorded footsteps do not create soundboard awards"),WithSteps.bAvailable && MovieResultsTests::Award(WithSteps,EMovieAwardType::SoundChaos)==nullptr);
    for(auto& Performance:Film.Performances){Performance.Recording.Events.Reset();}
    for (int32 Index=0;Index<3;++Index) { Film.Roles[Index].Objectives.Reset(); Film.Performances[Index].ObjectiveResult.Score=0; Film.Performances[Index].ObjectiveResult.CompletedObjectiveIds.Reset(); }
    Film.Event(0,EActingEventType::StopGesture,TEXT("Dance")); Film.Event(1,EActingEventType::UseProp,TEXT("Use.Donut")); Film.UpdateScores();
    auto Result=Film.Evaluate();
    TestTrue(TEXT("Zero-objective movie is available and one star"),Result.bAvailable && Result.Stars==1 && Result.FinalScore==0);
    TestEqual(TEXT("No dance/sound/director/costume awards without their conditions"),Result.Awards.Num(),1);
    TestEqual(TEXT("All zero-score ties are represented fairly in Best Actor"),Result.Awards[0].WinnerRoleIds.Num(),3);
    TestEqual(TEXT("Use-gesture presentation is not counted as another dance event"),Result.Roles[1].DanceEvents,0);
    Film.ChickenUniform(2); Film.Settings.CostumeDisasterRules[0].ForbiddenTags.Add(TEXT("Costume.OfficialUniform"));
    Result=Film.Evaluate(); TestTrue(TEXT("Forbidden costume tags prevent that optional award"),MovieResultsTests::Award(Result,EMovieAwardType::CostumeDisaster)==nullptr);
    for (int32 Index=0;Index<3;++Index)
    {
        auto& Objective=Film.Roles[Index].Objectives.AddDefaulted_GetRef(); Objective.Id=TEXT("OneChallenge"); Objective.Points=100;
        Film.Performances[Index].ObjectiveResult.Score=100;
    }
    Film.UpdateScores(); Result=Film.Evaluate();
    TestTrue(TEXT("High scores alone never imply all objectives were completed"),!Result.bAllObjectivesCompleted && MovieResultsTests::Award(Result,EMovieAwardType::DirectorsCut)==nullptr);
    Film.Performances[0].Recording.Samples.Reset();
    TestFalse(TEXT("Invalid recording cannot generate results or awards"),Film.Evaluate().bAvailable);
    return true;
}
#endif
