#include "ActingComponent.h"
#include "StudioAudioSubsystem.h"

#include "ActingRecorderComponent.h"
#include "ActingSceneClockSubsystem.h"
#include "PropInteractionComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Components/AudioComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "ReplayActor.h"
#include "Sound/SoundBase.h"

UActingComponent::UActingComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
    for (EActingGesture Gesture : {EActingGesture::Dance, EActingGesture::Point, EActingGesture::Scared, EActingGesture::Faint, EActingGesture::Arrest})
    {
        FActingGestureDefinition& Definition = Gestures.AddDefaulted_GetRef();
        Definition.Identifier = ActingIds::Gesture(Gesture);
        Definition.bLoop = Gesture == EActingGesture::Dance;
    }
    for (EActingSound Sound : {EActingSound::EvilLaugh, EActingSound::Scream, EActingSound::Cry, EActingSound::PoliceSiren, EActingSound::Kiss, EActingSound::Failure})
    {
        Sounds.AddDefaulted_GetRef().Identifier = ActingIds::Sound(Sound);
    }
}

USkeletalMeshComponent* UActingComponent::ResolveMesh() const
{
    if (IsValid(TargetMesh) && TargetMesh->GetOwner() == GetOwner()) { return TargetMesh; }
    if (const ACharacter* Character = Cast<ACharacter>(GetOwner())) { return Character->GetMesh(); }
    return GetOwner() ? GetOwner()->FindComponentByClass<USkeletalMeshComponent>() : nullptr;
}

FActingPlaybackSettings UActingComponent::CapturePlaybackSettings() const
{
    FActingPlaybackSettings Settings;
    Settings.Gestures = Gestures;
    Settings.Sounds = Sounds;
    Settings.Footsteps = Footsteps;
    Settings.DefaultAttenuation = DefaultAttenuation;
    if (USkeletalMeshComponent* Mesh = ResolveMesh())
    {
        Settings.SkeletalMesh = Mesh->GetSkeletalMeshAsset();
        Settings.AnimationClass=Mesh->GetAnimClass();
        Settings.MeshRelativeTransform = Mesh->GetComponentTransform().GetRelativeTransform(GetOwner()->GetActorTransform());
    }
    return Settings;
}

const FActingGestureDefinition* UActingComponent::FindGesture(FName Identifier) const
{
    const UActingRecorderComponent* Recorder = GetOwner() ? GetOwner()->FindComponentByClass<UActingRecorderComponent>() : nullptr;
    const TArray<FActingGestureDefinition>& Definitions = bReplayOnly ? ReplaySettings.Gestures
        : Recorder && Recorder->IsRecording() ? Recorder->GetRecordingData().ActingSettings.Gestures : Gestures;
    return Definitions.FindByPredicate([Identifier](const FActingGestureDefinition& Entry) { return Entry.Identifier == Identifier; });
}

const FActingSoundDefinition* UActingComponent::FindSound(FName Identifier) const
{
    const UActingRecorderComponent* Recorder = GetOwner() ? GetOwner()->FindComponentByClass<UActingRecorderComponent>() : nullptr;
    const TArray<FActingSoundDefinition>& Definitions = bReplayOnly ? ReplaySettings.Sounds
        : Recorder && Recorder->IsRecording() ? Recorder->GetRecordingData().ActingSettings.Sounds : Sounds;
    if (const auto* Found=Definitions.FindByPredicate([Identifier](const FActingSoundDefinition& Entry) { return Entry.Identifier == Identifier; })) { return Found; }
    const auto& Steps=bReplayOnly?ReplaySettings.Footsteps:Recorder && Recorder->IsRecording()?Recorder->GetRecordingData().ActingSettings.Footsteps:Footsteps;
    return Steps.FindByPredicate([Identifier](const FActingSoundDefinition& Entry) { return Entry.Identifier == Identifier; });
}

bool UActingComponent::CanTriggerActions() const
{
    if (!bLiveActionsEnabled || bReplayOnly || !IsValid(GetOwner()) || GetOwner()->IsA<AReplayActor>()) { return false; }
    const UActingRecorderComponent* Recorder = GetOwner()->FindComponentByClass<UActingRecorderComponent>();
    return !bRequireActiveRecording || (Recorder && Recorder->IsRecording());
}

void UActingComponent::SetLiveActionsEnabled(bool bEnabled)
{
    if (bLiveActionsEnabled == bEnabled) { return; }
    bLiveActionsEnabled = bEnabled;
    OnLiveActionsEnabledChanged.Broadcast(bEnabled);
}

double UActingComponent::GetLiveTime() const
{
    const UActingRecorderComponent* Recorder = GetOwner() ? GetOwner()->FindComponentByClass<UActingRecorderComponent>() : nullptr;
    if (Recorder && Recorder->IsRecording()) { return Recorder->GetRecordingElapsedSeconds(); }
    return GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
}

bool UActingComponent::TriggerGesture(EActingGesture Gesture) { return TriggerGestureById(ActingIds::Gesture(Gesture)); }
bool UActingComponent::TriggerSound(EActingSound Sound) { return TriggerSoundById(ActingIds::Sound(Sound)); }
bool UActingComponent::TriggerGestureById(FName Identifier) { return TriggerLive(EActingEventType::Gesture, Identifier); }
bool UActingComponent::TriggerSoundById(FName Identifier) { return TriggerLive(EActingEventType::Sound, Identifier); }
bool UActingComponent::StopGesture() { return !ActiveGestureId.IsNone() && TriggerLive(EActingEventType::StopGesture, ActiveGestureId); }

bool UActingComponent::TriggerLive(EActingEventType Type, FName Identifier)
{
    if (!CanTriggerActions() || Identifier.IsNone()
        || (Type == EActingEventType::Gesture && !FindGesture(Identifier))
        || ((Type == EActingEventType::Sound || Type == EActingEventType::Footstep) && !FindSound(Identifier))) { return false; }
    UActingRecorderComponent* Recorder = GetOwner()->FindComponentByClass<UActingRecorderComponent>();
    const double Now = GetLiveTime();
    // Append before callbacks/presentation, so listeners cannot record the same event again.
    if (Recorder && Recorder->IsRecording() && !Recorder->RecordActingEvent(Type, Identifier)) { return false; }
    FActingEvent Event;
    Event.TimeSeconds = Now;
    Event.Type = Type;
    Event.Identifier = Identifier;
    ExecuteEvent(Event, Now, false, true);
    return true;
}

void UActingComponent::ReportIssue(FName Identifier, const TCHAR* Message)
{
    OnPresentationIssue.Broadcast(Identifier, FText::FromString(Message));
}

bool UActingComponent::TriggerPropEvent(const FActingEvent& Event)
{
    if (!CanTriggerActions() || (Event.Type != EActingEventType::CollectProp && Event.Type != EActingEventType::UseProp)) { return false; }
    UActingRecorderComponent* Recorder = GetOwner()->FindComponentByClass<UActingRecorderComponent>();
    FActingEvent Recorded;
    if (!Recorder || !Recorder->RecordEvent(Event, Recorded)) { return false; }
    ExecuteEvent(Recorded, Recorded.TimeSeconds, false, true);
    return true;
}

void UActingComponent::ConfigureReplay(const FActingPlaybackSettings& Settings, USkeletalMeshComponent* Mesh)
{
    ResetPlaybackState();
    bReplayOnly = true;
    bLiveActionsEnabled = false;
    ReplaySettings = Settings;
    TargetMesh = Mesh;
    // ReplayActor supplies absolute scene time; no independent component clock is advanced.
    SetComponentTickEnabled(false);
}

void UActingComponent::DispatchRecordedEvent(const FActingEvent& Event, double CurrentTakeTime, bool bClockRunning)
{
    if (!bReplayOnly) { return; }
    ExecuteEvent(Event, CurrentTakeTime, true, bClockRunning);
}

void UActingComponent::PresentSceneGesture(FName Identifier,double SceneTime,bool bClockRunning)
{
    if(Identifier.IsNone() || !FindGesture(Identifier)){return;}
    bExternalScenePresentation=true;
    FActingEvent Event;Event.Type=EActingEventType::Gesture;Event.Identifier=Identifier;Event.TimeSeconds=SceneTime;
    ExecuteEvent(Event,SceneTime,true,bClockRunning);
}

void UActingComponent::UpdateScenePresentation(double SceneTime,bool bClockRunning)
{
    bExternalScenePresentation=true;UpdatePresentation(SceneTime,bClockRunning);
}

void UActingComponent::PlaySceneSound(USoundBase* Sound,double EventSceneTime,double CurrentSceneTime,bool bClockRunning,float Volume)
{
    if(!Sound){return;}
    const double Now=bReplayOnly || bExternalScenePresentation?CurrentSceneTime:GetLiveTime();
    FActingSoundDefinition Definition;Definition.Sound=Sound;Definition.VolumeMultiplier=Volume;
    PlayOwnedSound(Definition,Now-FMath::Max(0.0,CurrentSceneTime-EventSceneTime),Now,bClockRunning);
}

void UActingComponent::ExecuteEvent(const FActingEvent& Event, double Now, bool bReplay, bool bClockRunning)
{
    const uint32 Generation = ++ActionGeneration;
    LastTriggeredEvent = Event;
    ++DispatchedEventCount;
    if(Event.Type==EActingEventType::PresentationStop) { StopAllActions(); }
    else if (Event.Type == EActingEventType::Gesture)
    {
        StartGesture(Event, Now, bReplay);
    }
    else if (Event.Type == EActingEventType::StopGesture)
    {
        if (ActiveGestureId == Event.Identifier) { FinishGesture(true); }
    }
    else if (Event.Type == EActingEventType::Sound || Event.Type == EActingEventType::Footstep)
    {
        const FActingSoundDefinition* Definition = FindSound(Event.Identifier);
        if (!Definition || !Definition->Sound)
        {
            ReportIssue(Event.Identifier, TEXT("Assign a sound asset in ActingComponent > Sounds."));
        }
        else
        {
            PlayOwnedSound(*Definition,Event.TimeSeconds,Now,bClockRunning);
        }
    }
    else if (Event.Type == EActingEventType::CollectProp || Event.Type == EActingEventType::UseProp)
    {
        if (UPropInteractionComponent* Interaction = GetOwner()->FindComponentByClass<UPropInteractionComponent>()) { Interaction->ApplyEvent(Event, bReplay); }
        if (Generation == ActionGeneration)
        {
            FActingSoundDefinition Effect; Effect.Sound=Event.Type==EActingEventType::CollectProp?Event.PropVisual.PickupSound:Event.PropVisual.UseSound;
            Effect.VolumeMultiplier=Event.PropVisual.SoundVolume;
            PlayOwnedSound(Effect,Event.TimeSeconds,Now,bClockRunning);
        }
        if (Generation == ActionGeneration && Event.Type == EActingEventType::UseProp && !Event.PropVisual.UseGestureId.IsNone())
        {
            FActingEvent GestureEvent; GestureEvent.Type = EActingEventType::Gesture;
            GestureEvent.Identifier = Event.PropVisual.UseGestureId; GestureEvent.TimeSeconds = Event.TimeSeconds;
            StartGesture(GestureEvent, Now, bReplay);
        }
    }
    if (Generation != ActionGeneration || !IsValid(GetOwner())) { return; }
    OnActingEventTriggered.Broadcast(Event, bReplay);
    if (Generation != ActionGeneration || !IsValid(GetOwner())) { return; }
    OnActingEventTriggeredNative.Broadcast(Event, bReplay);
}

void UActingComponent::StartGesture(const FActingEvent& Event, double Now, bool bReplay)
{
    const uint32 Generation = ActionGeneration;
    const FActingGestureDefinition* Definition = FindGesture(Event.Identifier);
    if (!Definition) { ReportIssue(Event.Identifier, TEXT("This gesture identifier has no configured definition.")); return; }
    const FActingGestureDefinition DefinitionCopy = *Definition;
    FinishGesture(true);
    if (Generation != ActionGeneration || !IsValid(GetOwner())) { return; }
    CurrentGesture = DefinitionCopy;
    bGestureImpactPlayed=false;
    CurrentGesture.PlayRate = FMath::IsFinite(CurrentGesture.PlayRate) ? FMath::Max(0.01f, CurrentGesture.PlayRate) : 1.0f;
    const double Duration = FMath::IsFinite(CurrentGesture.DurationSeconds) ? FMath::Max(0.01f, CurrentGesture.DurationSeconds) : 3.0;
    GestureStartTime = Event.TimeSeconds;
    GestureEndTime = GestureStartTime + Duration;
    // Do not flash an already expired gesture during hitch catch-up.
    if (Now >= GestureEndTime) { return; }
    ActiveGestureId = Event.Identifier;
    USkeletalMeshComponent* Mesh = ResolveMesh();
    UAnimSequenceBase* Asset = CurrentGesture.Montage ? static_cast<UAnimSequenceBase*>(CurrentGesture.Montage.Get()) : CurrentGesture.Animation.Get();
    if (!Mesh || !Mesh->GetSkeletalMeshAsset() || !Asset)
    {
        ReportIssue(Event.Identifier, TEXT("Assign a skeletal mesh and compatible in-place animation/montage to display this gesture."));
        return;
    }
    if (!CurrentGesture.bLoop && Asset->GetPlayLength() > 0.0f)
    {
        GestureEndTime = FMath::Min(GestureEndTime, GestureStartTime + Asset->GetPlayLength() / CurrentGesture.PlayRate);
        if (Now >= GestureEndTime) { ActiveGestureId = NAME_None; return; }
    }
    const double Age = FMath::Max(0.0, Now - GestureStartTime);
    if (!bReplay && Mesh->GetAnimationMode() == EAnimationMode::AnimationBlueprint)
    {
        UAnimInstance* Instance = Mesh->GetAnimInstance();
        if (!Instance) { ReportIssue(Event.Identifier, TEXT("The live mesh needs an AnimInstance and a matching montage slot.")); return; }
        ControlledAnimInstance = Instance;
        PreviousRootMotionMode = Instance->RootMotionMode;
        bRootModeOverridden = true;
        Instance->SetRootMotionMode(ERootMotionMode::IgnoreRootMotion);
        if (CurrentGesture.Montage)
        {
            OwnedMontage = CurrentGesture.Montage;
            if (Instance->Montage_Play(OwnedMontage, CurrentGesture.PlayRate, EMontagePlayReturnType::MontageLength,
                static_cast<float>(Age * CurrentGesture.PlayRate), false) <= 0.0f)
            {
                ReportIssue(Event.Identifier, TEXT("The montage could not play. Check skeleton compatibility and the Anim Blueprint slot."));
            }
            if (CurrentGesture.bLoop && OwnedMontage->GetNumSections() > 0)
            {
                Instance->Montage_SetNextSection(OwnedMontage->GetSectionName(OwnedMontage->GetNumSections() - 1),
                    OwnedMontage->GetSectionName(0), OwnedMontage);
            }
        }
        else
        {
            OwnedMontage = Instance->PlaySlotAnimationAsDynamicMontage(CurrentGesture.Animation, CurrentGesture.SlotName,
                FMath::Max(0.0f, CurrentGesture.BlendInSeconds), 0.1f, CurrentGesture.PlayRate,
                CurrentGesture.bLoop ? MAX_int32 : 1, -1.0f, static_cast<float>(Age * CurrentGesture.PlayRate));
        }
    }
    else
    {
        PreviousAnimationMode = Mesh->GetAnimationMode();
        if (!bReplay || PreviousAnimationMode==EAnimationMode::AnimationBlueprint)
        {
            bRestoreSingleNode = true;
            if (UAnimSingleNodeInstance* Previous = Mesh->GetSingleNodeInstance())
            {
                PreviousSingleAsset = Previous->GetCurrentAsset();
                PreviousSingleTime = Previous->GetCurrentTime();
                PreviousSingleRate = Previous->GetPlayRate();
                bPreviousSinglePlaying = Previous->IsPlaying();
                bPreviousSingleLooping = Previous->IsLooping();
            }
        }
        Mesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
        Mesh->PlayAnimation(Asset, CurrentGesture.bLoop);
        if (UAnimSingleNodeInstance* Single = Mesh->GetSingleNodeInstance())
        {
            if (CurrentGesture.Montage && !CurrentGesture.Montage->SlotAnimTracks.IsEmpty())
            {
                Single->SetMontagePreviewSlot(CurrentGesture.Montage->SlotAnimTracks[0].SlotName);
            }
            ControlledAnimInstance = Single;
            PreviousRootMotionMode = Single->RootMotionMode;
            bRootModeOverridden = true;
            Single->SetRootMotionMode(ERootMotionMode::IgnoreRootMotion);
            // Absolute-time evaluation prevents drift and suppresses animation notifies on ghosts.
            Single->SetPlaying(false);
        }
    }
    UpdatePresentation(Now, true);
}

void UActingComponent::FinishGesture(bool bNotify)
{
    const FName FinishedId = ActiveGestureId;
    ActiveGestureId = NAME_None;
    if (UAnimInstance* Instance = ControlledAnimInstance.Get())
    {
        // End our montage before restoring root motion, including during its blend-out.
        if (OwnedMontage) { Instance->Montage_Stop(0.0f, OwnedMontage); }
        if (bRootModeOverridden) { Instance->SetRootMotionMode(static_cast<ERootMotionMode::Type>(PreviousRootMotionMode)); }
    }
    if (USkeletalMeshComponent* Mesh = ResolveMesh())
    {
        if (bRestoreSingleNode)
        {
            Mesh->SetAnimationMode(static_cast<EAnimationMode::Type>(PreviousAnimationMode));
            if (UAnimSingleNodeInstance* Single = Mesh->GetSingleNodeInstance())
            {
                Single->SetAnimationAsset(PreviousSingleAsset, bPreviousSingleLooping, PreviousSingleRate);
                Single->SetPosition(PreviousSingleTime, false);
                Single->SetPlaying(bPreviousSinglePlaying);
            }
        }
        else if (bReplayOnly)
        {
            // Return to the reference pose between gestures; replay locomotion is transform-driven.
            if (UAnimSingleNodeInstance* Single = Mesh->GetSingleNodeInstance()) { Single->SetAnimationAsset(nullptr); }
        }
    }
    bRestoreSingleNode = bRootModeOverridden = false;
    ControlledAnimInstance.Reset();
    OwnedMontage = nullptr;
    PreviousSingleAsset = nullptr;
    CurrentGesture = FActingGestureDefinition();
    if (bNotify && !FinishedId.IsNone()) { OnGestureEnded.Broadcast(FinishedId); }
}

void UActingComponent::UpdatePresentation(double Now, bool bClockRunning)
{
    if (!ActiveGestureId.IsNone())
    {
        if (!bGestureImpactPlayed && CurrentGesture.ImpactSound && Now>=GestureStartTime+CurrentGesture.ImpactDelaySeconds)
        {
            bGestureImpactPlayed=true;
            FActingSoundDefinition Impact; Impact.Sound=CurrentGesture.ImpactSound; Impact.VolumeMultiplier=CurrentGesture.ImpactVolume;
            PlayOwnedSound(Impact,GestureStartTime+CurrentGesture.ImpactDelaySeconds,Now,bClockRunning);
        }
        if (Now >= GestureEndTime) { FinishGesture(true); }
        else if (UAnimInstance* Instance = ControlledAnimInstance.Get())
        {
            const double Age = FMath::Max(0.0, Now - GestureStartTime);
            if (UAnimSingleNodeInstance* Single = Cast<UAnimSingleNodeInstance>(Instance))
            {
                const float Length = Single->GetLength();
                const double Position = Age * CurrentGesture.PlayRate;
                Single->SetPosition(Length > 0.0f && CurrentGesture.bLoop ? FMath::Fmod(Position, static_cast<double>(Length))
                    : FMath::Min(Position, static_cast<double>(Length)), false);
                Single->SetPlaying(false);
            }
            else if (OwnedMontage)
            {
                // Keep the live montage aligned if the shared scene clock is paused or hitching.
                const double Length = OwnedMontage->GetPlayLength();
                const double Position = Age * CurrentGesture.PlayRate;
                Instance->Montage_SetPosition(OwnedMontage, CurrentGesture.bLoop && Length > 0.0
                    ? FMath::Fmod(Position, Length) : FMath::Min(Position, Length));
                if (bClockRunning) { Instance->Montage_Resume(OwnedMontage); }
                else { Instance->Montage_Pause(OwnedMontage); }
            }
        }
    }
    for (int32 Index = OwnedAudio.Num() - 1; Index >= 0; --Index)
    {
        UAudioComponent* Audio = OwnedAudio[Index];
        if (!IsValid(Audio) || Now >= AudioEndTimes[Index] || !Audio->IsPlaying())
        {
            if (IsValid(Audio)) { Audio->Stop(); }
            OwnedAudio.RemoveAtSwap(Index);
            AudioEndTimes.RemoveAtSwap(Index);
            AudioBaseVolumes.RemoveAtSwap(Index);
        }
        else { Audio->SetVolumeMultiplier(AudioBaseVolumes[Index]*UStudioAudioSubsystem::EffectsMultiplier(this)); Audio->SetPaused(!bClockRunning); }
    }
}

void UActingComponent::UpdateReplayPresentation(double CurrentTakeTime, bool bClockRunning)
{
    if (bReplayOnly) { UpdatePresentation(CurrentTakeTime, bClockRunning); }
}

void UActingComponent::PlayOwnedSound(const FActingSoundDefinition& Definition,double StartTime,double Now,bool bClockRunning)
{
    if (!Definition.Sound || !GetOwner() || Now<StartTime) { return; }
    const float Pitch=FMath::IsFinite(Definition.PitchMultiplier)?FMath::Max(.01f,Definition.PitchMultiplier):1.f;
    const double Age=FMath::Max(0.,Now-StartTime);
    const double Limit=FMath::IsFinite(Definition.MaxDurationSeconds)?FMath::Max(.01f,Definition.MaxDurationSeconds):5.;
    const double Length=Definition.Sound->GetDuration();
    if (Age>=Limit || (Length>0 && Age*Pitch>=Length)) { return; }
    const float Volume=FMath::IsFinite(Definition.VolumeMultiplier)?FMath::Max(0.f,Definition.VolumeMultiplier):1.f;
    const auto* Recorder=GetOwner()->FindComponentByClass<UActingRecorderComponent>();
    USoundAttenuation* Attenuation=Definition.Attenuation?Definition.Attenuation.Get():bReplayOnly?ReplaySettings.DefaultAttenuation.Get()
        :Recorder && Recorder->IsRecording()?Recorder->GetRecordingData().ActingSettings.DefaultAttenuation.Get():DefaultAttenuation.Get();
    UAudioComponent* Audio=UGameplayStatics::SpawnSoundAttached(Definition.Sound,GetOwner()->GetRootComponent(),NAME_None,
        FVector::ZeroVector,FRotator::ZeroRotator,EAttachLocation::KeepRelativeOffset,true,
        Volume*UStudioAudioSubsystem::EffectsMultiplier(this),Pitch,static_cast<float>(Age*Pitch),Attenuation,nullptr,true);
    if (Audio)
    {
        ++StartedSoundCount; Audio->SetPaused(!bClockRunning); OwnedAudio.Add(Audio);
        AudioEndTimes.Add(StartTime+FMath::Min(Limit,Length>0?Length/Pitch:Limit)); AudioBaseVolumes.Add(Volume);
    }
}

int32 UActingComponent::GetActiveSoundCount() const
{
    int32 Count=0; for (const UAudioComponent* Audio:OwnedAudio) { if (IsValid(Audio) && Audio->IsPlaying()) { ++Count; } } return Count;
}

void UActingComponent::UpdateFootsteps(double Now,bool bClockRunning)
{
    const auto* Character=Cast<ACharacter>(GetOwner());
    const auto* Recorder=GetOwner()?GetOwner()->FindComponentByClass<UActingRecorderComponent>():nullptr;
    if (bReplayOnly || !Character || !Recorder || !Recorder->IsRecording() || !CanTriggerActions() || !bClockRunning)
    { bHasFootstepPosition=false; FootstepTravel=0; return; }
    const auto& Definitions=Recorder->GetRecordingData().ActingSettings.Footsteps;
    if (Definitions.IsEmpty()) { return; }
    const FVector Here=Character->GetActorLocation();
    const float Distance=bHasFootstepPosition?FVector::Dist2D(Here,LastFootstepLocation):0;
    LastFootstepLocation=Here; bHasFootstepPosition=true;
    if (!Character->GetCharacterMovement()->IsMovingOnGround() || Character->GetVelocity().SizeSquared2D()<400.f || Distance>500 || ActiveGestureId==ActingIds::Gesture(EActingGesture::Faint))
    { FootstepTravel=0; return; }
    FootstepTravel+=Distance;
    if (FootstepTravel>=FMath::Max(30.f,FootstepDistanceCm) && Now-LastFootstepTime>=.18)
    {
        const FName Id=Definitions[FootstepVariation%Definitions.Num()].Identifier;
        if (TriggerLive(EActingEventType::Footstep,Id)) { FootstepTravel=0; LastFootstepTime=Now; ++FootstepVariation; }
    }
}

void UActingComponent::StopAllActions()
{
    ++ActionGeneration;
    FinishGesture(false);
    for (UAudioComponent* Audio : OwnedAudio) { if (IsValid(Audio)) { Audio->Stop(); } }
    OwnedAudio.Reset();
    AudioEndTimes.Reset();
    AudioBaseVolumes.Reset();
    bHasFootstepPosition=false; FootstepTravel=0; LastFootstepTime=-1; FootstepVariation=0;
    if (!bNotifyingReset && !bEndingPlay && IsValid(GetOwner()) && !GetOwner()->IsActorBeingDestroyed()
        && GetWorld() && !GetWorld()->bIsTearingDown)
    {
        TGuardValue<bool> Guard(bNotifyingReset, true);
        OnActionsReset.Broadcast();
    }
}

void UActingComponent::ResetPlaybackState()
{
    bExternalScenePresentation=false;
    LastTriggeredEvent = FActingEvent();
    DispatchedEventCount = 0;
    StartedSoundCount = 0;
    StopAllActions();
}

void UActingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if(bExternalScenePresentation) { return; }
    if (!bReplayOnly)
    {
        const UActingRecorderComponent* Recorder = GetOwner() ? GetOwner()->FindComponentByClass<UActingRecorderComponent>() : nullptr;
        const UActingSceneClockSubsystem* Clock = GetWorld() ? GetWorld()->GetSubsystem<UActingSceneClockSubsystem>() : nullptr;
        const bool bRunning = !Recorder || !Recorder->IsRecording() || !Recorder->UsesSharedSceneClock() || !Clock || Clock->IsClockRunning();
        UpdatePresentation(GetLiveTime(), bRunning);
        UpdateFootsteps(GetLiveTime(),bRunning);
    }
}

void UActingComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    bEndingPlay = true;
    StopAllActions();
    Super::EndPlay(EndPlayReason);
}
