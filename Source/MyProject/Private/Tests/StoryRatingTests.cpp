#if WITH_DEV_AUTOMATION_TESTS
#include "MovieResultsLibrary.h"
#include "Misc/AutomationTest.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStoryRatingPolicyTest,"OneManStudio.Story.NarrativeScoreValidation",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FStoryRatingPolicyTest::RunTest(const FString& Parameters)
{
    TArray<FMoviePerformance> Performances;FMovieScoreSummary Summary;
    for(int32 Index=0;Index<3;++Index)
    {
        const FName Id(*FString::Printf(TEXT("Role%d"),Index));
        auto& Film=Performances.AddDefaulted_GetRef();Film.RoleId=Id;Film.ObjectiveResult.Score=Index==1?100:50;
        Film.Recording.Samples.AddDefaulted();Film.Recording.Samples.AddDefaulted_GetRef().TimeSeconds=1;
        auto& Role=Summary.RoleScores.AddDefaulted_GetRef();Role.RoleId=Id;Role.Score=Film.ObjectiveResult.Score;
    }
    Summary.bUseStoryScoring=true;Summary.StoryScore.Story=40;Summary.StoryScore.Interactions=20;Summary.StoryScore.Creativity=4;Summary.StoryScore.PropsAndNPCs=6;Summary.FinalMovieScore=70;
    const FMovieResults Movie=UMovieResultsLibrary::EvaluateMovie(Performances,{},Summary,FMovieResultsSettings());
    TestTrue(TEXT("Narrative categories produce available results independently of role average"),Movie.bAvailable && Movie.FinalScore==70 && Movie.Stars==3);
    Summary.bUseStoryScoring=false;TestFalse(TEXT("Legacy still rejects mismatched role average"),UMovieResultsLibrary::EvaluateMovie(Performances,{},Summary,FMovieResultsSettings()).bAvailable);
    Summary.bUseStoryScoring=true;Summary.StoryScore.Story=41;Summary.FinalMovieScore=71;
    TestFalse(TEXT("Narrative categories cannot exceed their transparent caps"),UMovieResultsLibrary::EvaluateMovie(Performances,{},Summary,FMovieResultsSettings()).bAvailable);
    Summary.StoryScore.Story=40;Summary.StoryScore.Interactions=30;Summary.StoryScore.Creativity=20;Summary.StoryScore.PropsAndNPCs=0;Summary.FinalMovieScore=90;
    TestEqual(TEXT("Exact unrounded 90-point story is five stars"),UMovieResultsLibrary::EvaluateMovie(Performances,{},Summary,FMovieResultsSettings()).Stars,5);
    return true;
}
#endif
