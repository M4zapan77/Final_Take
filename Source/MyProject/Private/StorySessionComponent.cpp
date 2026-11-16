#include "StorySessionComponent.h"
#include "StudioNPCPerformanceComponent.h"
#include "MovieManager.h"
#include "ActingRecorderComponent.h"
#include "ActingSceneClockSubsystem.h"
#include "ActingComponent.h"
#include "WardrobeComponent.h"
#include "StudioCharacterProfile.h"
#include "StudioAudioProfile.h"
#include "ObjectiveManagerComponent.h"
#include "PropInteractionComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PawnMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Components/SceneComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

UStorySessionComponent::UStorySessionComponent() { PrimaryComponentTick.bCanEverTick=false; }
bool UStorySessionComponent::IsCurrent() const { return bActive && Movie && Movie->bSessionActive && Movie->SessionGeneration==Generation; }
int32 UStorySessionComponent::GetTurnNumber() const { return IsCurrent() && Movie->CurrentTakeIndex>=0 && Movie->CurrentTakeIndex<6 ? Movie->CurrentTakeIndex+1 : 0; }
int32 UStorySessionComponent::GetRoundNumber() const { return GetTurnNumber()>0 ? (GetTurnNumber()-1)/3+1 : 0; }
int32 UStorySessionComponent::GetRoleIndex() const { return GetTurnNumber()>0 ? (GetTurnNumber()-1)%3 : INDEX_NONE; }
double UStorySessionComponent::GetTurnElapsed() const { return IsCurrent() ? FMath::Clamp(Movie->GetSceneTimeSeconds()-TurnStartSeconds,0.0,Movie->SessionRecordingDuration) : 0.0; }
APawn* UStorySessionComponent::FindActor(FName ActorId) const { for (const auto& Member:Cast) { if(Member.ActorId==ActorId) { return Member.Pawn; } } return nullptr; }

bool UStorySessionComponent::ConfigureActor(APawn* Pawn)
{
    if(!Pawn) { return false; }
    if(!Pawn->GetRootComponent())
    {
        auto* Root=NewObject<USceneComponent>(Pawn); Pawn->AddInstanceComponent(Root); Pawn->SetRootComponent(Root); Root->RegisterComponent();
    }
    const auto Ensure=[Pawn]<typename T>() -> T*
    {
        if(T* Existing=Pawn->FindComponentByClass<T>()) { return Existing; }
        T* Component=NewObject<T>(Pawn); Pawn->AddInstanceComponent(Component); Component->RegisterComponent(); return Component;
    };
    auto* Acting=Ensure.operator()<UActingComponent>();
    auto* Wardrobe=Ensure.operator()<UWardrobeComponent>();
    auto* Recorder=Ensure.operator()<UActingRecorderComponent>();
    Recorder->bCompactStationarySamples=!FParse::Param(FCommandLine::Get(),TEXT("StoryNoCompaction"));
    Ensure.operator()<UPropInteractionComponent>();
    if(Movie->CharacterProfile)
    {
        Movie->CharacterProfile->ApplyModel(::Cast<ACharacter>(Pawn));
        Movie->CharacterProfile->ConfigureActing(Acting);
        Wardrobe->FacialSettings=Movie->CharacterProfile->Face;
    }
    Wardrobe->Catalog=Movie->LiveWardrobeComponent->Catalog;
    if(Movie->AudioProfile) { Movie->AudioProfile->ConfigureActing(Acting); }
    Wardrobe->SetSelectionEnabled(false);
    Acting->bRequireActiveRecording=true; Acting->SetLiveActionsEnabled(false);
    Recorder->OnRecordingFinished.AddUniqueDynamic(Movie,&AMovieManager::HandleRecordingFinished);
    if(auto* Character=::Cast<ACharacter>(Pawn))
    {
        auto* Move=Character->GetCharacterMovement();
        if(Pawn==OriginalPawn)
        {OriginalControls.Speed=Move->MaxWalkSpeed;OriginalControls.Acceleration=Move->MaxAcceleration;OriginalControls.Braking=Move->BrakingDecelerationWalking;OriginalControls.RotationYaw=Move->RotationRate.Yaw;OriginalControls.bPhysicsInteraction=Move->bEnablePhysicsInteraction;OriginalControls.bSaved=true;}
        // Short indoor objectives need controlled stopping more than additional top speed.
        Move->MaxWalkSpeed=FMath::Min(Move->MaxWalkSpeed,500.f);Move->MaxAcceleration=2400;Move->BrakingDecelerationWalking=2600;Move->RotationRate.Yaw=600;Move->bEnablePhysicsInteraction=false;
    }
    if(auto* Arm=Pawn->FindComponentByClass<USpringArmComponent>())
    {
        if(Pawn==OriginalPawn)
        {OriginalControls.ArmLength=Arm->TargetArmLength;OriginalControls.SocketOffset=Arm->SocketOffset;OriginalControls.TargetOffset=Arm->TargetOffset;OriginalControls.bCameraLag=Arm->bEnableCameraLag;OriginalControls.bRotationLag=Arm->bEnableCameraRotationLag;OriginalControls.LagSpeed=Arm->CameraLagSpeed;OriginalControls.LagDistance=Arm->CameraLagMaxDistance;OriginalControls.bCollision=Arm->bDoCollisionTest;OriginalControls.ProbeSize=Arm->ProbeSize;OriginalControls.bSubstep=Arm->bUseCameraLagSubstepping;OriginalControls.bArmSaved=true;}
        Arm->TargetArmLength=400;Arm->SocketOffset=FVector(0,35,35);Arm->TargetOffset=FVector(0,0,35);Arm->bEnableCameraLag=true;Arm->CameraLagSpeed=16;Arm->CameraLagMaxDistance=45;Arm->bUseCameraLagSubstepping=true;Arm->bEnableCameraRotationLag=false;Arm->bDoCollisionTest=true;Arm->ProbeSize=12;
    }
    return true;
}

bool UStorySessionComponent::BeginSession()
{
    Movie=::Cast<AMovieManager>(GetOwner());
    if(!Movie || !Movie->bSessionActive || Movie->SessionRoles.Num()!=3 || !Movie->Controller.IsValid()) { return false; }
    if(!FMath::IsFinite(Movie->StoryWardrobeDurationSeconds) || Movie->StoryWardrobeDurationSeconds<0 ||
       !FMath::IsFinite(Movie->StoryTurnDurationSeconds) || Movie->StoryTurnDurationSeconds<.05 || Movie->StoryTurnDurationSeconds>10.0) { return false; }
    SessionScenario=FStoryScenarioDefinition();const UStoryScenarioData* Chosen=nullptr;TSet<FName> ScenarioIds;
    for(const UStoryScenarioData* Entry:Movie->ScenarioLibrary)
    {
        if(!Entry || !Entry->Definition.IsValid() || ScenarioIds.Contains(Entry->Definition.Id)){return false;}ScenarioIds.Add(Entry->Definition.Id);
        if((!Chosen && Movie->SelectedScenarioId.IsNone()) || Entry->Definition.Id==Movie->SelectedScenarioId){Chosen=Entry;}
    }
    if(Chosen){SessionScenario=Chosen->Definition;}
    Movie->SessionResultsSettings.MovieTitle=SessionScenario.Name;
    if(!Movie->SelectedScenarioId.IsNone() && SessionScenario.Id!=Movie->SelectedScenarioId){return false;}
    if(!SessionScenario.InitialMoneyOwner.IsNone() && !Movie->SessionRoles.ContainsByPredicate([this](const auto& Role){return Role.RoleId==SessionScenario.InitialMoneyOwner;})){return false;}
    bActive=true; Generation=Movie->SessionGeneration; OriginalPawn=Movie->LivePerformer;
    bOriginalCompact=Movie->Recorder->bCompactStationarySamples;
    bOriginalActorTick=OriginalPawn->IsActorTickEnabled();
    if(auto* Character=::Cast<ACharacter>(OriginalPawn)) { bOriginalPauseAnims=Character->GetMesh()->bPauseAnims; }
    bOriginalAutoCamera=Movie->Controller->bAutoManageActiveCameraTarget;
    Movie->Controller->bAutoManageActiveCameraTarget=false;
    Movie->SessionPreparationDuration=Movie->StoryWardrobeDurationSeconds;
    Movie->SessionRecordingDuration=Movie->StoryTurnDurationSeconds;
    Movie->SceneClock->StopClock(); TimelineDuration=0; TurnStartSeconds=0;
    Cast.Reset(); Segments.Reset(); bPendingEnd=bCutRequested=false;
    for(int32 Index=0;Index<3;++Index)
    {
        Movie->SessionRoles[Index].DisplayName=RoleName(Index);Movie->SessionRoles[Index].ScriptInstruction=RoleBrief(Index);
        APawn* Pawn=OriginalPawn;
        if(Index>0)
        {
            Pawn=GetWorld()->SpawnActorDeferred<APawn>(OriginalPawn->GetClass(),Movie->InitialWorldTransforms[Index],Movie,nullptr,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
            if(Pawn) { Pawn->AutoPossessPlayer=EAutoReceiveInput::Disabled; Pawn->AutoPossessAI=EAutoPossessAI::Disabled; UGameplayStatics::FinishSpawningActor(Pawn,Movie->InitialWorldTransforms[Index]); }
        }
        if(!Pawn) { return false; }
        FStoryCastMember& Member=Cast.AddDefaulted_GetRef(); Member.Pawn=Pawn; Member.ActorId=Movie->SessionRoles[Index].RoleId;
        Member.InitialTransform=Movie->InitialWorldTransforms[Index];
        if(!ConfigureActor(Pawn)) { return false; }
        if(auto* Character=::Cast<ACharacter>(Pawn))
        {
            const FVector Position=Member.InitialTransform.GetLocation();
            FHitResult Floor;
            FCollisionQueryParams Query(SCENE_QUERY_STAT(StoryInitialFloor),true,Pawn);
            if(GetWorld()->LineTraceSingleByObjectType(Floor,Position+FVector(0,0,500),Position-FVector(0,0,1000),FCollisionObjectQueryParams(ECC_WorldStatic),Query) && Floor.ImpactNormal.Z>.7)
            { Member.InitialTransform.SetLocation(FVector(Position.X,Position.Y,Floor.ImpactPoint.Z+Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()+1)); }
        }
        Pawn->SetActorTransform(Member.InitialTransform,false,nullptr,ETeleportType::TeleportPhysics);
    }
    if(!InitializeScene()){return false;}
    FreezeCast(true); BeginTurn(); return IsCurrent();
}

void UStorySessionComponent::FreezeCast(bool bFreezeActive)
{
    for(int32 Index=0;Index<Cast.Num();++Index)
    {
        APawn* Pawn=Cast[Index].Pawn; if(!IsValid(Pawn)) { continue; }
        const bool bFrozen=bFreezeActive || Index!=GetRoleIndex();
        Pawn->ConsumeMovementInputVector();
        if(auto* Character=::Cast<ACharacter>(Pawn))
        {
            auto* Move=Character->GetCharacterMovement(); Move->StopMovementImmediately();
            if(bFrozen) { Character->StopJumping(); Move->DisableMovement(); Move->Deactivate(); }
            else { Move->Activate(true); Move->SetMovementMode(MOVE_Walking); }
            Character->GetMesh()->bPauseAnims=bFrozen;
        }
        Pawn->SetActorTickEnabled(!bFrozen);
    }
}

bool UStorySessionComponent::SwitchActor(int32 Index)
{
    if(!IsCurrent() || !Cast.IsValidIndex(Index) || !IsValid(Cast[Index].Pawn)) { return false; }
    Movie->SetPerformerFrozen(true);
    APawn* Pawn=Cast[Index].Pawn;
    APawn* PreviousPawn=Movie->LivePerformer;
    for(auto& Member:Cast){if(Member.Pawn==PreviousPawn){Member.LastViewRotation=Movie->Controller->GetControlRotation();Member.bHasView=true;}}
    if(Movie->Controller->PlayerCameraManager)
    {
        FVector Location;FRotator Rotation;Movie->Controller->PlayerCameraManager->GetCameraViewPoint(Location,Rotation);
        if(!PossessionCamera){FActorSpawnParameters Spawn;Spawn.Owner=Movie;Spawn.ObjectFlags|=RF_Transient;PossessionCamera=GetWorld()->SpawnActor<ACameraActor>(Spawn);}
        if(PossessionCamera){PossessionCamera->SetActorLocationAndRotation(Location,Rotation);PossessionCamera->GetCameraComponent()->SetFieldOfView(Movie->Controller->PlayerCameraManager->GetFOVAngle());}
    }
    Movie->LivePerformer=Pawn; Movie->Recorder=Pawn->FindComponentByClass<UActingRecorderComponent>();
    Movie->LiveWardrobeComponent=Pawn->FindComponentByClass<UWardrobeComponent>();
    Movie->LiveActingComponent=Pawn->FindComponentByClass<UActingComponent>();
    Movie->LivePropInteraction=Pawn->FindComponentByClass<UPropInteractionComponent>();
    Movie->Movement=Pawn->GetMovementComponent(); Movie->bFrozen=true;
    if(Movie->Controller->GetPawn()!=Pawn) { Movie->Controller->Possess(Pawn); }
    Movie->Controller->SetControlRotation(Cast[Index].bHasView?Cast[Index].LastViewRotation:FRotator(-20,Pawn->GetActorRotation().Yaw,0));
    if(PossessionCamera){Movie->Controller->SetViewTarget(PossessionCamera);}
    Movie->Controller->SetViewTargetWithBlend(Pawn,.35f,VTBlend_Cubic);
    FreezeCast(true);
    return IsValid(Movie->Recorder) && IsValid(Movie->LiveWardrobeComponent) && IsValid(Movie->LiveActingComponent);
}

void UStorySessionComponent::BeginTurn()
{
    const uint32 ExpectedGeneration=Generation;
    if(!IsCurrent() || !SwitchActor(GetRoleIndex())) { if(Movie) { Movie->FailMovie(NSLOCTEXT("FinalTake","InvalidCast","Could not prepare the next character.")); } return; }
    bPendingEnd=bCutRequested=false; Movie->bPendingTakeFinished=false;
    if(TimelineDuration>0){AddSceneEvent(EStoryEventType::PresentationStop,Movie->GetCurrentRoleId(),NAME_None,TEXT("TurnStart"));}
    Movie->LiveActingComponent->ResetPlaybackState(); Movie->LiveActingComponent->SetLiveActionsEnabled(false);
    if(!IsCurrent() || Generation!=ExpectedGeneration) { return; }
    PreviewContextObjective();
    SetPhysicsPaused(true);Movie->bStartingTake=false;
    if(!Cast[GetRoleIndex()].bCustomized)
    {
        Movie->PreparationId=Movie->PreparationId==MAX_int32?1:Movie->PreparationId+1;
        if(!Movie->LiveWardrobeComponent->BeginSelection(Movie->SessionRoles[GetRoleIndex()].DefaultOutfit)) { Movie->FailMovie(NSLOCTEXT("FinalTake","InvalidOutfit","Could not prepare the outfit.")); return; }
        if(!IsCurrent() || Generation!=ExpectedGeneration) { return; }
        Movie->SetLookLocked(true); Movie->PhaseStartWorldTime=GetWorld()->GetTimeSeconds();
        Movie->ChangeState(EMovieState::Preparation);
        if(!IsCurrent() || Generation!=ExpectedGeneration || Movie->State!=EMovieState::Preparation) { return; }
        Movie->ShowObjectiveHUD();
        if(!Movie->OpenWardrobePresentation()) { Movie->FailMovie(NSLOCTEXT("FinalTake","InvalidWardrobe","Could not open the wardrobe.")); return; }
        if(IsCurrent() && Generation==ExpectedGeneration) { Movie->OnPreparationStarted.Broadcast(GetTurnNumber(),Movie->GetCurrentRoleId(),Movie->SessionPreparationDuration); }
    }
    else
    {
        Movie->bStartingTake=true;
        // Round two resumes the existing actor: no selection/default inventory is reapplied.
        BeginRecording();
    }
}

void UStorySessionComponent::BeginRecording()
{
    const uint32 ExpectedGeneration=Generation;
    if(!IsCurrent() || bPendingEnd || !Movie->bStartingTake || Movie->Recorder->IsRecording()) { return; }
    if(Movie->State!=EMovieState::Preparation && Movie->State!=EMovieState::TakeTransition && Movie->State!=EMovieState::Recording) { return; }
    FStoryCastMember& Member=Cast[GetRoleIndex()];const bool bFirstOutfit=!Member.bCustomized;
    if(!Member.bCustomized) { Member.Appearance=Movie->LiveWardrobeComponent->CaptureAppearance(); Member.ActingSettings=Movie->LiveActingComponent->CapturePlaybackSettings(); Member.bCustomized=true; }
    Movie->LiveWardrobeComponent->SetSelectionEnabled(false);
    Movie->SetLookLocked(false); Movie->SetPerformerFrozen(false); FreezeCast(false);
    Movie->Controller->bShowMouseCursor=false; Movie->Controller->SetInputMode(FInputModeGameOnly());
    TurnStartSeconds=Movie->SceneClock->GetSceneTimeSeconds();
    if(!Movie->Recorder->StartRecording(Movie->SessionRecordingDuration,true)) { Movie->FailMovie(NSLOCTEXT("FinalTake","RecordFailed","Could not start the performance.")); return; }
    Movie->SceneClock->ResumeClock(); Movie->PhaseStartWorldTime=GetWorld()->GetTimeSeconds();
    Movie->ChangeState(EMovieState::Recording);
    if(!IsCurrent() || Generation!=ExpectedGeneration || Movie->State!=EMovieState::Recording) { return; }
    SetDetentionInput(SceneState.DetainedUntil.FindRef(Member.ActorId)>Movie->GetSceneTimeSeconds());
    Movie->ObjectiveManager->BeginTake(Movie->LivePerformer,Movie->LiveActingComponent);
    if(Segments.IsEmpty() && SceneEvents.IsEmpty())
    {
        if(SessionScenario.bAlarmInitiallyActive){AddSceneEvent(EStoryEventType::AlarmChanged,NAME_None,NAME_None,TEXT("On"));}
        if(SessionScenario.bGuardInitiallyDistracted){SetNPCMood(TEXT("NPC.Guard"),EStoryNPCMood::Distracted,GuardDistractionSeconds,TEXT("Dance"));}
    }
    if(bFirstOutfit){InitializeOutfitProps();}SetPhysicsPaused(false);
    if(!SceneState.MoneyOwner().IsNone()){ApplyMoneyPresentation(SceneState.MoneyOwner(),SceneState.FindObject(SceneState.MoneyObjectId)->WorldTransform,false);}else{UpdateHandSuppression(false);}
    Movie->ShowObjectiveHUD(); Movie->LiveActingComponent->SetLiveActionsEnabled(true);
    if(!IsCurrent() || Generation!=ExpectedGeneration) { return; }
    Movie->OnRecordingStarted.Broadcast(GetTurnNumber(),Movie->GetCurrentRoleId(),Movie->SessionRecordingDuration);
}

bool UStorySessionComponent::CutTurn()
{
    if(!IsCurrent() || Movie->State!=EMovieState::Recording || bPendingEnd || bCutRequested || !Movie->Recorder->IsRecording() || GetTurnElapsed()<.05) { return false; }
    bCutRequested=true; return Movie->Recorder->StopRecording();
}

void UStorySessionComponent::CaptureFinishedSegment(const FActingRecording& Recording)
{
    const uint32 ExpectedGeneration=Generation;
    if(!IsCurrent() || Movie->State!=EMovieState::Recording || bPendingEnd) { return; }
    if(!Recording.IsValid() || Recording.GetDurationSeconds()<=0 || Recording.GetDurationSeconds()>Movie->SessionRecordingDuration+1.e-6) { Movie->FailMovie(NSLOCTEXT("FinalTake","InvalidSegment","The recorded turn is invalid.")); return; }
    FStorySegment& Segment=Segments.AddDefaulted_GetRef();
    Segment.ActorId=Movie->GetCurrentRoleId(); Segment.TurnNumber=GetTurnNumber(); Segment.StartSeconds=TurnStartSeconds;
    Segment.EndSeconds=TurnStartSeconds+Recording.GetDurationSeconds(); Segment.InitialTransform=Recording.Samples[0].WorldTransform;
    Segment.FinalTransform=Recording.Samples.Last().WorldTransform; Segment.Recording=Recording;
    Segment.Objective=CurrentObjective;Segment.ObjectiveResult=Movie->ObjectiveManager->FinishTake(Movie->GetSceneTimeSeconds(),Segment.FinalTransform.GetLocation());
    // Store appearance/settings once per character, not again for each turn.
    Segment.Recording.Appearance=FRecordedAppearance(); Segment.Recording.ActingSettings=FActingPlaybackSettings();
    TimelineDuration=Segment.EndSeconds;
    Movie->LivePerformer->SetActorTransform(Segment.FinalTransform,false,nullptr,ETeleportType::TeleportPhysics);
    Movie->SceneClock->PauseClockAt(TimelineDuration);
    SetDetentionInput(false);
    for(const auto& Object:SceneState.Objects){if(Object.OwnerId.IsNone()){ObserveProp(Object.ObjectId,true);}}SetPhysicsPaused(true);
    AddSceneEvent(EStoryEventType::PresentationStop,Movie->GetCurrentRoleId(),NAME_None,TEXT("TurnEnd"));
    Movie->SetPerformerFrozen(true); FreezeCast(true);
    bPendingEnd=true;
    Movie->LiveActingComponent->StopAllActions();
    if(!IsCurrent() || Generation!=ExpectedGeneration) { return; }
    Movie->LiveActingComponent->SetLiveActionsEnabled(false);
    UpdateScene();
}

void UStorySessionComponent::FinishTurn()
{
    const uint32 ExpectedGeneration=Generation;
    if(!IsCurrent() || !bPendingEnd || Segments.IsEmpty()) { return; }
    bPendingEnd=false;
    FMoviePerformance Completed; const auto& Segment=Segments.Last();
    Completed.RoleId=Segment.ActorId; Completed.InitialTransform=Segment.InitialTransform; Completed.Recording=Segment.Recording;
    Movie->OnTakeRecorded.Broadcast(Completed);
    if(!IsCurrent() || Generation!=ExpectedGeneration) { return; }
    ++Movie->CurrentTakeIndex;
    if(Movie->CurrentTakeIndex==6)
    {
        Movie->CurrentTakeIndex=INDEX_NONE;
        if(!AssembleMovie()) { Movie->FailMovie(NSLOCTEXT("FinalTake","AssembleFailed","Could not assemble the movie.")); return; }
        Movie->CalculateMovieScore();
        if(!IsCurrent() || Generation!=ExpectedGeneration){return;}
        if(!Movie->MovieResults.bAvailable){Movie->FailMovie(NSLOCTEXT("FinalTake","InvalidStoryResults","Could not calculate the story results."));return;}
    }
    Movie->PhaseStartWorldTime=GetWorld()->GetTimeSeconds(); Movie->SetLookLocked(true);
    if(Movie->CurrentTakeIndex!=INDEX_NONE){PreviewContextObjective(false);}
    Movie->ChangeState(EMovieState::TakeTransition);
    if(IsCurrent() && Movie->GetPhaseRemainingSeconds()<=0) { if(Movie->CurrentTakeIndex==INDEX_NONE) { Movie->BeginPremiere(); } else { BeginTurn(); } }
}

void UStorySessionComponent::UpdateStory(float DeltaSeconds)
{
    if(!IsCurrent()) { return; }
    UpdateScene();
    if(!Movie->Controller.IsValid() || Movie->Controller->GetPawn()!=Movie->LivePerformer || !IsValid(Movie->Recorder)) { Movie->FailMovie(NSLOCTEXT("FinalTake","PossessionLost","Lost control of the character.")); return; }
    if(Movie->State==EMovieState::Preparation && Movie->GetPhaseRemainingSeconds()<=0) { Movie->ReadyForTake(Movie->PreparationId); }
    else if(Movie->State==EMovieState::Recording && bPendingEnd) { FinishTurn(); }
    else if(Movie->State==EMovieState::Recording && !Movie->Recorder->IsRecording()) { Movie->FailMovie(NSLOCTEXT("FinalTake","RecorderCancelled","Another system cancelled the recording.")); }
    else if(Movie->State==EMovieState::TakeTransition && Movie->GetPhaseRemainingSeconds()<=0) { if(Movie->CurrentTakeIndex==INDEX_NONE) { Movie->BeginPremiere(); } else { BeginTurn(); } }
}

bool UStorySessionComponent::AssembleRoleRecording(FName ActorId,const FStoryCastMember& Member,const TArray<FStorySegment>& Source,double Duration,FActingRecording& Out)
{
    Out=FActingRecording(); Out.Appearance=Member.Appearance; Out.ActingSettings=Member.ActingSettings;
    const auto AddPose=[&Out](double Time,const FTransform& Transform)
    {
        if(!Out.Samples.IsEmpty() && FMath::IsNearlyEqual(Out.Samples.Last().TimeSeconds,Time,1.e-8)) { Out.Samples.Last().WorldTransform=Transform; return; }
        FActingTransformSample& Pose=Out.Samples.AddDefaulted_GetRef(); Pose.TimeSeconds=Time; Pose.WorldTransform=Transform;
    };
    AddPose(0,Member.InitialTransform);
    for(const FStorySegment& Segment:Source)
    {
        if(Segment.ActorId!=ActorId) { continue; }
        AddPose(Segment.StartSeconds,Segment.InitialTransform);
        for(const auto& Sample:Segment.Recording.Samples) { AddPose(Segment.StartSeconds+Sample.TimeSeconds,Sample.WorldTransform); }
        for(auto Event:Segment.Recording.Events) { Event.TimeSeconds+=Segment.StartSeconds; Out.Events.Add(Event); }
    }
    if(Out.Samples.Last().TimeSeconds<Duration) { AddPose(Duration,Out.Samples.Last().WorldTransform); }
    return Out.IsValid();
}

bool UStorySessionComponent::AssembleMovie()
{
    if(Segments.Num()!=6 || Cast.Num()!=3) { return false; }
    Movie->Performances.Reset();
    for(const FStoryCastMember& Member:Cast)
    {
        FMoviePerformance& Film=Movie->Performances.AddDefaulted_GetRef(); Film.RoleId=Member.ActorId; Film.InitialTransform=Member.InitialTransform;
        if(!AssembleRoleRecording(Member.ActorId,Member,Segments,TimelineDuration,Film.Recording)) { return false; }
        AppendSceneActingEvents(Member.ActorId,Film.Recording);
        const int32 RoleIndex=Cast.IndexOfByPredicate([&Member](const auto& C){return C.ActorId==Member.ActorId;});
        for(const auto& Segment:Segments)
        {
            if(Segment.ActorId!=Member.ActorId){continue;}
            Film.ObjectiveResult.Score+=Segment.ObjectiveResult.Score;
            Film.ObjectiveResult.CompletedObjectiveIds.Append(Segment.ObjectiveResult.CompletedObjectiveIds);
            Movie->SessionRoles[RoleIndex].Objectives.Add(Segment.Objective);
        }
        if(!Film.Recording.IsValid()){return false;}
    }
    SceneEvents.StableSort([](const auto& A,const auto& B){return A.TimeSeconds==B.TimeSeconds?A.Sequence<B.Sequence:A.TimeSeconds<B.TimeSeconds;});
    return true;
}

void UStorySessionComponent::PreparePremiere()
{
    bScenePlayback=false;SetPhysicsPaused(true);
    for(ACharacter* NPC:NPCActors)
    {
        if(auto* Background=NPC->FindComponentByClass<UStudioNPCPerformanceComponent>()){Background->ObserveBoundary(TimelineDuration);}
        NPC->GetMesh()->bPauseAnims=true;if(auto* Acting=NPC->FindComponentByClass<UActingComponent>()){Acting->ResetPlaybackState();}
    }
    FreezeCast(true);
    for(auto& Member:Cast) { if(IsValid(Member.Pawn)) { Member.Pawn->SetActorHiddenInGame(true); Member.Pawn->SetActorEnableCollision(false); } }
}

void UStorySessionComponent::ResetSession(bool bRestorePlayer)
{
    if(!bActive) { return; }
    SetDetentionInput(false);
    ClearScene();
    if(Movie) { Movie->SetPerformerFrozen(false); }
    for(auto& Member:Cast)
    {
        if(!IsValid(Member.Pawn)) { continue; }
        if(auto* Recorder=Member.Pawn->FindComponentByClass<UActingRecorderComponent>()) { Recorder->OnRecordingFinished.RemoveDynamic(Movie,&AMovieManager::HandleRecordingFinished); Recorder->ClearRecording(); }
        if(auto* Acting=Member.Pawn->FindComponentByClass<UActingComponent>()) { Acting->ResetPlaybackState(); Acting->SetLiveActionsEnabled(false); }
    }
    if(Movie && IsValid(OriginalPawn))
    {
        if(Movie->Controller.IsValid()) { Movie->Controller->bAutoManageActiveCameraTarget=bOriginalAutoCamera; if(bRestorePlayer && Movie->Controller->GetPawn()!=OriginalPawn) { Movie->Controller->Possess(OriginalPawn); } }
        Movie->LivePerformer=OriginalPawn; Movie->Recorder=OriginalPawn->FindComponentByClass<UActingRecorderComponent>();
        Movie->LiveWardrobeComponent=OriginalPawn->FindComponentByClass<UWardrobeComponent>(); Movie->LiveActingComponent=OriginalPawn->FindComponentByClass<UActingComponent>();
        Movie->Movement=OriginalPawn->GetMovementComponent(); Movie->bFrozen=true;
        Movie->Recorder->bCompactStationarySamples=bOriginalCompact;
        OriginalPawn->SetActorTickEnabled(bOriginalActorTick);
        if(auto* Character=::Cast<ACharacter>(OriginalPawn)) { Character->GetMesh()->bPauseAnims=bOriginalPauseAnims; }
        if(auto* Character=::Cast<ACharacter>(OriginalPawn);Character && OriginalControls.bSaved)
        {auto* Move=Character->GetCharacterMovement();Move->MaxWalkSpeed=OriginalControls.Speed;Move->MaxAcceleration=OriginalControls.Acceleration;Move->BrakingDecelerationWalking=OriginalControls.Braking;Move->RotationRate.Yaw=OriginalControls.RotationYaw;Move->bEnablePhysicsInteraction=OriginalControls.bPhysicsInteraction;}
        if(auto* Arm=OriginalPawn->FindComponentByClass<USpringArmComponent>();Arm && OriginalControls.bArmSaved)
        {Arm->TargetArmLength=OriginalControls.ArmLength;Arm->SocketOffset=OriginalControls.SocketOffset;Arm->TargetOffset=OriginalControls.TargetOffset;Arm->bEnableCameraLag=OriginalControls.bCameraLag;Arm->bEnableCameraRotationLag=OriginalControls.bRotationLag;Arm->CameraLagSpeed=OriginalControls.LagSpeed;Arm->CameraLagMaxDistance=OriginalControls.LagDistance;Arm->bDoCollisionTest=OriginalControls.bCollision;Arm->ProbeSize=OriginalControls.ProbeSize;Arm->bUseCameraLagSubstepping=OriginalControls.bSubstep;}
    }
    for(auto& Member:Cast) { if(IsValid(Member.Pawn) && Member.Pawn!=OriginalPawn) { Member.Pawn->Destroy(); } }
    if(IsValid(PossessionCamera)){PossessionCamera->Destroy();}PossessionCamera=nullptr;OriginalControls=FOriginalMovementCamera();
    Cast.Reset(); Segments.Reset(); OriginalPawn=nullptr; bActive=false; bPendingEnd=bCutRequested=false; TimelineDuration=TurnStartSeconds=0;
}
