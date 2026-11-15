#include "MovieManager.h"
#include "MovieResultsLibrary.h"
#include "MoviePresentationWidget.h"
#include "StudioGameMode.h"

#include "ActingRecorderComponent.h"
#include "ActingComponent.h"
#include "ActingSceneClockSubsystem.h"
#include "WardrobeComponent.h"
#include "WardrobeWidget.h"
#include "UnifiedWardrobeWidget.h"
#include "StudioCharacterProfile.h"
#include "StudioAudioProfile.h"
#include "ObjectiveManagerComponent.h"
#include "ObjectiveHUDWidget.h"
#include "InteractableProp.h"
#include "PropInteractionComponent.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "Components/AudioComponent.h"
#include "Components/SceneComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/LocalPlayer.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PawnMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/HUD.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "ReplayActor.h"
#include "StorySessionComponent.h"
#include "MovieDirectorComponent.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

DEFINE_LOG_CATEGORY_STATIC(LogMovieManager, Log, All);

AMovieManager::AMovieManager()
{
    CharacterProfile=LoadObject<UStudioCharacterProfile>(nullptr,TEXT("/Game/OneManStudio/Characters/Base3/Customization/DA_Base3Character"),nullptr,LOAD_NoWarn|LOAD_Quiet);
    AudioProfile=LoadObject<UStudioAudioProfile>(nullptr,TEXT("/Game/OneManStudio/Audio/DA_StudioAudio"),nullptr,LOAD_NoWarn|LOAD_Quiet);
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup = TG_PostUpdateWork;
    bReplicates = false;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("MovieOrigin")));
    ReplayActorClass = AReplayActor::StaticClass();
    WardrobeWidgetClass = UUnifiedWardrobeWidget::StaticClass();
    ObjectiveHUDClass = UObjectiveHUDWidget::StaticClass();
    MoviePresentationWidgetClass = UMoviePresentationWidget::StaticClass();
    PresentationMusic = CreateDefaultSubobject<UAudioComponent>(TEXT("PresentationMusic"));
    PresentationSting = CreateDefaultSubobject<UAudioComponent>(TEXT("PresentationSting"));
    PresentationClick = CreateDefaultSubobject<UAudioComponent>(TEXT("PresentationClick"));
    for (UAudioComponent* Audio : {PresentationMusic.Get(), PresentationSting.Get(), PresentationClick.Get()})
    { Audio->SetupAttachment(RootComponent); Audio->bAutoActivate=false; Audio->bAllowSpatialization=false; Audio->bIsUISound=true; }
    ObjectiveManager = CreateDefaultSubobject<UObjectiveManagerComponent>(TEXT("ObjectiveManager"));
    StorySession = CreateDefaultSubobject<UStorySessionComponent>(TEXT("StorySession"));
    MovieDirector=CreateDefaultSubobject<UMovieDirectorComponent>(TEXT("MovieDirector"));
    CosmeticCatalog = WardrobeDefaults::MakeCatalog();
    Roles.SetNum(3);
    const FName RoleIds[] = {TEXT("Robber"), TEXT("PoliceOfficer"), TEXT("Customer")};
    const FText Names[] = {NSLOCTEXT("FinalTakeUI","MovieManager.Names.89ebc07e45","Robber"), NSLOCTEXT("FinalTakeUI","MovieManager.Names.a35f3c0224","Police Officer"), NSLOCTEXT("FinalTakeUI","MovieManager.Names.0e85749a6f","Customer")};
    const FText Scripts[] = {NSLOCTEXT("FinalTakeUI","MovieManager.Scripts.f1f86f0cce","Sneak into the shop, grab imaginary money, and make a dramatic escape."),
        NSLOCTEXT("FinalTakeUI","MovieManager.Scripts.b61ba2b30f","Enter the scene, point at the robber, and perform an over-the-top arrest."),
        NSLOCTEXT("FinalTakeUI","MovieManager.Scripts.33510d0d95","React to the chaos in the shop. Make your surprise impossible to miss.")};
    for (int32 Index = 0; Index < Roles.Num(); ++Index)
    {
        Roles[Index].RoleId = RoleIds[Index];
        Roles[Index].DisplayName = Names[Index];
        Roles[Index].ScriptInstruction = Scripts[Index];
        Roles[Index].DefaultOutfit = WardrobeDefaults::Casual();
        Roles[Index].Objectives = MovieObjectiveDefaults::BankHeist(Index, RoleIds[0], RoleIds[1]);
    }
    Roles[0].DefaultOutfit.HeadId = TEXT("Head.RobberMask");
    Roles[0].DefaultOutfit.BodyId = TEXT("Body.Suit");
    Roles[0].DefaultOutfit.PropId = TEXT("Prop.MoneyBag");
    Roles[1].DefaultOutfit.HeadId = TEXT("Head.PoliceCap");
    Roles[1].DefaultOutfit.BodyId = TEXT("Body.Uniform");
    Roles[1].DefaultOutfit.PropId = TEXT("Prop.Donut");
    Roles[1].InitialTransform.SetLocation(FVector(0.0, 180.0, 0.0));
    Roles[2].InitialTransform.SetLocation(FVector(0.0, -180.0, 0.0));
}

bool AMovieManager::ValidateSetup(FText& OutError) const
{
    if (!PremiereSettings.IsValid())
    {
        OutError=NSLOCTEXT("FinalTakeUI","MovieManager.07d19ed372","Premiere transition durations, letterbox size and audio volumes must be finite and within their configured limits.");
        return false;
    }
    if (!FMath::IsFinite(PreparationDurationSeconds) || PreparationDurationSeconds < 0.0
        || !FMath::IsFinite(RecordingDurationSeconds) || RecordingDurationSeconds <= 0.0
        || !FMath::IsFinite(BetweenActorsSeconds) || BetweenActorsSeconds<0.0
        || !FMath::IsFinite(WrapSeconds) || WrapSeconds<0.0)
    {
        OutError = NSLOCTEXT("FinalTakeUI","MovieManager.b13f4a219e","Preparation must be finite and >= 0; recording must be finite and > 0.");
        return false;
    }
    if (Roles.Num() != 3 || !ReplayActorClass || AutomaticCameraOffset.ContainsNaN()
        || WardrobeCameraOffset.ContainsNaN() || WardrobeCameraLookAtOffset.ContainsNaN()
        || !FMath::IsFinite(WardrobeCameraFieldOfView) || WardrobeCameraFieldOfView < 10.0f || WardrobeCameraFieldOfView > 120.0f
        || !FMath::IsFinite(WardrobeFillLightLumens) || WardrobeFillLightLumens < 0.0f)
    {
        OutError = NSLOCTEXT("FinalTakeUI","MovieManager.75fc8e5401","Configure exactly three roles, a ReplayActor class, and a finite camera offset.");
        return false;
    }
    TSet<FName> RoleIds;
    for (const FMovieRoleDefinition& Definition : Roles)
    {
        if (Definition.RoleId.IsNone() || RoleIds.Contains(Definition.RoleId) || !Definition.InitialTransform.IsValid())
        {
            OutError = NSLOCTEXT("FinalTakeUI","MovieManager.d256a43fe6","Each role needs a unique identifier and a valid initial transform.");
            return false;
        }
        RoleIds.Add(Definition.RoleId);
    }
    if (PremiereCamera && PremiereCamera->GetWorld() != GetWorld())
    {
        OutError = NSLOCTEXT("FinalTakeUI","MovieManager.875227313d","The premiere camera must belong to this world.");
        return false;
    }
    if (WardrobePreviewCamera && WardrobePreviewCamera->GetWorld() != GetWorld())
    {
        OutError = NSLOCTEXT("FinalTakeUI","MovieManager.9eed30e4c0","The wardrobe preview camera must belong to this world.");
        return false;
    }
    return true;
}

bool AMovieManager::StartMovie()
{
    if (bSessionActive || bClearingSession || IsActorBeingDestroyed()) { return false; }
    bAutoStartAttempted = true;
    FText Error;
    if (!GetWorld() || !GetWorld()->IsGameWorld() || !ValidateSetup(Error))
    {
        FailMovie(Error.IsEmpty() ? NSLOCTEXT("FinalTakeUI","MovieManager.a60128220f","Start the movie in a gameplay world.") : Error);
        return false;
    }
    for (TActorIterator<AMovieManager> It(GetWorld()); It; ++It)
    {
        if (*It != this && It->HasActiveSession())
        {
            FailMovie(NSLOCTEXT("FinalTakeUI","MovieManager.e35a3a9bed","Another MovieManager already owns the shared scene timeline."));
            return false;
        }
    }
    APlayerController* Player = UGameplayStatics::GetPlayerController(this, PlayerIndex);
    APawn* Pawn = Player ? Player->GetPawn() : nullptr;
    UActingSceneClockSubsystem* Clock = GetWorld()->GetSubsystem<UActingSceneClockSubsystem>();
    if (!IsValid(Player) || !IsValid(Pawn) || !Clock || !Pawn->GetActorTransform().IsValid())
    {
        FailMovie(NSLOCTEXT("FinalTakeUI","MovieManager.a4f7f0808e","A local player controller with a valid possessed pawn is required."));
        return false;
    }
    UActingRecorderComponent* Component = Pawn->FindComponentByClass<UActingRecorderComponent>();
    if (Component && Component->IsRecording())
    {
        FailMovie(NSLOCTEXT("FinalTakeUI","MovieManager.a6431a7556","The player's recorder is already recording. Stop its other coordinator first."));
        return false;
    }
    if (!Component)
    {
        Component = NewObject<UActingRecorderComponent>(Pawn);
        Pawn->AddInstanceComponent(Component);
        Component->RegisterComponent();
    }
    Controller = Player;
    LivePerformer = Pawn;
    Recorder = Component;
    LiveActingComponent = Pawn->FindComponentByClass<UActingComponent>();
    if (!LiveActingComponent)
    {
        LiveActingComponent = NewObject<UActingComponent>(Pawn);
        Pawn->AddInstanceComponent(LiveActingComponent);
        LiveActingComponent->RegisterComponent();
    }
    const bool bCustomCharacter=CharacterProfile && CharacterProfile->ApplyModel(Cast<ACharacter>(Pawn));
    if(bCustomCharacter){CharacterProfile->ConfigureActing(LiveActingComponent);}
    if(AudioProfile){AudioProfile->ConfigureActing(LiveActingComponent);AudioProfile->ConfigurePremiere(PremiereSettings);}
    bOriginalLiveActionsEnabled = LiveActingComponent->AreLiveActionsEnabled();
    bOriginalRequireRecording = LiveActingComponent->bRequireActiveRecording;
    LiveActingComponent->bRequireActiveRecording = true;
    SceneClock = Clock;
    OriginalPawnTransform = Pawn->GetActorTransform();
    OriginalControlRotation = Player->GetControlRotation();
    OriginalViewTarget = Player->GetViewTarget();
    bOriginalHidden = Pawn->IsHidden();
    bOriginalCollision = Pawn->GetActorEnableCollision();
    bOriginalCursorVisible = Player->bShowMouseCursor;
    bOriginalClickEvents = Player->bEnableClickEvents;
    bOriginalMouseOverEvents = Player->bEnableMouseOverEvents;
    OriginalHUD=Player->GetHUD();
    bOriginalHUDVisible=OriginalHUD.IsValid()?OriginalHUD->bShowHUD:true;
    bGameplayHUDHidden=false;
    Movement = Pawn->GetMovementComponent();
    bOriginalMovementActive = Movement.IsValid() && Movement->IsActive();
    if (const UCharacterMovementComponent* CharacterMovement = Cast<UCharacterMovementComponent>(Movement.Get()))
    {
        OriginalMovementMode = CharacterMovement->MovementMode;
        OriginalCustomMovementMode = CharacterMovement->CustomMovementMode;
    }
    LiveWardrobeComponent = Pawn->FindComponentByClass<UWardrobeComponent>();
    if (!LiveWardrobeComponent)
    {
        LiveWardrobeComponent = NewObject<UWardrobeComponent>(Pawn);
        Pawn->AddInstanceComponent(LiveWardrobeComponent);
        LiveWardrobeComponent->RegisterComponent();
    }
    OriginalCosmeticCatalog = LiveWardrobeComponent->Catalog;
    OriginalDefaultOutfit = LiveWardrobeComponent->DefaultSelection;
    OriginalAppearance = LiveWardrobeComponent->CaptureAppearance();
    bOriginalWardrobeEnabled = LiveWardrobeComponent->IsSelectionEnabled();
    if (!bUsePerformerCosmeticCatalog) { LiveWardrobeComponent->Catalog = CosmeticCatalog; }
    if(bCustomCharacter){LiveWardrobeComponent->FacialSettings=CharacterProfile->Face;CharacterProfile->ConfigureCatalog(LiveWardrobeComponent->Catalog);}
    if(AudioProfile){AudioProfile->ConfigureCatalog(LiveWardrobeComponent->Catalog);}
    for(auto& Item:LiveWardrobeComponent->Catalog){Item.DisplayName=WardrobeDefaults::LocalizedName(Item.Id,Item.DisplayName);}
    if (!LiveWardrobeComponent->ValidateCatalog(Error)) { FailMovie(Error); return false; }
    LiveWardrobeComponent->SetSelectionEnabled(false);
    MovieOrigin = bUsePlayerStartAsOrigin ? OriginalPawnTransform : GetActorTransform();
    SessionRoles = Roles;
    for (int32 Index=0; Index<SessionRoles.Num(); ++Index)
    {
        if (bUseDefaultHeistObjectivesIfEmpty && SessionRoles[Index].Objectives.IsEmpty())
        { SessionRoles[Index].Objectives = MovieObjectiveDefaults::BankHeist(Index, SessionRoles[0].RoleId, SessionRoles[1].RoleId); }
    }
    LivePropInteraction = Pawn->FindComponentByClass<UPropInteractionComponent>();
    if (!LivePropInteraction)
    {
        LivePropInteraction = NewObject<UPropInteractionComponent>(Pawn);
        Pawn->AddInstanceComponent(LivePropInteraction); LivePropInteraction->RegisterComponent();
    }
    LivePropInteraction->ResetState(); ScoreSummary = FMovieScoreSummary();
    InitialWorldTransforms.Reset();
    for (const FMovieRoleDefinition& Definition : SessionRoles)
    {
        InitialWorldTransforms.Add(Definition.InitialTransform * MovieOrigin);
    }
    SessionPreparationDuration = PreparationDurationSeconds;
    SessionRecordingDuration = RecordingDurationSeconds;
    SessionBetweenActors=bUseTakeTransitions?BetweenActorsSeconds:0.0;
    SessionWrap=bUseTakeTransitions?WrapSeconds:0.0;
    SessionPremiereSettings=PremiereSettings;
    SessionResultsSettings=ResultsSettings;
    MovieResults=FMovieResults();
    PresentationPhase=EMoviePresentationPhase::None;
    Performances.Reset();
    LastError = FText::GetEmpty();
    CurrentTakeIndex = 0;
    bPendingTakeFinished = false;
    bSessionActive = true;
    ++SessionGeneration;
    Recorder->ClearRecording();
    Recorder->OnRecordingFinished.AddUniqueDynamic(this, &AMovieManager::HandleRecordingFinished);
    if (bBindRestartKey || bBindActingKeys || bBindInteractionKey)
    {
        EnableInput(Player);
        if (bBindRestartKey && !bRestartKeyBound)
        {
            InputComponent->BindKey(EKeys::R, IE_Pressed, this, &AMovieManager::HandleRestartKey);
            bRestartKeyBound = true;
        }
        if (bBindActingKeys && !bActingKeysBound)
        {
            for (const FKey& Key : {EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five,
                EKeys::Z, EKeys::X, EKeys::C, EKeys::V, EKeys::B, EKeys::N})
            {
                InputComponent->BindKey(Key, IE_Pressed, this, &AMovieManager::HandleActingKey);
            }
            bActingKeysBound = true;
        }
        if (bBindInteractionKey && !bInteractionKeyBound)
        {
            InputComponent->BindKey(EKeys::E, IE_Pressed, this, &AMovieManager::HandleInteractKey);
            bInteractionKeyBound = true;
        }
    }
    const bool bStoryRequested=bUseChronologicalGameplay || FParse::Param(FCommandLine::Get(),TEXT("FinalTakeStory"));
    if(bStoryRequested && !FParse::Param(FCommandLine::Get(),TEXT("StudioLegacyMovie")))
    {
        if(InputComponent && !bEndTurnKeyBound)
        {
            InputComponent->BindKey(EndTurnKey,IE_Pressed,this,&AMovieManager::HandleEndTurnKey);
            InputComponent->BindKey(DropObjectKey,IE_Pressed,this,&AMovieManager::HandleDropKey);
            InputComponent->BindKey(ThrowObjectKey,IE_Pressed,this,&AMovieManager::HandleThrowKey);bEndTurnKeyBound=true;
        }
        if(!StorySession->BeginSession()) { FailMovie(NSLOCTEXT("FinalTake","StartStoryFailed","Could not prepare the movie cast.")); }
    }
    else { BeginPreparation(); }
    return bSessionActive;
}

bool AMovieManager::IsChronologicalMovie() const { return StorySession && StorySession->IsStoryActive(); }
int32 AMovieManager::GetNumberOfTakes() const { return IsChronologicalMovie()?6:(bSessionActive?SessionRoles.Num():Roles.Num()); }
int32 AMovieManager::GetRoundNumber() const { return IsChronologicalMovie()?StorySession->GetRoundNumber():1; }
bool AMovieManager::EndCurrentTurn() { return IsChronologicalMovie() && StorySession->CutTurn(); }
void AMovieManager::HandleEndTurnKey() { EndCurrentTurn(); }
void AMovieManager::HandleDropKey() { if(IsChronologicalMovie()){StorySession->DropHeldObject();} }
void AMovieManager::HandleThrowKey() { if(IsChronologicalMovie()){StorySession->ThrowHeldObject();} }

bool AMovieManager::IsWardrobeOpen() const
{
    return bSessionActive && State == EMovieState::Preparation && !bStartingTake && IsValid(LiveWardrobeComponent);
}

void AMovieManager::HandleInteractKey()
{
    if(IsChronologicalMovie()){StorySession->Interact();return;}
    if (bBindInteractionKey && State == EMovieState::Recording && LivePropInteraction) { LivePropInteraction->Interact(); }
}
int32 AMovieManager::GetCurrentScore() const { return ObjectiveManager ? ObjectiveManager->CurrentScore : 0; }
int32 AMovieManager::GetScoreForRole(FName InRoleId) const
{
    for (const FMoviePerformance& Performance : Performances) { if (Performance.RoleId == InRoleId) { return Performance.ObjectiveResult.Score; } }
    return ObjectiveManager && ObjectiveManager->RoleId == InRoleId ? ObjectiveManager->CurrentScore : 0;
}
AReplayActor* AMovieManager::FindReplayForRole(FName InRoleId) const
{
    for (int32 Index=0; Index<Performances.Num(); ++Index)
    { if (Performances[Index].RoleId == InRoleId && ActiveReplays.IsValidIndex(Index)) { return ActiveReplays[Index]; } }
    return nullptr;
}
AInteractableProp* AMovieManager::FindPickupById(FName Id) const
{
    if (Id.IsNone()) { return nullptr; }
    for (TActorIterator<AInteractableProp> It(GetWorld()); It; ++It) { if (It->PickupId == Id) { return *It; } }
    return nullptr;
}
void AMovieManager::ResetWorldProps()
{
    for (TActorIterator<AInteractableProp> It(GetWorld()); It; ++It) { It->ResetForTake(); }
}
void AMovieManager::CalculateMovieScore()
{
    if(IsChronologicalMovie()){StorySession->CalculateStoryScore();return;}
    ScoreSummary = FMovieScoreSummary();
    int32 Total = 0;
    for (const FMoviePerformance& Performance : Performances)
    {
        FMovieRoleScore& Score = ScoreSummary.RoleScores.AddDefaulted_GetRef();
        Score.RoleId = Performance.RoleId; Score.Score = Performance.ObjectiveResult.Score;
        Total += Score.Score;
    }
    ScoreSummary.FinalMovieScore = Total / 3.0;
    MovieResults=UMovieResultsLibrary::EvaluateMovie(Performances,SessionRoles,ScoreSummary,SessionResultsSettings);
    const FMovieScoreSummary Copy = ScoreSummary;
    OnMovieScoreCalculated.Broadcast(Copy);
}
void AMovieManager::ShowObjectiveHUD()
{
    if(bUseUnifiedCustomization&&State==EMovieState::Preparation){if(ActiveObjectiveHUD){ActiveObjectiveHUD->SetVisibility(ESlateVisibility::Collapsed);}return;}
    APlayerController* Player = Controller.Get();
    if (!bShowObjectiveHUD || !Player || !Player->GetLocalPlayer() || !GEngine || !GEngine->GameViewport) { return; }
    if (!ActiveObjectiveHUD)
    {
        UClass* HUDClass = ObjectiveHUDClass ? ObjectiveHUDClass.Get() : UObjectiveHUDWidget::StaticClass();
        ActiveObjectiveHUD = CreateWidget<UObjectiveHUDWidget>(Player, HUDClass);
        if (ActiveObjectiveHUD) { ActiveObjectiveHUD->BindMovie(this); ActiveObjectiveHUD->AddToViewport(40); }
    }
    if (ActiveObjectiveHUD) { ActiveObjectiveHUD->SetVisibility(ESlateVisibility::HitTestInvisible);ActiveObjectiveHUD->RefreshObjectives(); }
}

bool AMovieManager::SelectWardrobeItem(int32 ExpectedPreparationId, ECosmeticCategory Category, FName CosmeticId)
{
    return IsWardrobeOpen() && ExpectedPreparationId == PreparationId && LiveWardrobeComponent->SelectItem(Category, CosmeticId);
}

bool AMovieManager::SetWardrobeBodyColor(int32 ExpectedPreparationId, bool bUseColor, FLinearColor Color)
{
    return IsWardrobeOpen() && ExpectedPreparationId == PreparationId && LiveWardrobeComponent->SetBodyColor(bUseColor, Color);
}

bool AMovieManager::ReadyForTake(int32 ExpectedPreparationId)
{
    if (!IsWardrobeOpen() || ExpectedPreparationId != PreparationId) { return false; }
    const uint32 Generation = SessionGeneration;
    bStartingTake = true; // Latch before widget/delegate callbacks or the timeout in this same frame.
    LiveWardrobeComponent->SetSelectionEnabled(false);
    CloseWardrobePresentation();
    OnWardrobeClosed.Broadcast();
    if (Generation != SessionGeneration || !bSessionActive || State != EMovieState::Preparation) { return false; }
    BeginRecording();
    return Generation == SessionGeneration && bSessionActive && State == EMovieState::Recording;
}

FText AMovieManager::GetCurrentRoleName() const
{
    if(IsChronologicalMovie())
    {
        const int32 Index=StorySession->GetRoleIndex();
        if(Index==0) { return NSLOCTEXT("FinalTake","RoleRobber","Robber"); }
        if(Index==1) { return NSLOCTEXT("FinalTake","RolePolice","Police Officer"); }
        if(Index==2) { return NSLOCTEXT("FinalTake","RoleCivilian","Civilian"); }
        return FText::GetEmpty();
    }
    if (!SessionRoles.IsValidIndex(CurrentTakeIndex)) { return FText::GetEmpty(); }
    const FMovieRoleDefinition& Definition = SessionRoles[CurrentTakeIndex];
    if (!Definition.DisplayName.IsEmpty()) { return Definition.DisplayName; }
    // Older Blueprint role structs retain their IDs and gain useful display metadata.
    const FText Names[] = {NSLOCTEXT("FinalTakeUI","MovieManager.Names.89ebc07e45","Robber"), NSLOCTEXT("FinalTakeUI","MovieManager.Names.a35f3c0224","Police Officer"), NSLOCTEXT("FinalTakeUI","MovieManager.Names.0e85749a6f","Customer")};
    return Names[CurrentTakeIndex];
}

FText AMovieManager::GetCurrentScriptInstruction() const
{
    if(IsChronologicalMovie()) { return StorySession->GetPrimaryObjectiveText(); }
    if (!SessionRoles.IsValidIndex(CurrentTakeIndex)) { return FText::GetEmpty(); }
    const FMovieRoleDefinition& Definition = SessionRoles[CurrentTakeIndex];
    return Definition.ScriptInstruction.IsEmpty() ? NSLOCTEXT("FinalTakeUI","MovieManager.a6310a4300","Improvise this role for the camera. Make every action clear.") : Definition.ScriptInstruction;
}

bool AMovieManager::OpenWardrobePresentation()
{
    APlayerController* Player = Controller.Get();
    // Transient automation worlds have no LocalPlayer/viewport. Their wardrobe flow still runs.
    if (!bShowWardrobeUI || !Player || !Player->GetLocalPlayer() || !GEngine || !GEngine->GameViewport) { return true; }
    const uint32 Generation = SessionGeneration;
    const int32 OpeningPreparationId = PreparationId;
    UClass* WidgetClass = WardrobeWidgetClass ? WardrobeWidgetClass.Get() : UWardrobeWidget::StaticClass();
    if(bUseUnifiedCustomization&&!WidgetClass->IsChildOf(UUnifiedWardrobeWidget::StaticClass())){WidgetClass=UUnifiedWardrobeWidget::StaticClass();}
    UWardrobeWidget* CreatedWidget = CreateWidget<UWardrobeWidget>(Player, WidgetClass);
    if (!CreatedWidget) { return false; }
    if (Generation != SessionGeneration || !IsWardrobeOpen() || PreparationId != OpeningPreparationId) { return true; }
    ActiveWardrobeWidget = CreatedWidget;
    if (bUseWardrobePreviewCamera)
    {
        ACameraActor* Preview = WardrobePreviewCamera;
        if (!IsValid(Preview))
        {
            if (!IsValid(OwnedWardrobeCamera))
            {
                FActorSpawnParameters Parameters;
                Parameters.Owner = this;
                Parameters.ObjectFlags |= RF_Transient;
                Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
                OwnedWardrobeCamera = GetWorld()->SpawnActor<ACameraActor>(Parameters);
            }
            Preview = OwnedWardrobeCamera;
            if (!Preview) { return false; }
            const FTransform Origin = LivePerformer->GetActorTransform();
            const FVector Location = Origin.TransformPositionNoScale(WardrobeCameraOffset);
            Preview->SetActorLocationAndRotation(Location, (Origin.TransformPositionNoScale(WardrobeCameraLookAtOffset) - Location).Rotation());
            Preview->GetCameraComponent()->SetFieldOfView(WardrobeCameraFieldOfView);
        }
        Player->SetViewTargetWithBlend(Preview, 0.0f);
        if (bUseWardrobeFillLight && WardrobeFillLightLumens > 0.0f)
        {
            if (!WardrobeFillLight)
            {
                WardrobeFillLight = NewObject<UPointLightComponent>(this);
                AddInstanceComponent(WardrobeFillLight);
                WardrobeFillLight->SetupAttachment(RootComponent);
                WardrobeFillLight->SetMobility(EComponentMobility::Movable);
                WardrobeFillLight->SetCastShadows(false);
                WardrobeFillLight->SetIntensityUnits(ELightUnits::Lumens);
                WardrobeFillLight->SetAttenuationRadius(700.0f);
                WardrobeFillLight->RegisterComponent();
            }
            const FVector TowardCamera = (Preview->GetActorLocation() - LivePerformer->GetActorLocation()).GetSafeNormal();
            WardrobeFillLight->SetWorldLocation(LivePerformer->GetActorLocation() + TowardCamera * 180.0 + FVector(0,0,60));
            WardrobeFillLight->SetIntensity(WardrobeFillLightLumens);
            WardrobeFillLight->SetVisibility(true);
        }
    }
    ActiveWardrobeWidget->OpenWardrobe(this, PreparationId);
    if (Generation != SessionGeneration || !IsWardrobeOpen() || !ActiveWardrobeWidget) { return true; }
    ActiveWardrobeWidget->AddToViewport(50);
    FInputModeGameAndUI InputMode;
    InputMode.SetWidgetToFocus(ActiveWardrobeWidget->TakeWidget());
    InputMode.SetHideCursorDuringCapture(false);
    InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    Player->SetInputMode(InputMode);
    Player->bShowMouseCursor = true;
    Player->bEnableClickEvents = true;
    Player->bEnableMouseOverEvents = true;
    bWardrobeInputHeld = true;
    ActiveWardrobeWidget->SetKeyboardFocus();
    return true;
}

void AMovieManager::CloseWardrobePresentation()
{
    const bool bHadPresentation = bWardrobeInputHeld || IsValid(ActiveWardrobeWidget);
    if (IsValid(ActiveWardrobeWidget)) { ActiveWardrobeWidget->SetIsEnabled(false); ActiveWardrobeWidget->RemoveFromParent(); }
    ActiveWardrobeWidget = nullptr;
    if (WardrobeFillLight) { WardrobeFillLight->SetVisibility(false); }
    if (APlayerController* Player = Controller.Get(); Player && bHadPresentation)
    {
        Player->SetInputMode(FInputModeGameOnly());
        Player->bShowMouseCursor = bOriginalCursorVisible;
        Player->bEnableClickEvents = bOriginalClickEvents;
        Player->bEnableMouseOverEvents = bOriginalMouseOverEvents;
        AActor* View = OriginalViewTarget.IsValid() ? OriginalViewTarget.Get() : LivePerformer.Get();
        if (View) { Player->SetViewTargetWithBlend(View, 0.0f); }
    }
    bWardrobeInputHeld = false;
    SetLookLocked(false);
}

bool AMovieManager::RestartMovie()
{
    StopMovie();
    OnMovieReset.Broadcast();
    // A Blueprint reset listener may have started the session itself.
    return bSessionActive || StartMovie();
}

void AMovieManager::StopMovie()
{
    bAutoStartAttempted = true;
    ClearSession(true);
    ChangeState(EMovieState::Idle);
}

void AMovieManager::HandleRestartKey() { if (bBindRestartKey) { RestartMovie(); } }

void AMovieManager::HandleActingKey(FKey Key)
{
    if (!bBindActingKeys || State != EMovieState::Recording || !IsValid(LiveActingComponent)) { return; }
    const FKey GestureKeys[] = {EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five};
    const FKey SoundKeys[] = {EKeys::Z, EKeys::X, EKeys::C, EKeys::V, EKeys::B, EKeys::N};
    for (int32 Index = 0; Index < UE_ARRAY_COUNT(GestureKeys); ++Index)
    {
        if (Key == GestureKeys[Index]) { LiveActingComponent->TriggerGesture(static_cast<EActingGesture>(Index)); return; }
    }
    for (int32 Index = 0; Index < UE_ARRAY_COUNT(SoundKeys); ++Index)
    {
        if (Key == SoundKeys[Index]) { LiveActingComponent->TriggerSound(static_cast<EActingSound>(Index)); return; }
    }
}

void AMovieManager::SetPerformerFrozen(bool bFreeze)
{
    APlayerController* Player = Controller.Get();
    if (Player && bFreeze != bMoveLockHeld)
    {
        Player->SetIgnoreMoveInput(bFreeze);
        bMoveLockHeld = bFreeze;
    }
    if (!IsValid(LivePerformer) || bFrozen == bFreeze) { return; }
    LivePerformer->ConsumeMovementInputVector();
    if (ACharacter* Character = Cast<ACharacter>(LivePerformer)) { Character->StopJumping(); }
    if (Movement.IsValid())
    {
        Movement->StopMovementImmediately();
        if (bFreeze)
        {
            if (UCharacterMovementComponent* CharacterMovement = Cast<UCharacterMovementComponent>(Movement.Get()))
            {
                CharacterMovement->DisableMovement();
            }
            Movement->Deactivate();
        }
        else
        {
            if (UCharacterMovementComponent* CharacterMovement = Cast<UCharacterMovementComponent>(Movement.Get()))
            {
                CharacterMovement->SetMovementMode(static_cast<EMovementMode>(OriginalMovementMode), OriginalCustomMovementMode);
            }
            if (bOriginalMovementActive) { Movement->Activate(true); }
        }
    }
    bFrozen = bFreeze;
}

void AMovieManager::SetLookLocked(bool bLocked)
{
    if (APlayerController* Player = Controller.Get(); Player && bLookLockHeld != bLocked)
    {
        Player->SetIgnoreLookInput(bLocked);
        bLookLockHeld = bLocked;
    }
}

bool AMovieManager::PrepareReplays(bool bPlay)
{
    const uint32 Generation = SessionGeneration;
    for (int32 Index = 0; Index < Performances.Num(); ++Index)
    {
        if (!ActiveReplays.IsValidIndex(Index))
        {
            FActorSpawnParameters Parameters;
            Parameters.Owner = this;
            Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            AReplayActor* Replay = GetWorld()->SpawnActor<AReplayActor>(ReplayActorClass, Performances[Index].InitialTransform, Parameters);
            if (!IsValid(Replay)) { return false; }
            ActiveReplays.Add(Replay);
            // Sample the end frame before the manager advances to the next state.
            AddTickPrerequisiteActor(Replay);
        }
        if (!IsValid(ActiveReplays[Index]) || !(bPlay
            ? ActiveReplays[Index]->PlayRecording(Performances[Index].Recording, 0.0)
            : ActiveReplays[Index]->PrepareRecording(Performances[Index].Recording, 0.0)))
        {
            return false;
        }
        if (Generation != SessionGeneration || !bSessionActive) { return false; }
    }
    return true;
}

void AMovieManager::BeginPreparation()
{
    const uint32 Generation = SessionGeneration;
    LivePropInteraction->ResetState(); ResetWorldProps();
    ObjectiveManager->PreviewRole(this, SessionRoles[CurrentTakeIndex].RoleId, SessionRoles[CurrentTakeIndex].Objectives);
    bStartingTake = false;
    PreparationId = PreparationId == MAX_int32 ? 1 : PreparationId + 1;
    LiveActingComponent->ResetPlaybackState();
    if (Generation != SessionGeneration || !bSessionActive) { return; }
    LiveActingComponent->SetLiveActionsEnabled(false);
    if (Generation != SessionGeneration || !bSessionActive) { return; }
    SceneClock->StopClock();
    SetPerformerFrozen(true);
    SetLookLocked(true);
    LivePerformer->SetActorHiddenInGame(bOriginalHidden);
    LivePerformer->SetActorEnableCollision(bOriginalCollision);
    LivePerformer->SetActorTransform(InitialWorldTransforms[CurrentTakeIndex], false, nullptr, ETeleportType::TeleportPhysics);
    if (APlayerController* Player = Controller.Get())
    {
        FRotator View = OriginalControlRotation;
        View.Yaw = InitialWorldTransforms[CurrentTakeIndex].Rotator().Yaw
            + OriginalControlRotation.Yaw - OriginalPawnTransform.Rotator().Yaw;
        Player->SetControlRotation(View);
    }
    const bool bReplaysReady = PrepareReplays(false);
    if (Generation != SessionGeneration || !bSessionActive) { return; }
    if (!bReplaysReady)
    {
        FailMovie(NSLOCTEXT("FinalTakeUI","MovieManager.c044757d96","Could not prepare a replay actor."));
        return;
    }
    if (!LiveWardrobeComponent->BeginSelection(SessionRoles[CurrentTakeIndex].DefaultOutfit))
    {
        FailMovie(NSLOCTEXT("FinalTakeUI","MovieManager.b15fe70bee","Could not apply the role's default outfit."));
        return;
    }
    if (Generation != SessionGeneration || !bSessionActive) { return; }
    PhaseStartWorldTime = GetWorld()->GetTimeSeconds();
    ChangeState(EMovieState::Preparation);
    if (Generation != SessionGeneration || !bSessionActive || State != EMovieState::Preparation) { return; }
    ShowObjectiveHUD();
    if (Generation != SessionGeneration || !bSessionActive || State != EMovieState::Preparation) { return; }
    if (!OpenWardrobePresentation()) { FailMovie(NSLOCTEXT("FinalTakeUI","MovieManager.41c1dd63d7","Could not create the wardrobe UI or preview camera.")); return; }
    if (Generation != SessionGeneration || !IsWardrobeOpen()) { return; }
    OnPreparationStarted.Broadcast(GetCurrentTakeNumber(), GetCurrentRoleId(), SessionPreparationDuration);
}

void AMovieManager::BeginRecording()
{
    if(IsChronologicalMovie()) { StorySession->BeginRecording(); return; }
    if (!bSessionActive || State != EMovieState::Preparation || !bStartingTake || Recorder->IsRecording()) { return; }
    const uint32 Generation = SessionGeneration;
    // Every start reads the same UWorld time in this frame. Preparation never advances the scene clock.
    SceneClock->StopClock();
    LivePerformer->SetActorTransform(InitialWorldTransforms[CurrentTakeIndex], false, nullptr, ETeleportType::TeleportPhysics);
    if (!Recorder->StartRecording(SessionRecordingDuration, true))
    {
        FailMovie(NSLOCTEXT("FinalTakeUI","MovieManager.b88070a5df","Could not start synchronized recording/playback."));
        return;
    }
    SceneClock->StartClock(0.0);
    const bool bReplaysReady = PrepareReplays(true);
    if (Generation != SessionGeneration || !bSessionActive) { return; }
    if (!bReplaysReady)
    {
        FailMovie(NSLOCTEXT("FinalTakeUI","MovieManager.4a12f3f98a","Could not start synchronized playback."));
        return;
    }
    SetPerformerFrozen(false);
    LivePropInteraction->BeginTake(this);
    ObjectiveManager->BeginTake(LivePerformer, LiveActingComponent);
    PhaseStartWorldTime = GetWorld()->GetTimeSeconds();
    ChangeState(EMovieState::Recording);
    ShowObjectiveHUD();
    if (Generation != SessionGeneration || !bSessionActive || State != EMovieState::Recording) { return; }
    LiveActingComponent->SetLiveActionsEnabled(true);
    if (Generation != SessionGeneration || !bSessionActive || State != EMovieState::Recording) { return; }
    ObjectiveManager->EvaluateContinuous(0.0, LivePerformer->GetActorLocation());
    if (Generation != SessionGeneration || !bSessionActive || State != EMovieState::Recording) { return; }
    if (ActiveObjectiveHUD) { ActiveObjectiveHUD->RefreshObjectives(); }
    if (Generation != SessionGeneration || !bSessionActive || State != EMovieState::Recording) { return; }
    OnRecordingStarted.Broadcast(GetCurrentTakeNumber(), GetCurrentRoleId(), SessionRecordingDuration);
}

void AMovieManager::BeginTakeTransition()
{
    if (!bSessionActive) { return; }
    for (AReplayActor* Replay:ActiveReplays) { if (IsValid(Replay)) { Replay->StopPlayback(false); } }
    SceneClock->StopClock();
    SetPerformerFrozen(true); SetLookLocked(true);
    PhaseStartWorldTime=GetWorld()->GetTimeSeconds();
    ChangeState(EMovieState::TakeTransition);
}

void AMovieManager::HandleRecordingFinished(const FActingRecording& Recording)
{
    if(IsChronologicalMovie()) { StorySession->CaptureFinishedSegment(Recording); return; }
    const uint32 Generation = SessionGeneration;
    if (!bSessionActive || State != EMovieState::Recording || bPendingTakeFinished) { return; }
    if (!Recording.IsValid() || !FMath::IsNearlyEqual(Recording.GetDurationSeconds(), SessionRecordingDuration, 1.e-6))
    {
        FailMovie(NSLOCTEXT("FinalTakeUI","MovieManager.61435e5eb7","The recorder returned invalid data or ended before the configured take duration."));
        return;
    }
    const FTakeObjectiveResult ObjectiveResult = ObjectiveManager->FinishTake(Recording.GetDurationSeconds(), Recording.Samples.Last().WorldTransform.GetLocation());
    if (Generation != SessionGeneration || !bSessionActive || State != EMovieState::Recording) { return; }
    LivePropInteraction->EndTake();
    FMoviePerformance& Performance = Performances.AddDefaulted_GetRef();
    Performance.RoleId = SessionRoles[CurrentTakeIndex].RoleId;
    Performance.InitialTransform = InitialWorldTransforms[CurrentTakeIndex];
    Performance.Recording = Recording;
    Performance.ObjectiveResult = ObjectiveResult;
    bPendingTakeFinished = true;
    SetPerformerFrozen(true);
    LiveActingComponent->StopAllActions();
    if (Generation != SessionGeneration || !bSessionActive) { return; }
    LiveActingComponent->SetLiveActionsEnabled(false);
    // Complete the transition in the manager's late tick, after replay actors sampled this frame.
}

bool AMovieManager::PrepareCamera()
{
    if (IsValid(SessionCamera)) { return true; }
    SessionCamera = PremiereCamera;
    if (IsValid(SessionCamera)) { return true; }
    FActorSpawnParameters Parameters;
    Parameters.Owner = this;
    Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    const FVector Location = MovieOrigin.TransformPosition(AutomaticCameraOffset);
    FVector Center = FVector::ZeroVector;
    for (const FTransform& Transform : InitialWorldTransforms) { Center += Transform.GetLocation(); }
    Center /= InitialWorldTransforms.Num();
    OwnedCamera = GetWorld()->SpawnActor<ACameraActor>(Location, (Center - Location).Rotation(), Parameters);
    if (!IsValid(OwnedCamera)) { return false; }
    OwnedCamera->GetCameraComponent()->SetFieldOfView(60.0f);
    SessionCamera = OwnedCamera;
    return true;
}

void AMovieManager::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    const AStudioGameMode* StudioMode=GetWorld()?GetWorld()->GetAuthGameMode<AStudioGameMode>():nullptr;
    if (bAutoStart && !bAutoStartAttempted && (!StudioMode || !StudioMode->ShouldUseFrontEnd()))
    {
        if (APlayerController* Player = UGameplayStatics::GetPlayerController(this, PlayerIndex); Player && Player->GetPawn())
        {
            StartMovie();
        }
    }
    if(bSessionActive && IsChronologicalMovie() && State!=EMovieState::MoviePremiere && State!=EMovieState::Finished)
    {
        StorySession->UpdateStory(DeltaSeconds);
        if(ActiveObjectiveHUD) { ActiveObjectiveHUD->RefreshObjectives(); }
        UpdatePresentationAudioMix(); UpdateDebugOverlay();
        return;
    }
    if (bSessionActive)
    {
        if(IsChronologicalMovie() && State==EMovieState::MoviePremiere){StorySession->UpdateScenePlayback();if(MovieDirector && PresentationPhase==EMoviePresentationPhase::Playback){MovieDirector->Update(DeltaSeconds);}}
        if (!Controller.IsValid() || !IsValid(LivePerformer) || Controller->GetPawn() != LivePerformer
            || !IsValid(Recorder) || !IsValid(LiveWardrobeComponent))
        {
            FailMovie(NSLOCTEXT("FinalTakeUI","MovieManager.e0bbde121f","The movie's live performer or player possession was lost."));
        }
        else if (State == EMovieState::Preparation && GetPhaseRemainingSeconds() <= 0.0)
        {
            ReadyForTake(PreparationId);
        }
        else if (State == EMovieState::Recording && bPendingTakeFinished)
        {
            bPendingTakeFinished = false;
            const uint32 Generation = SessionGeneration;
            const FMoviePerformance CompletedPerformance = Performances.Last();
            OnTakeRecorded.Broadcast(CompletedPerformance);
            if (Generation == SessionGeneration && bSessionActive && State == EMovieState::Recording)
            {
                UE_LOG(LogMovieManager, Display, TEXT("Take %d / %s captured: %d samples, %.3f seconds."),
                    GetCurrentTakeNumber(), *CompletedPerformance.RoleId.ToString(), CompletedPerformance.Recording.Samples.Num(),
                    CompletedPerformance.Recording.GetDurationSeconds());
                if (++CurrentTakeIndex < SessionRoles.Num())
                { if (SessionBetweenActors>0.0) { BeginTakeTransition(); } else { BeginPreparation(); } }
                else
                {
                    CurrentTakeIndex = INDEX_NONE; CalculateMovieScore();
                    if (Generation == SessionGeneration && bSessionActive)
                    { if (SessionWrap>0.0) { BeginTakeTransition(); } else { BeginPremiere(); } }
                }
            }
        }
        else if (State == EMovieState::Recording && !Recorder->IsRecording())
        {
            FailMovie(NSLOCTEXT("FinalTakeUI","MovieManager.bc55d9c9ee","The movie recorder was cancelled by another system."));
        }
        else if (State == EMovieState::Recording)
        {
            ObjectiveManager->EvaluateContinuous(GetSceneTimeSeconds(), LivePerformer->GetActorLocation());
        }
        else if (State==EMovieState::TakeTransition && GetPhaseRemainingSeconds()<=0.0)
        { if (CurrentTakeIndex==INDEX_NONE) { BeginPremiere(); } else { BeginPreparation(); } }
        else if (State == EMovieState::MoviePremiere)
        {
            if (PresentationPhase==EMoviePresentationPhase::TitleCard && GetPhaseRemainingSeconds()<=0.0) { StartPremierePlayback(); }
            else if (PresentationPhase==EMoviePresentationPhase::Playback && GetPhaseRemainingSeconds()<=0.0) { FinishPremiere(); }
        }
    }
    if (ActiveObjectiveHUD) { ActiveObjectiveHUD->RefreshObjectives(); }
    if (ActiveMoviePresentation) { ActiveMoviePresentation->RefreshPresentation(); }
    UpdatePresentationAudioMix();
    UpdateDebugOverlay();
}

void AMovieManager::ClearSession(bool bRestorePlayer)
{
    if (bClearingSession) { return; }
    TGuardValue<bool> Guard(bClearingSession, true);
    ++SessionGeneration;
    const bool bHadSession = bSessionActive;
    bSessionActive = false;
    PresentationId=PresentationId==MAX_int32?1:PresentationId+1;
    CloseMoviePresentation();
    StopPresentationAudio();
    if(MovieDirector){MovieDirector->Stop();}
    if (bGameplayHUDHidden && OriginalHUD.IsValid()) { OriginalHUD->bShowHUD=bOriginalHUDVisible; }
    bGameplayHUDHidden=false; OriginalHUD.Reset();
    PresentationPhase=EMoviePresentationPhase::None; MovieResults=FMovieResults();
    bPendingTakeFinished = false;
    ObjectiveManager->ResetObjectives();
    if (LivePropInteraction) { LivePropInteraction->ResetState(); }
    LivePropInteraction = nullptr;
    ScoreSummary = FMovieScoreSummary();
    if (ActiveObjectiveHUD) { ActiveObjectiveHUD->RemoveFromParent(); ActiveObjectiveHUD = nullptr; }
    if (bHadSession && !IsChronologicalMovie()) { ResetWorldProps(); }
    CloseWardrobePresentation();
    if(StorySession && StorySession->IsStoryActive()) { StorySession->ResetSession(bRestorePlayer); }
    if (IsValid(LiveWardrobeComponent))
    {
        LiveWardrobeComponent->ClearAppearance();
        LiveWardrobeComponent->Catalog = OriginalCosmeticCatalog;
        LiveWardrobeComponent->DefaultSelection = OriginalDefaultOutfit;
        if (OriginalAppearance.bHasAppearance)
        {
            LiveWardrobeComponent->ApplySnapshot(OriginalAppearance, LiveWardrobeComponent->TargetMesh, LiveWardrobeComponent->FallbackBodyMesh, false);
        }
        LiveWardrobeComponent->SetReadOnly(false);
        LiveWardrobeComponent->SetSelectionEnabled(bOriginalWardrobeEnabled);
    }
    if (IsValid(LiveActingComponent))
    {
        LiveActingComponent->ResetPlaybackState();
        LiveActingComponent->bRequireActiveRecording = bOriginalRequireRecording;
        LiveActingComponent->SetLiveActionsEnabled(bOriginalLiveActionsEnabled);
    }
    if (IsValid(Recorder))
    {
        Recorder->OnRecordingFinished.RemoveDynamic(this, &AMovieManager::HandleRecordingFinished);
        Recorder->ClearRecording();
    }
    if (bHadSession && IsValid(SceneClock)) { SceneClock->StopClock(); }
    for (AReplayActor* Replay : ActiveReplays)
    {
        if (IsValid(Replay)) { RemoveTickPrerequisiteActor(Replay); Replay->Destroy(); }
    }
    ActiveReplays.Reset();
    Performances.Reset();
    if (Movement.IsValid()) { Movement->StopMovementImmediately(); }
    SetPerformerFrozen(false);
    SetLookLocked(false);
    if (APlayerController* Player = Controller.Get())
    {
        DisableInput(Player);
        if (bRestorePlayer)
        {
            Player->SetControlRotation(OriginalControlRotation);
            AActor* View = OriginalViewTarget.Get();
            if (!IsValid(View) || Player->GetPawn() != LivePerformer) { View = Player->GetPawn(); }
            if (View) { Player->SetViewTargetWithBlend(View, 0.0f); }
        }
    }
    if (bRestorePlayer && IsValid(LivePerformer))
    {
        LivePerformer->SetActorHiddenInGame(bOriginalHidden);
        LivePerformer->SetActorEnableCollision(bOriginalCollision);
        LivePerformer->SetActorTransform(OriginalPawnTransform, false, nullptr, ETeleportType::TeleportPhysics);
    }
    if (IsValid(OwnedCamera)) { OwnedCamera->Destroy(); }
    if (IsValid(OwnedWardrobeCamera)) { OwnedWardrobeCamera->Destroy(); }
    if (WardrobeFillLight) { RemoveInstanceComponent(WardrobeFillLight); WardrobeFillLight->DestroyComponent(); WardrobeFillLight = nullptr; }
    OwnedWardrobeCamera = nullptr;
    OwnedCamera = nullptr;
    SessionCamera = nullptr;
    Recorder = nullptr;
    SceneClock = nullptr;
    Controller.Reset();
    OriginalViewTarget.Reset();
    Movement.Reset();
    LivePerformer = nullptr;
    LiveActingComponent = nullptr;
    LiveWardrobeComponent = nullptr;
    OriginalAppearance = FRecordedAppearance();
    OriginalCosmeticCatalog.Reset();
    CurrentTakeIndex = INDEX_NONE;
    SessionRoles.Reset();
    InitialWorldTransforms.Reset();
    SessionPremiereSettings=FMoviePremiereSettings(); SessionResultsSettings=FMovieResultsSettings();
    SessionPreparationDuration=SessionRecordingDuration=PremiereDuration=PhaseStartWorldTime=0.0;
    SessionBetweenActors=SessionWrap=0.0;
    bFrozen = bMoveLockHeld = bLookLockHeld = false;
}

void AMovieManager::FailMovie(const FText& Error)
{
    ClearSession(true);
    LastError = Error;
    UE_LOG(LogMovieManager, Warning, TEXT("%s"), *Error.ToString());
    const uint32 Generation = SessionGeneration;
    ChangeState(EMovieState::Error);
    if (Generation == SessionGeneration && State == EMovieState::Error) { OnMovieError.Broadcast(Error); }
}

void AMovieManager::ChangeState(EMovieState NewState)
{
    if (State == NewState) { return; }
    const EMovieState PreviousState = State;
    State = NewState;
    OnStateChanged.Broadcast(PreviousState, State);
}

void AMovieManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    ClearSession(EndPlayReason == EEndPlayReason::Destroyed);
    if (GEngine) { GEngine->RemoveOnScreenDebugMessage(GetUniqueID()); }
    Super::EndPlay(EndPlayReason);
}

int32 AMovieManager::GetCurrentTakeNumber() const
{
    if(IsChronologicalMovie()) { return StorySession->GetTurnNumber(); }
    return SessionRoles.IsValidIndex(CurrentTakeIndex) ? CurrentTakeIndex + 1 : 0;
}

FName AMovieManager::GetCurrentRoleId() const
{
    if(IsChronologicalMovie()) { const int32 Index=StorySession->GetRoleIndex(); return SessionRoles.IsValidIndex(Index)?SessionRoles[Index].RoleId:NAME_None; }
    return SessionRoles.IsValidIndex(CurrentTakeIndex) ? SessionRoles[CurrentTakeIndex].RoleId : NAME_None;
}

double AMovieManager::GetPhaseDurationSeconds() const
{
    if (State == EMovieState::Preparation) { return SessionPreparationDuration; }
    if (State == EMovieState::Recording) { return SessionRecordingDuration; }
    if (State==EMovieState::TakeTransition) { return CurrentTakeIndex==INDEX_NONE?SessionWrap:SessionBetweenActors; }
    if (State == EMovieState::MoviePremiere && PresentationPhase==EMoviePresentationPhase::TitleCard) { return SessionPremiereSettings.GetTitleDuration(); }
    if (State == EMovieState::MoviePremiere || State == EMovieState::Finished) { return PremiereDuration; }
    return 0.0;
}

double AMovieManager::GetPhaseElapsedSeconds() const
{
    if(IsChronologicalMovie() && State==EMovieState::Recording) { return StorySession->GetTurnElapsed(); }
    if (State==EMovieState::Finished) { return GetPhaseDurationSeconds(); }
    if (!bSessionActive || !GetWorld()) { return 0.0; }
    return FMath::Clamp(GetWorld()->GetTimeSeconds() - PhaseStartWorldTime, 0.0, GetPhaseDurationSeconds());
}

double AMovieManager::GetPhaseRemainingSeconds() const
{
    return GetPhaseDurationSeconds() - GetPhaseElapsedSeconds();
}

double AMovieManager::GetSceneTimeSeconds() const
{
    return IsValid(SceneClock) ? SceneClock->GetSceneTimeSeconds() : 0.0;
}

void AMovieManager::UpdateDebugOverlay() const
{
    if (!bShowDebugOverlay || !GEngine || !bSessionActive) { return; }
    if (State==EMovieState::MoviePremiere || State==EMovieState::Finished)
    { GEngine->RemoveOnScreenDebugMessage(GetUniqueID()); return; }
    FString Message;
    if (State == EMovieState::Preparation || State == EMovieState::Recording)
    {
        Message = FString::Printf(TEXT("%s | Take %d/3 | %s | %.1f seconds remaining\nSaved takes: %d | R restarts the entire movie"),
            State == EMovieState::Preparation ? TEXT("Wardrobe / Preparation") : TEXT("Recording"), GetCurrentTakeNumber(),
            *GetCurrentRoleId().ToString(), GetPhaseRemainingSeconds(), Performances.Num());
    }
    else
    {
        Message = FString::Printf(TEXT("%s | %.1f seconds remaining | R restarts the entire movie"),
            State == EMovieState::Finished ? TEXT("Movie finished") : TEXT("Movie premiere"), GetPhaseRemainingSeconds());
    }
    if (bBindActingKeys && IsValid(LiveActingComponent))
    {
        Message += FString::Printf(TEXT("\n1 Dance | 2 Point | 3 Scared | 4 Faint | 5 Arrest\nZ Laugh | X Scream | C Cry | V Siren | B Kiss | N Failure\nGesture: %s | Events: %d (assign assets on ActingComponent)"),
            *LiveActingComponent->ActiveGestureId.ToString(), LiveActingComponent->DispatchedEventCount);
    }
    GEngine->AddOnScreenDebugMessage(static_cast<uint64>(GetUniqueID()), 0.1f, FColor::Cyan, Message);
}
