#include "ActingRecording.h"

bool FActingRecording::AppendPose(TArray<FActingTransformSample>& Target,double Time,const FTransform& Transform,double& PendingHoldTime,bool Compact,bool Boundary)
{
    if(!FMath::IsFinite(Time) || Time<0 || !Transform.IsValid() || (!Target.IsEmpty() && Time<Target.Last().TimeSeconds)){return false;}
    if(!Target.IsEmpty())
    {
        auto& Last=Target.Last();
        // A manual stop can be a few ULPs after the fixed sample grid. Keep its
        // exact endpoint so an event recorded at that instant remains in range.
        if(FMath::IsNearlyEqual(Last.TimeSeconds,Time,1.e-8)){Last.TimeSeconds=Time;Last.WorldTransform=Transform;PendingHoldTime=Time;return true;}
        if(Compact && !Boundary && Last.WorldTransform.Equals(Transform,.0001)){PendingHoldTime=Time;return true;}
        if(Compact && PendingHoldTime>Last.TimeSeconds && PendingHoldTime<Time){auto Hold=Last;Hold.TimeSeconds=PendingHoldTime;Target.Add(Hold);}
    }
    auto& Sample=Target.AddDefaulted_GetRef();Sample.TimeSeconds=Time;Sample.WorldTransform=Transform;PendingHoldTime=Time;return true;
}

bool FActingRecording::IsValid() const
{
    if (Samples.IsEmpty() || Samples[0].TimeSeconds != 0.0 || !ActingSettings.MeshRelativeTransform.IsValid() || !Appearance.IsValid())
    {
        return false;
    }

    double PreviousTime = -1.0;
    for (const FActingTransformSample& Sample : Samples)
    {
        if (!FMath::IsFinite(Sample.TimeSeconds) || Sample.TimeSeconds <= PreviousTime
            || !Sample.WorldTransform.IsValid())
        {
            return false;
        }
        PreviousTime = Sample.TimeSeconds;
    }
    PreviousTime = 0.0;
    for (const FActingEvent& Event : Events)
    {
        if (!FMath::IsFinite(Event.TimeSeconds) || Event.TimeSeconds < PreviousTime
            || Event.TimeSeconds > GetDurationSeconds() || !Event.HasValidPayload())
        {
            return false;
        }
        PreviousTime = Event.TimeSeconds;
    }
    return true;
}

double FActingRecording::GetDurationSeconds() const
{
    return Samples.IsEmpty() ? 0.0 : Samples.Last().TimeSeconds;
}

FTransform FActingRecording::InterpolateTransforms(const FTransform& From, const FTransform& To, double Alpha)
{
    const double ClampedAlpha = FMath::Clamp(Alpha, 0.0, 1.0);
    return FTransform(
        FQuat::Slerp(From.GetRotation(), To.GetRotation(), ClampedAlpha).GetNormalized(),
        FMath::Lerp(From.GetLocation(), To.GetLocation(), ClampedAlpha),
        FMath::Lerp(From.GetScale3D(), To.GetScale3D(), ClampedAlpha));
}

bool FActingRecording::Evaluate(double TimeSeconds, FTransform& OutTransform) const
{
    if (Samples.IsEmpty() || !FMath::IsFinite(TimeSeconds))
    {
        return false;
    }
    if (TimeSeconds <= Samples[0].TimeSeconds)
    {
        OutTransform = Samples[0].WorldTransform;
        return true;
    }
    if (TimeSeconds >= Samples.Last().TimeSeconds)
    {
        OutTransform = Samples.Last().WorldTransform;
        return true;
    }

    // Binary search also supports clock seeks without a stale playback cursor.
    int32 Lower = 0;
    int32 Upper = Samples.Num() - 1;
    while (Upper - Lower > 1)
    {
        const int32 Middle = Lower + (Upper - Lower) / 2;
        if (Samples[Middle].TimeSeconds <= TimeSeconds)
        {
            Lower = Middle;
        }
        else
        {
            Upper = Middle;
        }
    }

    const double Alpha = (TimeSeconds - Samples[Lower].TimeSeconds)
        / (Samples[Upper].TimeSeconds - Samples[Lower].TimeSeconds);
    OutTransform = InterpolateTransforms(Samples[Lower].WorldTransform, Samples[Upper].WorldTransform, Alpha);
    return true;
}
