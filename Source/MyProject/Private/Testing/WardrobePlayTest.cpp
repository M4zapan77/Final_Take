// Optional integration test in a real game world/LocalPlayer. Never active during ordinary Play.
#if !UE_BUILD_SHIPPING

#include "MovieManager.h"
#include "WardrobeComponent.h"
#include "WardrobeWidget.h"
#include "ActingComponent.h"
#include "ActingRecorderComponent.h"
#include "ReplayActor.h"
#include "Camera/CameraActor.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformTime.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "TimerManager.h"
#include "UnrealClient.h"

namespace WardrobePlayTest
{
    class FRun
    {
    public:
        TWeakObjectPtr<UWorld> World;
        TWeakObjectPtr<AMovieManager> Manager;
        FTimerHandle Timer;
        double StartedAt = FPlatformTime::Seconds();
        double WardrobeStart = 0.0;
        int32 CurrentPreparation = 0;
        int32 WardrobeScreens = 0;
        int32 ReadyStarts = 0;
        int32 TimeoutStarts = 0;
        bool Acted[3] = {};
        bool Captured[4] = {};
        bool bFinished = false;
        bool bPremiereChecked = false;
        bool bScreenshots = false;
        TArray<FCosmeticSelection> Expected;

        bool Check(bool bCondition, const TCHAR* Message)
        {
            if (!bCondition) { Finish(false, Message); }
            return bCondition;
        }

        void Screenshot(const FString& Name)
        {
            if (!bScreenshots) { return; }
            const FString Directory = FPaths::ProjectSavedDir() / TEXT("Automation/WardrobePlay");
            IFileManager::Get().MakeDirectory(*Directory, true);
            FScreenshotRequest::RequestScreenshot(Directory / (Name + TEXT(".png")), true, false);
        }

        bool PressItem(UWardrobeWidget* Widget, ECosmeticCategory Category, FName Id)
        {
            for (UWardrobeChoiceButton* Button : Widget->ChoiceButtons)
            {
                if (!Button->bColorChoice && Button->Category == Category && Button->CosmeticId == Id)
                {
                    Button->OnClicked.Broadcast();
                    return true;
                }
            }
            return false;
        }

        void Finish(bool bPassed, const TCHAR* Message)
        {
            if (bFinished) { return; }
            bFinished = true;
            if (World.IsValid()) { World->GetTimerManager().ClearTimer(Timer); }
            const FString Directory = FPaths::ProjectSavedDir() / TEXT("Automation/WardrobePlay");
            IFileManager::Get().MakeDirectory(*Directory, true);
            const FString Result = FString::Printf(TEXT("{\n  \"passed\": %s,\n  \"wardrobeScreens\": %d,\n  \"readyStarts\": %d,\n  \"timeoutStarts\": %d,\n  \"message\": \"%s\"\n}\n"),
                bPassed ? TEXT("true") : TEXT("false"), WardrobeScreens, ReadyStarts, TimeoutStarts, Message);
            FFileHelper::SaveStringToFile(Result, *(Directory / TEXT("result.json")));
            if (Manager.IsValid()) { Manager->StopMovie(); }
            UE_LOG(LogTemp, Display, TEXT("WardrobePlay: %s - %s"), bPassed ? TEXT("PASS") : TEXT("FAIL"), Message);
            // The explicit -WardrobeSmoke -game gate below makes process exit safe for this test runner.
            FPlatformMisc::RequestExitWithStatus(false, bPassed ? 0 : 1);
        }

        void Tick()
        {
            if (bFinished || !World.IsValid()) { return; }
            if (FPlatformTime::Seconds() - StartedAt > 90.0) { Finish(false, TEXT("Timed out waiting for game/wardrobe flow")); return; }
            APlayerController* Player = UGameplayStatics::GetPlayerController(World.Get(), 0);
            APawn* Pawn = Player ? Player->GetPawn() : nullptr;
            if (!Player || !Pawn) { return; }
            if (!Manager.IsValid())
            {
                for (TActorIterator<AMovieManager> It(World.Get()); It; ++It) { It->StopMovie(); }
                AMovieManager* Movie = World->SpawnActor<AMovieManager>();
                Manager = Movie;
                Movie->PreparationDurationSeconds = 3.0;
                Movie->RecordingDurationSeconds = 2.0;
                Movie->bShowDebugOverlay = false;
                Movie->bBindActingKeys = Movie->bBindRestartKey = false;
                bScreenshots = FParse::Param(FCommandLine::Get(), TEXT("WardrobeSmokeScreenshots"));
                for (const FName Head : {FName(TEXT("Head.Crown")), FName(TEXT("Head.PoliceCap")), FName(TEXT("Head.Chicken"))})
                {
                    FCosmeticSelection Outfit;
                    Outfit.HeadId = Head;
                    Outfit.BodyId = TEXT("Body.Casual");
                    Outfit.PropId = TEXT("Prop.Banana");
                    Expected.Add(Outfit);
                }
                Expected[0].BodyId = TEXT("Body.Dress");
                Expected[1].BodyId = TEXT("Body.Uniform");
                Expected[1].PropId = TEXT("Prop.Donut");
                Expected[2].BodyId = TEXT("Body.Suit");
                Expected[2].PropId = TEXT("Prop.Microphone");
                if (!Check(Movie->StartMovie(), TEXT("Real game could not start a movie"))) { return; }
            }
            AMovieManager* Movie = Manager.Get();
            if (!Check(Movie->State != EMovieState::Error, TEXT("Movie entered Error"))) { return; }
            if (Movie->IsWardrobeOpen())
            {
                UWardrobeWidget* Widget = Movie->ActiveWardrobeWidget;
                const int32 Take = Movie->CurrentTakeIndex;
                if (!Check(Widget && Widget->IsInViewport(), TEXT("Wardrobe was not added to the real LocalPlayer viewport"))) { return; }
                if (!Check(Player->bShowMouseCursor && Player->IsMoveInputIgnored() && Player->IsLookInputIgnored(), TEXT("Wardrobe input/cursor ownership failed"))) { return; }
                if (!Check(!Movie->LiveActingComponent->CanTriggerActions() && Movie->GetSceneTimeSeconds() == 0.0, TEXT("Wardrobe advanced or recorded the scene timeline"))) { return; }
                if (!Check(Player->GetViewTarget() != Pawn, TEXT("Wardrobe preview camera was not selected"))) { return; }
                if (CurrentPreparation != Movie->PreparationId)
                {
                    CurrentPreparation = Movie->PreparationId;
                    WardrobeStart = World->GetTimeSeconds();
                    ++WardrobeScreens;
                    if (!Check(PressItem(Widget, ECosmeticCategory::Head, Expected[Take].HeadId)
                        && PressItem(Widget, ECosmeticCategory::Body, Expected[Take].BodyId)
                        && PressItem(Widget, ECosmeticCategory::Prop, Expected[Take].PropId), TEXT("A native UMG choice button was missing"))) { return; }
                    if (!Check(Movie->LiveWardrobeComponent->GetSelection().HeadId == Expected[Take].HeadId, TEXT("Preview did not receive UMG selection"))) { return; }
                    UE_LOG(LogTemp, Display, TEXT("WardrobePlay: real wardrobe %d visible with role %s"), Take + 1, *Movie->GetCurrentRoleName().ToString());
                }
                const double Age = World->GetTimeSeconds() - WardrobeStart;
                if (Age > .2 && !Check(Widget->ViewData.RemainingSeconds < 2.95, TEXT("Visible UMG countdown did not update"))) { return; }
                if (Age > .15 && !Captured[Take]) { Screenshot(FString::Printf(TEXT("Take%d"), Take + 1)); Captured[Take] = true; }
                // Take 2 deliberately uses timeout; the other two use the actual native Ready button.
                if (Take != 1 && Age > .5 && (!bScreenshots || !FScreenshotRequest::IsScreenshotRequested()))
                {
                    if (!Check(Widget->ReadyButton != nullptr, TEXT("Ready button was missing"))) { return; }
                    Widget->ReadyButton->OnClicked.Broadcast();
                    ++ReadyStarts;
                    if (!Check(Movie->State == EMovieState::Recording && !Movie->ReadyForTake(CurrentPreparation), TEXT("Ready did not start exactly once"))) { return; }
                }
            }
            else if (Movie->State == EMovieState::Recording)
            {
                // Recorder finishes in PostPhysics; manager advances in PostUpdateWork, after replays.
                // The timer can observe this intentional one-frame handoff, when input is already frozen.
                const UActingRecorderComponent* LiveRecorder = Pawn->FindComponentByClass<UActingRecorderComponent>();
                if (!LiveRecorder || !LiveRecorder->IsRecording()) { return; }
                const int32 Take = Movie->CurrentTakeIndex;
                if (!Check(!Movie->ActiveWardrobeWidget && !Player->IsMoveInputIgnored() && !Player->IsLookInputIgnored(), TEXT("Gameplay input/UI was not restored"))) { return; }
                if (!Check(Player->GetViewTarget() == Pawn, TEXT("Gameplay camera was not restored"))) { return; }
                Pawn->AddMovementInput(FVector(1.0, Take == 1 ? 1.0 : 0.0, 0.0).GetSafeNormal(), .5f);
                if (!Acted[Take])
                {
                    if (Take == 1) { ++TimeoutStarts; }
                    Movie->LiveActingComponent->TriggerGesture(static_cast<EActingGesture>(Take));
                    Movie->LiveActingComponent->TriggerSound(static_cast<EActingSound>(Take));
                    Acted[Take] = true;
                }
                for (int32 Prior = 0; Prior < Movie->ActiveReplays.Num(); ++Prior)
                {
                    if (!Check(Movie->ActiveReplays[Prior]->Wardrobe->GetSelection().HeadId == Expected[Prior].HeadId, TEXT("A previous replay outfit changed"))) { return; }
                }
            }
            else if (Movie->State == EMovieState::MoviePremiere)
            {
                bPremiereChecked = true;
                if (!Check(Movie->ActiveReplays.Num() == 3 && Player->GetViewTarget() == Movie->GetPremiereCamera(), TEXT("Premiere camera or actor count failed"))) { return; }
                for (int32 Index = 0; Index < 3; ++Index)
                {
                    if (!Check(Movie->ActiveReplays[Index]->Wardrobe->GetSelection().BodyId == Expected[Index].BodyId, TEXT("Premiere outfit was not restored per actor"))) { return; }
                }
                if (Movie->GetSceneTimeSeconds() > .15 && !Captured[3]) { Screenshot(TEXT("Premiere")); Captured[3] = true; }
            }
            else if (Movie->State == EMovieState::Finished)
            {
                if (!Check(WardrobeScreens == 3 && ReadyStarts == 2 && TimeoutStarts == 1 && bPremiereChecked, TEXT("Did not exercise all three wardrobes, Ready, timeout, and premiere"))) { return; }
                for (int32 Index = 0; Index < Movie->Performances.Num(); ++Index)
                {
                    const FActingRecording& Data = Movie->Performances[Index].Recording;
                    if (!Check(Data.IsValid() && Data.Appearance.Selection.PropId == Expected[Index].PropId
                        && Data.Events.Num() == 2 && Movie->ActiveReplays[Index]->Acting->DispatchedEventCount == 2, TEXT("A completed performance lost its appearance or acting events"))) { return; }
                }
                const int32 OldPreparation = Movie->PreparationId;
                Movie->RestartMovie();
                UActingRecorderComponent* Recorder = Pawn->FindComponentByClass<UActingRecorderComponent>();
                if (!Check(Movie->IsWardrobeOpen() && Movie->ActiveWardrobeWidget && Movie->Performances.IsEmpty()
                    && Movie->ActiveReplays.IsEmpty() && !Recorder->GetRecordingData().Appearance.bHasAppearance
                    && !Movie->ReadyForTake(OldPreparation), TEXT("Restart failed to clear data or reject stale Ready"))) { return; }
                Finish(true, TEXT("Three real UMG wardrobes, early Ready, timeout, independent replay outfits, premiere, and restart passed"));
            }
        }
    };

    void Start(UWorld* World)
    {
        if (!FParse::Param(FCommandLine::Get(), TEXT("WardrobeSmoke")) || !FParse::Param(FCommandLine::Get(), TEXT("game"))
            || !World || !World->IsGameWorld())
        {
            UE_LOG(LogTemp, Display, TEXT("Movie.WardrobeSmoke requires a separate -game -WardrobeSmoke process. Use Scripts/Test-WardrobePlay.ps1."));
            return;
        }
        TSharedPtr<FRun> Run = MakeShared<FRun>();
        Run->World = World;
        World->GetTimerManager().SetTimer(Run->Timer, FTimerDelegate::CreateLambda([Run]() { Run->Tick(); }), .05f, true);
    }

    FAutoConsoleCommandWithWorld Command(TEXT("Movie.WardrobeSmoke"), TEXT("Integration test in a separate -game -WardrobeSmoke process only; writes report and exits."),
        FConsoleCommandWithWorldDelegate::CreateStatic(&Start));
}
#endif
