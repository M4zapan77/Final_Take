#include "ActingTypes.h"

bool FActingEvent::HasValidPayload() const
{
    if (Identifier.IsNone() || InteractionLocation.ContainsNaN()) { return false; }
    switch (Type)
    {
    case EActingEventType::Gesture:
    case EActingEventType::Sound:
    case EActingEventType::Footstep:
    case EActingEventType::PresentationStop:
    case EActingEventType::StopGesture: return true;
    case EActingEventType::UseProp: return !PropId.IsNone();
    case EActingEventType::CollectProp:
        return !PropId.IsNone() && !TargetId.IsNone() && PropVisual.Id == PropId
            && PropVisual.Category == ECosmeticCategory::Prop && PropVisual.RelativeTransform.IsValid()
            && PropVisual.ActorRelativeTransform.IsValid() && !(PropVisual.StaticMesh && PropVisual.SkeletalMesh);
    default: return false;
    }
}

FName ActingIds::Gesture(EActingGesture Value)
{
    switch (Value)
    {
    case EActingGesture::Dance: return TEXT("Dance");
    case EActingGesture::Point: return TEXT("Point");
    case EActingGesture::Scared: return TEXT("Scared");
    case EActingGesture::Faint: return TEXT("Faint");
    case EActingGesture::Arrest: return TEXT("Arrest");
    default: return NAME_None;
    }
}

FName ActingIds::Sound(EActingSound Value)
{
    switch (Value)
    {
    case EActingSound::EvilLaugh: return TEXT("EvilLaugh");
    case EActingSound::Scream: return TEXT("Scream");
    case EActingSound::Cry: return TEXT("Cry");
    case EActingSound::PoliceSiren: return TEXT("PoliceSiren");
    case EActingSound::Kiss: return TEXT("Kiss");
    case EActingSound::Failure: return TEXT("Failure");
    default: return NAME_None;
    }
}
