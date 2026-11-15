#if !UE_BUILD_SHIPPING
#include "ActingComponent.h"
#include "AudioMixerBlueprintLibrary.h"
#include "ReplayActor.h"
#include "ActingRecorderComponent.h"
#include "ActingSceneClockSubsystem.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"

#include "Components/SkeletalMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformTime.h"
#include "InputKeyEventArgs.h"
#include "Kismet/GameplayStatics.h"

#include "Misc/CommandLine.h"
#include "Misc/App.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "MovieManager.h"
#include "ObjectiveManagerComponent.h"
#include "StudioAnimInstance.h"
#include "UObject/UObjectIterator.h"
#include "UnifiedWardrobeWidget.h"
#include "UnrealClient.h"
#include "WardrobeComponent.h"
#include "WardrobePreviewActor.h"
#include "Widgets/SWindow.h"

namespace CustomizationSurvey
{
class FRun
{
public:
    double Started = FPlatformTime::Seconds(), At = 0;
    int32 Step = 0, Shots = 0;
    bool bSetup = false, bDone = false;
    FString Directory;
    const TArray<FString> Actions = {TEXT("Action.Reset"),
                                     TEXT("@Faceless"),
                                     TEXT("Item.Eyes.Classic"),
                                     TEXT("Tab.Mouth"),
                                     TEXT("Item.Mouth.Smile"),
                                     TEXT("Tab.Eyebrows"),
                                     TEXT("Item.Brows.Neutral"),
                                     TEXT("@FaceFront"),
                                     TEXT("Rotate.Left"),
                                     TEXT("Rotate.Left"),
                                     TEXT("@FaceThreeQuarter"),
                                     TEXT("Rotate.Left"),
                                     TEXT("Rotate.Left"),
                                     TEXT("@FaceSide"),
                                     TEXT("Rotate.Right"),
                                     TEXT("Rotate.Right"),
                                     TEXT("Rotate.Right"),
                                     TEXT("Rotate.Right"),
                                     TEXT("Tab.Skin"),
                                     TEXT("Skin.Purple"),
                                     TEXT("@Skin"),
                                     TEXT("Tab.Clothing"),
                                     TEXT("Item.Body.Uniform"),
                                     TEXT("@Clothing"),
                                     TEXT("Tab.Hats"),
                                     TEXT("Item.Head.PoliceCap"),
                                     TEXT("@Hat"),
                                     TEXT("Tab.Accessories"),
                                     TEXT("Item.Face.Mustache"),
                                     TEXT("Tab.Objects"),
                                     TEXT("Hand.Left"),
                                     TEXT("Item.Prop.Banana"),
                                     TEXT("Hand.Right"),
                                     TEXT("Item.Prop.MoneyBag"),
                                     TEXT("@BothProps"),
                                     TEXT("Action.Random"),
                                     TEXT("@Random"),
                                     TEXT("Action.Reset"),
                                     TEXT("@Reset")};
    void Finish(bool Pass, const TCHAR* Message, AMovieManager* Movie)
    {
        if (bDone)
        {
            return;
        }
        bDone = true;
        IFileManager::Get().MakeDirectory(*Directory, true);
        FFileHelper::SaveStringToFile(
            FString::Printf(TEXT("{\"passed\":%s,\"steps\":%d,\"screenshots\":%d,\"message\":\"%s\"}"),
                            Pass ? TEXT("true") : TEXT("false"), Step, Shots, Message),
            *(Directory / TEXT("result.json")));
        if (Movie)
        {
            Movie->StopMovie();
        }
        UE_LOG(LogTemp, Display, TEXT("CustomizationSurvey %s: %s"), Pass ? TEXT("PASS") : TEXT("FAIL"), Message);
        FPlatformMisc::RequestExitWithStatus(false, Pass ? 0 : 1);
    }
    bool Click(UCustomizationOptionButton* Button)
    {
        if (!Button || Button->GetCachedGeometry().GetLocalSize().X < 1)
        {
            return false;
        }
        const auto Geometry = Button->GetCachedGeometry();
        const FVector2D Point = Geometry.LocalToAbsolute(Geometry.GetLocalSize() * .5);
        auto& Slate = FSlateApplication::Get();
        Slate.SetCursorPos(Point);
        const TSet<FKey> Empty;
        FPointerEvent Move(0, Point, Point, Empty, EKeys::Invalid, 0, FModifierKeysState());
        Slate.ProcessMouseMoveEvent(Move, true);
        const TSet<FKey> Down = {EKeys::LeftMouseButton};
        FPointerEvent Press(0, Point, Point, Down, EKeys::LeftMouseButton, 0, FModifierKeysState());
        const auto Window = GEngine->GameViewport->GetWindow();
        Slate.ProcessMouseButtonDownEvent(Window->GetNativeWindow(), Press);
        FPointerEvent Release(0, Point, Point, Empty, EKeys::LeftMouseButton, 0, FModifierKeysState());
        Slate.ProcessMouseButtonUpEvent(Release);
        return true;
    }
    void Tick(UWorld* World)
    {
        if (bDone)
        {
            return;
        }
        AMovieManager* Movie = nullptr;
        for (TActorIterator<AMovieManager> I(World); I; ++I)
        {
            Movie = *I;
            break;
        }
        if (FPlatformTime::Seconds() - Started > 120)
        {
            Finish(false, TEXT("Customization survey timed out"), Movie);
            return;
        }
        if (!Movie || !Movie->LivePerformer)
        {
            return;
        }
        if (!bSetup)
        {
            Movie->StopMovie();
            Movie->PreparationDurationSeconds = 60;
            Movie->RecordingDurationSeconds = 2;
            if (!Movie->StartMovie())
            {
                Finish(false, TEXT("Could not start customization test"), Movie);
                return;
            }
            bSetup = true;
            At = FPlatformTime::Seconds();
            return;
        }
        auto* Widget = Cast<UUnifiedWardrobeWidget>(Movie->ActiveWardrobeWidget);
        if (!Widget || !Widget->PreviewActor || !Widget->GetPreviewMaterial())
        {
            return;
        }
        int32 Visible = 0;
        for (TObjectIterator<UWardrobeWidget> I; I; ++I)
        {
            if (I->IsInViewport())
            {
                ++Visible;
            }
        }
        const auto* Clock = World->GetSubsystem<UActingSceneClockSubsystem>();
        const auto* Recorder = Movie->LivePerformer->FindComponentByClass<UActingRecorderComponent>();
        if (Visible != 1 || !Movie->IsWardrobeOpen() || Clock->IsClockRunning() || Clock->GetSceneTimeSeconds() != 0 ||
            Movie->ObjectiveManager->CurrentScore != 0 || (Recorder && Recorder->IsRecording()))
        {
            Finish(false, TEXT("Duplicate UI, scoring or recording during customization"), Movie);
            return;
        }
        if (FPlatformTime::Seconds() - At < .28 || FScreenshotRequest::IsScreenshotRequested())
        {
            return;
        }
        if (Actions.IsValidIndex(Step))
        {
            const FString Action = Actions[Step];
            if (Action.StartsWith(TEXT("@")))
            {
                const auto S = Movie->LiveWardrobeComponent->GetSelection();
                const bool Face = S.EyeId == TEXT("Eyes.Classic") && S.MouthId == TEXT("Mouth.Smile") &&
                                  S.EyebrowId == TEXT("Brows.Neutral");
                if ((Action == TEXT("@FaceFront") && !Face) ||
                    (Action == TEXT("@Skin") && (!Face || !S.SkinColor.Equals(FLinearColor(.45, .22, .85)))) ||
                    (Action == TEXT("@Clothing") &&
                     (!Face || S.BodyId != TEXT("Body.Uniform") || !S.SkinColor.Equals(FLinearColor(.45, .22, .85)))) ||
                    (Action == TEXT("@BothProps") &&
                     (S.LeftHandPropId != TEXT("Prop.Banana") || S.PropId != TEXT("Prop.MoneyBag") ||
                      S.HeadId != TEXT("Head.PoliceCap") || S.FaceId != TEXT("Face.Mustache"))))
                {
                    Finish(false, TEXT("Pointer selections did not persist independently across tabs"), Movie);
                    return;
                }
                IFileManager::Get().MakeDirectory(*Directory, true);
                FScreenshotRequest::RequestScreenshot(Directory / (Action.Mid(1) + TEXT(".png")), true, false);
                ++Shots;
            }
            else if (!Click(Widget->FindControl(FName(*Action))))
            {
                Finish(false, TEXT("A native customization control was missing"), Movie);
                return;
            }
            ++Step;
            At = FPlatformTime::Seconds();
            return;
        }
        const auto S = Movie->LiveWardrobeComponent->GetSelection();
        const bool White = S.SkinColor.Equals(FLinearColor::White) && S.EyeId.IsNone() && S.MouthId.IsNone() &&
                           S.EyebrowId.IsNone() && S.HeadId.IsNone() && S.FaceId.IsNone() && S.PropId.IsNone() &&
                           S.LeftHandPropId.IsNone() && S.BodyId == TEXT("Body.None");
        Finish(White && Movie->LiveWardrobeComponent->CaptureAppearance().IsValid(),
               TEXT("Native tabs, two hands, 3D portrait rotation, random/reset and preparation isolation verified"),
               Movie);
    }
};
TSharedPtr<FRun> Run;
class FMotionRun
{
public:
    FString Directory;
    double Started = FPlatformTime::Seconds();
    bool Setup = false, Done = false, Idle = false, Walk = false, RunFast = false, Air = false, JumpSent = false,
         Released = false;
    int32 Gesture = 0, SoundInput = 0;
    bool bCaptureAudio=false;
    int32 Phase = 0, CapturedEvents = 0;
    bool ReplayAir = false;
    TWeakObjectPtr<ACameraActor> Camera;
    TWeakObjectPtr<APlayerController> Player;
    TArray<FKey> PendingRelease;
    TSet<FString> Shots;
    void Finish(bool Pass, const TCHAR* Message, AMovieManager* M)
    {
        if (Done)
        {
            return;
        }
        Done = true;
        if(bCaptureAudio && M){UAudioMixerBlueprintLibrary::StopRecordingOutput(M,EAudioRecordingExportType::WavFile,TEXT("ActingSoundboardMix"),Directory);}
        const auto B = [](bool V) { return V ? TEXT("true") : TEXT("false"); };
        const int32 Events =
            CapturedEvents ? CapturedEvents
            : M && M->LiveActingComponent
                ? M->LivePerformer->FindComponentByClass<UActingRecorderComponent>()->GetRecordingData().Events.Num()
                : 0;
        IFileManager::Get().MakeDirectory(*Directory, true);
        FFileHelper::SaveStringToFile(
            FString::Printf(
                TEXT("{\"passed\":%s,\"idle\":%s,\"walk\":%s,\"run\":%s,\"jump\":"
                     "%s,\"gestureEvents\":%d,\"replayJump\":%s,\"replayAndSecondTake\":%s,\"message\":\"%s\"}"),
                B(Pass), B(Idle), B(Walk), B(RunFast), B(Air), Events, B(ReplayAir), B(Phase == 2 && Pass), Message),
            *(Directory / TEXT("result.json")));
        if (M)
        {
            M->StopMovie();
        }
        UE_LOG(LogTemp, Display, TEXT("CustomizationMotion %s: %s"), Pass ? TEXT("PASS") : TEXT("FAIL"), Message);
        FPlatformMisc::RequestExitWithStatus(false, Pass ? 0 : 1);
    }
    void Capture(const FString& Name)
    {
        if (!Shots.Contains(Name) && !FScreenshotRequest::IsScreenshotRequested())
        {
            Shots.Add(Name);
            IFileManager::Get().MakeDirectory(*Directory, true);
            FScreenshotRequest::RequestScreenshot(Directory / (Name + TEXT(".png")), true, false);
        }
    }
    void Press(FKey Key)
    {
        Player->InputKey(FInputKeyEventArgs(nullptr, INPUTDEVICEID_NONE, Key, IE_Pressed, FPlatformTime::Cycles64()));
        PendingRelease.Add(Key);
    }
    bool AttachmentsMatch(USkeletalMeshComponent* Mesh,UWardrobeComponent* Wardrobe,const TArray<FCosmeticDefinition>& Definitions)
    {
        for(const UMeshComponent* Part:Wardrobe->CosmeticMeshes)
        {
            for(const auto& Item:Definitions)
            {
                if(!Part->ComponentHasTag(Item.Id)){continue;}
                const bool Left=Part->GetAttachSocketName()==TEXT("LeftHand");
                if(Item.Category==ECosmeticCategory::LeftProp && !Left){continue;}
                if(Item.Category==ECosmeticCategory::Prop && Left && !Item.bUseLeftHandTransform){continue;}
                const FName Bone=Left?FName(TEXT("LeftHand")):Item.SocketName;
                const FTransform& Offset=Left && Item.bUseLeftHandTransform?Item.LeftHandRelativeTransform:Item.RelativeTransform;
                if(Part->GetAttachParent()!=Mesh || Part->GetAttachSocketName()!=Bone || !Part->GetRelativeTransform().Equals(Offset,.001f)
                    || Part->GetCollisionEnabled()!=ECollisionEnabled::NoCollision){return false;}
            }
        }
        return true;
    }
    void Tick(UWorld* W)
    {
        if (Done)
        {
            return;
        }
        AMovieManager* M = nullptr;
        for (TActorIterator<AMovieManager> I(W); I; ++I)
        {
            M = *I;
            break;
        }
        if (FPlatformTime::Seconds() - Started > 90)
        {
            Finish(false, TEXT("Motion survey timed out"), M);
            return;
        }
        if (!M || !M->LivePerformer)
        {
            return;
        }
        if (!Setup)
        {
            Player = UGameplayStatics::GetPlayerController(W, 0);
            M->StopMovie();
            M->PreparationDurationSeconds = 60;
            M->RecordingDurationSeconds = 18;
            if (!M->StartMovie())
            {
                Finish(false, TEXT("Motion setup failed"), M);
                return;
            }
            bCaptureAudio=FParse::Param(FCommandLine::Get(),TEXT("CorrectionAudio"));
            if(bCaptureAudio){FApp::SetUnfocusedVolumeMultiplier(1.f);FApp::SetVolumeMultiplier(1.f);UAudioMixerBlueprintLibrary::StartRecordingOutput(W,40);}
            FCosmeticSelection S;
            S.BodyId = TEXT("Body.None");
            S.EyeId = TEXT("Eyes.Classic");
            S.MouthId = TEXT("Mouth.Smile");
            S.EyebrowId = TEXT("Brows.Neutral");
            S.SkinColor = FLinearColor(.45, .22, .85);
            FString Head,Face,Prop;
            if(FParse::Value(FCommandLine::Get(),TEXT("CorrectionHead="),Head)){S.HeadId=FName(*Head);}
            if(FParse::Value(FCommandLine::Get(),TEXT("CorrectionFace="),Face)){S.FaceId=FName(*Face);}
            if(FParse::Value(FCommandLine::Get(),TEXT("CorrectionProp="),Prop)){S.PropId=FName(*Prop);S.LeftHandPropId=S.PropId;}
            M->LiveWardrobeComponent->SetSelection(S);
            if (!M->ReadyForTake(M->PreparationId))
            {
                Finish(false, TEXT("Ready failed"), M);
                return;
            }
            Camera = W->SpawnActor<ACameraActor>();
            Camera->GetCameraComponent()->FieldOfView = 40;
            Player->SetViewTarget(Camera.Get());
            Setup = true;
            return;
        }
        for (FKey Key : PendingRelease)
        {
            Player->InputKey(
                FInputKeyEventArgs(nullptr, INPUTDEVICEID_NONE, Key, IE_Released, FPlatformTime::Cycles64()));
        }
        PendingRelease.Reset();
        if (Phase == 1)
        {
            if (M->IsWardrobeOpen() && M->CurrentTakeIndex == 1)
            {
                if (!M->ReadyForTake(M->PreparationId) || M->ActiveReplays.Num() != 1)
                {
                    Finish(false, TEXT("Concurrent replay setup failed"), M);
                    return;
                }
                M->LivePerformer->SetActorHiddenInGame(true);
                Player->SetViewTarget(Camera.Get());
                Phase = 2;
            }
            return;
        }
        if (Phase == 2)
        {
            const double ReplayTime = M->GetSceneTimeSeconds();
            AReplayActor* Ghost = M->ActiveReplays[0];
            if(!AttachmentsMatch(Ghost->PerformanceMesh,Ghost->Wardrobe,M->Performances[0].Recording.Appearance.Items))
            {Finish(false,TEXT("Replay cosmetic attachment changed during animated playback"),M);return;}
            const FVector Target = Ghost->GetActorLocation() + FVector(0, 0, 5);
            const FVector Location = Target + Ghost->GetActorForwardVector() * 650 + FVector(0, 0, 25);
            Camera->SetActorLocationAndRotation(Location, (Target - Location).Rotation());
            if (const auto* Anim = Cast<UStudioAnimInstance>(Ghost->PerformanceMesh->GetAnimInstance()))
            {
                ReplayAir |= Anim->bIsInAir;
            }
            if (ReplayTime > 1.15 && ReplayTime < 1.6)
            {
                Capture(TEXT("ReplayJump"));
            }
            if (ReplayTime > 3 && ReplayTime < 4)
            {
                Capture(TEXT("ReplayDance"));
            }
            if (ReplayTime > 12.1)
            {
                Capture(TEXT("ReplayArrest"));
            }
            if (ReplayTime > 14.1)
            {
                const auto* Live = M->LivePerformer->FindComponentByClass<UActingRecorderComponent>();
                const auto& Look = Ghost->Wardrobe->GetSelection();
                const bool Preserved = Look.EyeId == TEXT("Eyes.Classic") && Look.MouthId == TEXT("Mouth.Smile") &&
                                       Look.EyebrowId == TEXT("Brows.Neutral") &&
                                       Look.SkinColor.Equals(FLinearColor(.45, .22, .85));
                Finish(ReplayAir && Ghost->Acting->DispatchedEventCount == M->Performances[0].Recording.Events.Num() && Live && Live->IsRecording() &&
                           Live->GetRecordingData().Events.IsEmpty() && Preserved,
                       TEXT("Final model locomotion and five gestures recorded and replayed; jump/face preserved while "
                            "second take records without duplicate events"),
                       M);
            }
            return;
        }
        const double T = M->GetSceneTimeSeconds();
        APawn* Pawn = M->LivePerformer;
        auto* Mesh = Pawn->FindComponentByClass<USkeletalMeshComponent>();
        if(!AttachmentsMatch(Mesh,M->LiveWardrobeComponent,M->LiveWardrobeComponent->Catalog))
        {Finish(false,TEXT("Live cosmetic attachment changed during movement/gesture"),M);return;}
        if(bCaptureAudio && SoundInput<6 && T>.3+SoundInput*2)
        {
            const FKey SoundKeys[]={EKeys::Z,EKeys::X,EKeys::C,EKeys::V,EKeys::B,EKeys::N};Press(SoundKeys[SoundInput++]);
        }
        const FVector Target = Pawn->GetActorLocation() + FVector(0, 0, 5);
        const FVector Location = Target + Pawn->GetActorForwardVector() * 650 + FVector(0, 0, 25);
        Camera->SetActorLocationAndRotation(Location, (Target - Location).Rotation());
        if (const auto* Anim = Cast<UStudioAnimInstance>(Mesh->GetAnimInstance()))
        {
            Idle |= T < .35 && !Anim->bIsMoving && !Anim->bIsInAir;
            Walk |= Anim->bIsMoving && !Anim->bIsRunning && !Anim->bIsInAir;
            RunFast |= Anim->bIsRunning;
            Air |= Anim->bIsInAir;
        }
        if (T > .1 && T < .3)
        {
            Capture(TEXT("Idle"));
        }
        if (T > .4 && !Released && !Shots.Contains(TEXT("Moving")))
        {
            Player->InputKey(
                FInputKeyEventArgs(nullptr, INPUTDEVICEID_NONE, EKeys::W, IE_Pressed, FPlatformTime::Cycles64()));
            Capture(TEXT("Moving"));
        }
        if (T > .85 && !JumpSent)
        {
            Press(EKeys::SpaceBar);
            JumpSent = true;
        }
        if (T > 1.15 && T < 1.6)
        {
            Capture(TEXT("Jump"));
        }
        if (T > 2 && !Released)
        {
            Player->InputKey(
                FInputKeyEventArgs(nullptr, INPUTDEVICEID_NONE, EKeys::W, IE_Released, FPlatformTime::Cycles64()));
            Released = true;
        }
        const FKey Keys[] = {EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five};
        const TCHAR* Names[] = {TEXT("Dance"), TEXT("Point"), TEXT("Scared"), TEXT("Faint"), TEXT("Arrest")};
        if (Gesture < 5 && T > 2.5 + Gesture * 2.3)
        {
            Press(Keys[Gesture]);
            ++Gesture;
        }
        if (Gesture > 0 && T > 2.9 + (Gesture - 1) * 2.3)
        {
            Capture(Names[Gesture - 1]);
        }
        for (UMeshComponent* Part : M->LiveWardrobeComponent->CosmeticMeshes)
        {
            if (Part->ComponentHasTag(TEXT("Customization.Face")) &&
                (Part->GetAttachParent() != Mesh || Part->GetAttachSocketName() != TEXT("Head")))
            {
                Finish(false, TEXT("Face detached from animated head"), M);
                return;
            }
        }
        if (T > 14.1)
        {
            const auto* R = Pawn->FindComponentByClass<UActingRecorderComponent>();
            bool Events = R && R->GetRecordingData().Events.FilterByPredicate([](const auto& Event){return Event.Type==EActingEventType::Gesture;}).Num() == 5;
            if (R)
            {
                for (const auto& E : R->GetRecordingData().Events)
                {
                    Events &= E.Type == EActingEventType::Gesture || E.Type == EActingEventType::Footstep || (bCaptureAudio && E.Type==EActingEventType::Sound);
                }
            }
            if(bCaptureAudio){Events &= SoundInput==6 && R->GetRecordingData().Events.FilterByPredicate([](const auto& Event){return Event.Type==EActingEventType::Sound;}).Num()==6 && M->LiveActingComponent->StartedSoundCount>=6;}
            if (!(Idle && Walk && RunFast && Air && Events))
            {
                Finish(false, TEXT("Live locomotion or five gesture inputs failed"), M);
                return;
            }
            CapturedEvents = 5;
            Phase = 1;
        }
    }
};
TSharedPtr<FMotionRun> Motion;
void Tick(UWorld* World, ELevelTick Type, float Delta)
{
    if (!World || !World->IsGameWorld())
    {
        return;
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("CustomizationMotion")))
    {
        if (!Motion)
        {
            Motion = MakeShared<FMotionRun>();
            Motion->Directory = FPaths::ProjectSavedDir() / TEXT("Automation/CustomizationMotion");
            FParse::Value(FCommandLine::Get(), TEXT("CustomizationReportDir="), Motion->Directory);
        }
        Motion->Tick(World);
        return;
    }
    if (!FParse::Param(FCommandLine::Get(), TEXT("CustomizationSurvey")))
    {
        return;
    }
    if (!Run)
    {
        Run = MakeShared<FRun>();
        Run->Directory = FPaths::ProjectSavedDir() / TEXT("Automation/CustomizationSurvey");
        FParse::Value(FCommandLine::Get(), TEXT("CustomizationReportDir="), Run->Directory);
    }
    Run->Tick(World);
}
struct FRegistration
{
    FDelegateHandle Handle;
    FRegistration()
    {
        Handle = FWorldDelegates::OnWorldTickStart.AddStatic(&Tick);
    }
    ~FRegistration()
    {
        FWorldDelegates::OnWorldTickStart.Remove(Handle);
        Run.Reset();
        Motion.Reset();
    }
} Registration;
} // namespace CustomizationSurvey
#endif
