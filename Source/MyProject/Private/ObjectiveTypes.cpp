#include "ObjectiveTypes.h"

TArray<FMovieObjectiveDefinition> MovieObjectiveDefaults::BankHeist(int32 RoleIndex, FName RobberRole, FName PoliceRole)
{
    TArray<FMovieObjectiveDefinition> Result;
    const auto Add = [&](FName Id, const FText& Label, EMovieObjectiveType Type, int32 Points) -> FMovieObjectiveDefinition&
    {
        FMovieObjectiveDefinition& Item = Result.AddDefaulted_GetRef();
        Item.Id = Id; Item.DisplayName = Label; Item.Description = Item.DisplayName;
        Item.Type = Type; Item.Points = Points;
        return Item;
    };
    if (RoleIndex == 0)
    {
        Add(TEXT("Robber.EnterBank"), NSLOCTEXT("ObjectiveText","Robber.EnterBank","Enter the bank"), EMovieObjectiveType::ReachZone, 25).TargetTag = TEXT("Bank.Entry");
        auto& Bag = Add(TEXT("Robber.CollectMoney"), NSLOCTEXT("ObjectiveText","Robber.CollectMoney","Collect the money bag"), EMovieObjectiveType::CollectProp, 25);
        Bag.RequiredPropId = TEXT("Prop.MoneyBag"); Bag.PrerequisiteId = TEXT("Robber.EnterBank");
        auto& Escape = Add(TEXT("Robber.Escape"), NSLOCTEXT("ObjectiveText","Robber.Escape","Reach the escape zone"), EMovieObjectiveType::ReachZone, 25);
        Escape.TargetTag = TEXT("Bank.Escape"); Escape.PrerequisiteId = TEXT("Robber.CollectMoney");
        auto& Dance = Add(TEXT("Robber.DanceWithMoney"), NSLOCTEXT("ObjectiveText","Robber.DanceWithMoney","Dance while holding the money bag"), EMovieObjectiveType::UseProp, 15);
        Dance.RequiredEventId = TEXT("Dance"); Dance.RequiredPropId = TEXT("Prop.MoneyBag"); Dance.PrerequisiteId = TEXT("Robber.CollectMoney");
        Add(TEXT("Robber.Laugh"), NSLOCTEXT("ObjectiveText","Robber.Laugh","Trigger EvilLaugh"), EMovieObjectiveType::PlaySound, 10).RequiredEventId = TEXT("EvilLaugh");
    }
    else if (RoleIndex == 1)
    {
        Add(TEXT("Police.EnterBank"), NSLOCTEXT("ObjectiveText","Police.EnterBank","Enter the bank"), EMovieObjectiveType::ReachZone, 25).TargetTag = TEXT("Bank.Entry");
        auto& Approach = Add(TEXT("Police.ApproachRobber"), NSLOCTEXT("ObjectiveText","Police.ApproachRobber","Approach the recorded Robber (200 units)"), EMovieObjectiveType::ReachActor, 25);
        Approach.TargetRoleId = RobberRole; Approach.MaxInteractionDistance = 200.0; Approach.PrerequisiteId = TEXT("Police.EnterBank");
        auto& Arrest = Add(TEXT("Police.Arrest"), NSLOCTEXT("ObjectiveText","Police.Arrest","Perform Arrest near the Robber (200 units)"), EMovieObjectiveType::ActionNearActor, 25);
        Arrest.TargetRoleId = RobberRole; Arrest.MaxInteractionDistance = 200.0; Arrest.RequiredEventId = TEXT("Arrest"); Arrest.PrerequisiteId = TEXT("Police.ApproachRobber");
        auto& Donut = Add(TEXT("Police.UseDonut"), NSLOCTEXT("ObjectiveText","Police.UseDonut","Use the Donut prop (E)"), EMovieObjectiveType::UseProp, 15);
        Donut.RequiredEventType = EActingEventType::UseProp; Donut.RequiredEventId = TEXT("Use.Donut"); Donut.RequiredPropId = TEXT("Prop.Donut");
        Add(TEXT("Police.Siren"), NSLOCTEXT("ObjectiveText","Police.Siren","Trigger PoliceSiren"), EMovieObjectiveType::PlaySound, 10).RequiredEventId = TEXT("PoliceSiren");
    }
    else if (RoleIndex == 2)
    {
        Add(TEXT("Customer.Counter"), NSLOCTEXT("ObjectiveText","Customer.Counter","Reach the bank counter"), EMovieObjectiveType::ReachZone, 25).TargetTag = TEXT("Bank.Counter");
        auto& Approach = Add(TEXT("Customer.ApproachPolice"), NSLOCTEXT("ObjectiveText","Customer.ApproachPolice","Approach the recorded Police Officer"), EMovieObjectiveType::ReachActor, 25);
        Approach.TargetRoleId = PoliceRole; Approach.MaxInteractionDistance = 200.0;
        auto& Scared = Add(TEXT("Customer.Scared"), NSLOCTEXT("ObjectiveText","Customer.Scared","Perform Scared near the Robber"), EMovieObjectiveType::ActionNearActor, 25);
        Scared.TargetRoleId = RobberRole; Scared.MaxInteractionDistance = 200.0; Scared.RequiredEventId = TEXT("Scared");
        auto& Dance = Add(TEXT("Customer.Dance"), NSLOCTEXT("ObjectiveText","Customer.Dance","Dance near another recorded character"), EMovieObjectiveType::ActionNearActor, 15);
        Dance.bAnyRecordedRole = true; Dance.MaxInteractionDistance = 200.0; Dance.RequiredEventId = TEXT("Dance");
        Add(TEXT("Customer.Scream"), NSLOCTEXT("ObjectiveText","Customer.Scream","Trigger Scream"), EMovieObjectiveType::PlaySound, 10).RequiredEventId = TEXT("Scream");
    }
    return Result;
}
