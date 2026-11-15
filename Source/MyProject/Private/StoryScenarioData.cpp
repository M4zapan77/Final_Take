#include "StoryScenarioData.h"

FStoryScenarioDefinition::FStoryScenarioDefinition()
{
    Name=NSLOCTEXT("StoryScenarios","BankName","The Perfect Robbery");
    Synopsis=NSLOCTEXT("StoryScenarios","BankSynopsis","A teller, a guard and two witnesses share the bank with three suspiciously familiar actors. Whoever holds the money changes the ending.");
    RoleOffsets={FVector(-180,0,0),FVector(20,250,0),FVector(20,-250,0)};
    NPCOffsets={FVector(-210,-565,0),FVector(420,220,0),FVector(-80,80,0),FVector(350,-150,0)};
    AvailablePropActions={TEXT("Grab"),TEXT("Drop"),TEXT("Throw")};
    CinematicPriorities={{EStoryEventType::Escape,110},{EStoryEventType::MoneySecured,110},{EStoryEventType::MoneyConfiscated,100},{EStoryEventType::Detain,100},{EStoryEventType::PropImpact,95},{EStoryEventType::PropThrown,90},{EStoryEventType::MoneyTaken,80},{EStoryEventType::MoneyTransferred,80},{EStoryEventType::GuardDistracted,75}};
}
bool FStoryScenarioDefinition::IsValid() const
{
    const auto ValidOffset=[](FVector Value,double Radius){return !Value.ContainsNaN() && Value.Size2D()<=Radius && FMath::Abs(Value.Z)<=300;};
    if(Id.IsNone() || RoleOffsets.Num()!=3 || NPCOffsets.Num()!=4 || !ValidOffset(SceneAnchorOffset,2000) || !ValidOffset(MoneyOffset,850) || !ValidOffset(ExitOffset,900) || !ValidOffset(SecureOffset,850) || !FMath::IsFinite(MoneySurfaceHeight) || MoneySurfaceHeight<0 || MoneySurfaceHeight>200){return false;}
    for(FVector Offset:RoleOffsets){if(!ValidOffset(Offset,850)){return false;}}
    for(FVector Offset:NPCOffsets){if(!ValidOffset(Offset,850)){return false;}}
    TSet<FName> Actions;for(FName Action:AvailablePropActions){if((Action!=TEXT("Grab") && Action!=TEXT("Drop") && Action!=TEXT("Throw")) || Actions.Contains(Action)){return false;}Actions.Add(Action);}
    for(const auto& Pair:CinematicPriorities){if(Pair.Value<0 || Pair.Value>200){return false;}}
    return true;
}
