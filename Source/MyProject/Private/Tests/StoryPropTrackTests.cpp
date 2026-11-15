#if WITH_DEV_AUTOMATION_TESTS
#include "StorySessionComponent.h"
#include "Misc/AutomationTest.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStoryPropTrajectoryTest,"OneManStudio.PhysicalProps.CompactChronologicalTrajectory",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FStoryPropTrajectoryTest::RunTest(const FString& Parameters)
{
    FStoryPropTrack Track;const FTransform Start(FVector(0,0,20)),End(FVector(100,0,20));
    Track.Observe(0,Start);for(int32 Index=1;Index<=100;++Index){Track.Observe(Index*.05,Start);}
    TestEqual(TEXT("Five seconds stationary stores one pose"),Track.Samples.Num(),1);
    Track.Observe(5.05,End);FTransform Value;
    TestTrue(TEXT("Evaluates before motion"),Track.Evaluate(4,Value));TestTrue(TEXT("No early movement from compacted idle"),Value.Equals(Start,.001));
    TestEqual(TEXT("Retains hold boundary and moving pose"),Track.Samples.Num(),3);
    Track.Evaluate(5.025,Value);TestTrue(TEXT("Recorded motion interpolates"),Value.GetLocation().Equals(FVector(50,0,20),.001));
    Track.Observe(5.05,FTransform(FVector(120,0,20)),true);TestEqual(TEXT("Equal-time ownership boundary does not duplicate timestamp"),Track.Samples.Num(),3);
    Track.Observe(4,Start);TestEqual(TEXT("Out-of-order samples rejected"),Track.Samples.Num(),3);
    Track.Observe(6,FTransform(FVector(120,0,20)),true);Track.Evaluate(99,Value);TestTrue(TEXT("Final pose clamps without physics"),Value.GetLocation().Equals(FVector(120,0,20),.001));
    return true;
}
#endif
