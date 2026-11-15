#if !UE_BUILD_SHIPPING
#include "ActingComponent.h"
#include "ActingSceneClockSubsystem.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformTime.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "MovieManager.h"
#include "ObjectiveHUDWidget.h"
#include "ReplayActor.h"
#include "StudioCinemaPresenter.h"
#include "UnifiedWardrobeWidget.h"
#include "UnrealClient.h"
#include "WardrobeComponent.h"
#include "WardrobePreviewActor.h"
#include "Components/MeshComponent.h"

namespace ProductionArtSurvey
{
struct FRun
{
    double Started = FPlatformTime::Seconds(), At = 0, ResultsAt = -1;
    FString Directory;
    int32 Step = 0, Shots = 0, LastTake = -1, Frames = 0;
    bool Setup = false, Done = false, BankShot = false, CinemaShot = false;
    TArray<FCosmeticDefinition> Items;
    TWeakObjectPtr<ACameraActor> Overview;
    bool Acted[3] = {};
    void Finish(bool Pass, const TCHAR* Message, AMovieManager* M)
    {
        if (Done)
        {
            return;
        }
        Done = true;
        IFileManager::Get().MakeDirectory(*Directory, true);
        FFileHelper::SaveStringToFile(
            FString::Printf(
                TEXT(
                    "{\"passed\":%s,\"items\":%d,\"screenshots\":%d,\"cinemaFrames\":%d,\"message\":\"%s\"}"),
                Pass ? TEXT("true") : TEXT("false"), Items.Num(), Shots, Frames, Message),
            *(Directory / TEXT("result.json")));
        if (M)
        {
            M->StopMovie();
        }
        UE_LOG(LogTemp, Display, TEXT("ProductionArtSurvey %s: %s"), Pass ? TEXT("PASS") : TEXT("FAIL"),
               Message);
        FPlatformMisc::RequestExitWithStatus(false, Pass ? 0 : 1);
    }
    void Shot(const FString& Name)
    {
        IFileManager::Get().MakeDirectory(*Directory, true);
        FScreenshotRequest::RequestScreenshot(Directory / (Name + TEXT(".png")), true, false);
        ++Shots;
    }
    void Tick(UWorld* World)
    {
        if (Done)
        {
            return;
        }
        AMovieManager* M = nullptr;
        for (TActorIterator<AMovieManager> I(World); I; ++I)
        {
            M = *I;
            break;
        }
        if (FPlatformTime::Seconds() - Started > 240)
        {
            Finish(false, TEXT("Production survey timed out"), M);
            return;
        }
        auto* Player = UGameplayStatics::GetPlayerController(World, 0);
        if (!M || !Player || !M->LivePerformer)
        {
            return;
        }
        if (!Setup)
        {
            M->StopMovie();
            M->PreparationDurationSeconds = 180;
            M->RecordingDurationSeconds = 3;
            if (!M->StartMovie())
            {
                Finish(false, TEXT("Could not start art survey"), M);
                return;
            }
            for (const auto& I : M->LiveWardrobeComponent->Catalog)
            {
                FString ViewItem;
                const bool ItemViews=FParse::Value(FCommandLine::Get(),TEXT("CorrectionItemViews="),ViewItem);
                if(ItemViews && I.Id!=FName(*ViewItem)){continue;}
                if (FParse::Param(FCommandLine::Get(), TEXT("CorrectionMaskViews")) && I.Id != TEXT("Head.RobberMask"))
                {
                    continue;
                }
                if (FParse::Param(FCommandLine::Get(), TEXT("ProductionFaceSurvey")) &&
                    I.Category != ECosmeticCategory::Face && I.Id != TEXT("Head.RobberMask") &&
                    I.Id != TEXT("Head.PoliceCap"))
                {
                    continue;
                }
                if (I.Category == ECosmeticCategory::Body || I.Category == ECosmeticCategory::Head ||
                    I.Category == ECosmeticCategory::Face || I.Category == ECosmeticCategory::Prop)
                {
                    Items.Add(I);
                }
            }
            Setup = true;
            const bool FaceOnly=FParse::Param(FCommandLine::Get(),TEXT("ProductionFaceSurvey"));
            FString ViewItem;const bool ItemViews=FParse::Value(FCommandLine::Get(),TEXT("CorrectionItemViews="),ViewItem);
            if(M->LiveWardrobeComponent->Catalog.Num()!=44 || Items.Num()!=(FParse::Param(FCommandLine::Get(),TEXT("CorrectionMaskViews"))||ItemViews?1:FaceOnly?6:23))
            {
                Finish(false,TEXT("Production catalog or cooked cosmetic assets are missing"),M);return;
            }
            At = FPlatformTime::Seconds();
            return;
        }
        if (FScreenshotRequest::IsScreenshotRequested() || FPlatformTime::Seconds() - At < .65)
        {
            return;
        }
        if (M->IsWardrobeOpen())
        {
            auto* UI = Cast<UUnifiedWardrobeWidget>(M->ActiveWardrobeWidget);
            if (!UI || !UI->PreviewActor)
            {
                return;
            }
            FString ViewItem;const bool ItemViews=FParse::Value(FCommandLine::Get(),TEXT("CorrectionItemViews="),ViewItem);
            const int32 StepsPerItem=FParse::Param(FCommandLine::Get(),TEXT("CorrectionMaskViews"))||ItemViews?10:2;
            if (M->CurrentTakeIndex == 0 && Step < Items.Num() * StepsPerItem)
            {
                const auto& I = Items[Step / StepsPerItem];
                if (Step % StepsPerItem == 0)
                {
                    UI->ResetAll();
                    UI->ChooseItem(ECosmeticCategory::Eyes, TEXT("Eyes.Classic"));
                    UI->ChooseItem(ECosmeticCategory::Mouth, TEXT("Mouth.Smile"));
                    UI->ChooseItem(ECosmeticCategory::Eyebrows, TEXT("Brows.Neutral"));
                    UI->SelectTab(I.Category == ECosmeticCategory::Body   ? ECustomizationTab::Clothing
                                  : I.Category == ECosmeticCategory::Head ? ECustomizationTab::Hats
                                  : I.Category == ECosmeticCategory::Face ? ECustomizationTab::Accessories
                                                                          : ECustomizationTab::Objects);
                    UI->ChooseItem(I.Category, I.Id);
                    if(I.Category==ECosmeticCategory::Body && FParse::Param(FCommandLine::Get(),TEXT("CorrectionSkinSurvey"))){UI->ChooseSkin(FLinearColor(.45,.22,.85));}
                    if (I.Category == ECosmeticCategory::Prop)
                    {
                        UI->ChooseItem(ECosmeticCategory::LeftProp, I.Id);
                    }
                    if (M->LiveWardrobeComponent->GetSelection().GetId(I.Category) != I.Id ||
                        !M->LiveWardrobeComponent->CaptureAppearance().IsValid())
                    {
                        Finish(false, TEXT("Invalid production cosmetic selection"), M);
                        return;
                    }
                }
                else if (StepsPerItem == 10 && Step % 2 == 0)
                {
                    UI->PreviewActor->RotateView(Step == 6 ? -180.f : 45.f);
                }
                else
                {
                    for (const UMeshComponent* Part : UI->PreviewActor->Wardrobe->CosmeticMeshes)
                    {
                        if (Part->ComponentHasTag(I.Id))
                        {
                            UE_LOG(LogTemp, Display, TEXT("ArtFit %s: actor-relative %s bounds %s extent %s"),
                                   *I.Id.ToString(),
                                   *(Part->GetComponentTransform()
                                         .GetRelativeTransform(UI->PreviewActor->GetActorTransform())
                                         .ToString()),
                                   *(Part->Bounds.Origin - UI->PreviewActor->GetActorLocation()).ToString(),
                                   *Part->Bounds.BoxExtent.ToString());
                        }
                    }
                    Shot(I.Id.ToString().Replace(TEXT("."), TEXT("_"))+(StepsPerItem==10?FString::Printf(TEXT("_View%d"),Step/2):TEXT("")));
                }
                ++Step;
                At = FPlatformTime::Seconds();
                return;
            }
            FCosmeticSelection S;
            S.BodyId = M->CurrentTakeIndex == 0   ? TEXT("Body.Robber")
                       : M->CurrentTakeIndex == 1 ? TEXT("Body.Uniform")
                                                  : TEXT("Body.Dress");
            S.HeadId = M->CurrentTakeIndex == 0   ? TEXT("Head.Beanie")
                       : M->CurrentTakeIndex == 1 ? TEXT("Head.PoliceCap")
                                                  : TEXT("Head.Crown");
            S.EyeId = TEXT("Eyes.Classic");
            S.MouthId = TEXT("Mouth.Smile");
            S.EyebrowId = TEXT("Brows.Neutral");
            M->LiveWardrobeComponent->SetSelection(S);
            LastTake = M->CurrentTakeIndex;
            if (!UI->Ready())
            {
                Finish(false, TEXT("Ready failed"), M);
                return;
            }
            At = FPlatformTime::Seconds();
            return;
        }
        if (M->State == EMovieState::Recording)
        {
            const int32 Index = M->CurrentTakeIndex;
            if (!Acted[Index] && M->GetSceneTimeSeconds() > .1)
            {
                if (!M->LiveActingComponent->TriggerGesture(EActingGesture::Dance) ||
                    !M->LiveActingComponent->TriggerSound(EActingSound::EvilLaugh))
                {
                    Finish(false, TEXT("Live events failed during art test"), M);
                    return;
                }
                Acted[Index] = true;
            }
            // A small visible path in the open acting area, recorded by the existing sampler.
            M->LivePerformer->SetActorLocation(
                FVector(-450 + Index * 300 + M->GetSceneTimeSeconds() * 70, (Index - 1) * 120, 96), false,
                nullptr, ETeleportType::TeleportPhysics);
            if (!Overview.IsValid())
            {
                Overview = World->SpawnActor<ACameraActor>();
                Overview->SetActorLocation(FVector(-150, -100, 1850));
                Overview->SetActorRotation((FVector(30, 0, 0) - Overview->GetActorLocation()).Rotation());
                Overview->GetCameraComponent()->SetFieldOfView(72);
            }
            if (!BankShot && M->GetSceneTimeSeconds() > .9)
            {
                Player->SetViewTarget(Overview.Get());
                if (M->ActiveObjectiveHUD)
                {
                    M->ActiveObjectiveHUD->SetVisibility(ESlateVisibility::Collapsed);
                }
                BankShot = true;
                At = FPlatformTime::Seconds();
                return;
            }
            if (BankShot && Shots == Items.Num())
            {
                Shot(TEXT("BankOverview"));
                At = FPlatformTime::Seconds();
            }
            return;
        }
        if (M->State == EMovieState::MoviePremiere &&
            M->PresentationPhase == EMoviePresentationPhase::Playback && M->GetSceneTimeSeconds() > .7)
        {
            AStudioCinemaPresenter* Presenter = nullptr;
            for (TActorIterator<AStudioCinemaPresenter> I(World); I; ++I)
            {
                Presenter = *I;
                break;
            }
            if (!Presenter || !Presenter->IsPresenting() ||
                Presenter->GetFilmCamera() != M->GetPremiereCamera() ||
                Player->GetViewTarget() != Presenter->GetAudienceCamera() || Presenter->CapturedFrames < 1 ||
                M->ActiveReplays.Num() != 3)
            {
                Finish(false, TEXT("Cinema did not present all three existing performances"), M);
                return;
            }
            Frames = Presenter->CapturedFrames;
            for (int32 Index = 0; Index < 3; ++Index)
            {
                const AReplayActor* Replay = M->ActiveReplays[Index];
                if (Replay->GetActorEnableCollision() ||
                    Replay->Acting->DispatchedEventCount != M->Performances[Index].Recording.Events.FilterByPredicate([&](const auto& Event){return Event.TimeSeconds<=Replay->GetEvaluatedTakeTimeSeconds();}).Num() ||
                    Replay->Wardrobe->GetSelection().BodyId !=
                        M->Performances[Index].Recording.Appearance.Selection.BodyId)
                {
                    Finish(false, TEXT("Premiere lost events or independent outfits"), M);
                    return;
                }
            }
            if (!CinemaShot)
            {
                UKismetRenderingLibrary::ExportRenderTarget(World, Presenter->ScreenTexture, Directory,
                                                            TEXT("CapturedFilm.png"));
                Shot(TEXT("CinemaMovie"));
                CinemaShot = true;
                At = FPlatformTime::Seconds();
            }
            return;
        }
        if (M->IsResultsOpen())
        {
            if (ResultsAt < 0)
            {
                ResultsAt = World->GetTimeSeconds();
            }
            if (World->GetTimeSeconds() - ResultsAt < .8)
            {
                return;
            }
            if (!CinemaShot)
            {
                Finish(false, TEXT("No rendered cinema playback observed"), M);
                return;
            }
            if (Shots == Items.Num() + 2)
            {
                Shot(TEXT("Results"));
                At = FPlatformTime::Seconds();
                return;
            }
            const bool Restart = M->RecordNewMovie() && M->IsWardrobeOpen() && M->Performances.IsEmpty() &&
                                 M->ActiveReplays.IsEmpty();
            bool Clean = Restart;
            for (TActorIterator<AStudioCinemaPresenter> I(World); I; ++I)
            {
                Clean &= !I->IsPresenting() && !I->FilmCapture->bCaptureEveryFrame;
            }
            Finish(
                Clean,
                TEXT("Production cosmetics, bank, three take cinema, event isolation and restart verified"),
                M);
        }
    }
};
TSharedPtr<FRun> Run;
void Tick(UWorld* World, ELevelTick Type, float Delta)
{
    if (!World || !World->IsGameWorld() || !FParse::Param(FCommandLine::Get(), TEXT("ProductionArtSurvey")))
    {
        return;
    }
    if (!Run)
    {
        Run = MakeShared<FRun>();
        Run->Directory = FPaths::ProjectSavedDir() / TEXT("Automation/ProductionArtSurvey");
        FParse::Value(FCommandLine::Get(), TEXT("ProductionArtReportDir="), Run->Directory);
    }
    Run->Tick(World);
}
struct FRegistration
{
    FDelegateHandle Handle = FWorldDelegates::OnWorldTickStart.AddStatic(&Tick);
    ~FRegistration()
    {
        FWorldDelegates::OnWorldTickStart.Remove(Handle);
        Run.Reset();
    }
} Registration;
} // namespace ProductionArtSurvey
#endif
