#include "StoryWorldTypes.h"
bool FStoryWorldState::HasUniqueMoney() const
{
    if(MoneyObjectId.IsNone()){return false;}
    int32 Count=0;TSet<FName> Ids;
    for(const auto& Object:Objects){if(Object.ObjectId.IsNone() || Ids.Contains(Object.ObjectId)){return false;}Ids.Add(Object.ObjectId);if(Object.ObjectId==MoneyObjectId){++Count;}}
    return Count==1;
}
EStoryOutcome FStoryWorldState::ResolveOutcome(FName RobberId,FName PoliceId,FName CivilianId) const
{
    if(!bMoneyWasTaken){return EStoryOutcome::FailedRobbery;}
    const FName Owner=MoneyOwner();
    if(bMoneySecured || Owner==PoliceId){return EStoryOutcome::PoliceRecovered;}
    if(Owner==CivilianId){return EStoryOutcome::CivilianBetrayal;}
    if(Owner==RobberId && bRobberEscaped){return EStoryOutcome::RobberEscaped;}
    return EStoryOutcome::Unresolved;
}
