#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "StudioLanguageSubsystem.h"
#include "StorySessionComponent.h"
#include "MovieResultsTypes.h"
#include "WardrobeTypes.h"
#include "Engine/GameInstance.h"
#include "Internationalization/Internationalization.h"
#include "Internationalization/Culture.h"
#include "Internationalization/TextLocalizationManager.h"
#include "Misc/AutomationTest.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStudioLanguageResourcesTest,"OneManStudio.Localization.LiveFTextAndValidation",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FStudioLanguageResourcesTest::RunTest(const FString& Parameters)
{
    const FString Original=FInternationalization::Get().GetCurrentCulture()->GetName();const bool Preview=FTextLocalizationManager::Get().IsGameLocalizationPreviewEnabled();const FString PreviewCulture=FTextLocalizationManager::Get().GetConfiguredGameLocalizationPreviewLanguage();
    auto* Instance=NewObject<UGameInstance>();auto* Language=NewObject<UStudioLanguageSubsystem>(Instance);
    const FText Role=UStorySessionComponent::RoleName(0),Hat=WardrobeDefaults::LocalizedName(TEXT("Head.PoliceCap"),FText::GetEmpty());const FMovieResultsSettings Settings;
    TestTrue(TEXT("Select English without saving"),Language->SetLanguage(TEXT("en"),false));
    TestEqual(TEXT("Existing role FText translates to English"),Role.ToString(),FString(TEXT("Robber")));
    TestEqual(TEXT("Stable cosmetic ID translates"),Hat.ToString(),FString(TEXT("Police cap")));
    TestEqual(TEXT("Rating text in English"),Settings.RatingTexts[0].Name.ToString(),FString(TEXT("Cinematic Disaster")));
    TestTrue(TEXT("Select Spanish without a movie restart"),Language->SetLanguage(TEXT("es"),false));
    TestEqual(TEXT("Same role object refreshes"),Role.ToString(),FString(TEXT("Ladrón")));
    TestEqual(TEXT("Same cosmetic object refreshes"),Hat.ToString(),FString(TEXT("Gorra de policía")));
    TestEqual(TEXT("Same rating object refreshes"),Settings.RatingTexts[0].Name.ToString(),FString(TEXT("Desastre cinematográfico")));
    TestFalse(TEXT("Unsupported language rejected"),Language->SetLanguage(TEXT("invalid"),false));
    TestEqual(TEXT("Invalid request preserves selected language"),Language->GetLanguage(),FString(TEXT("es")));
    FInternationalization::Get().SetCurrentCulture(Original);if(Preview){FTextLocalizationManager::Get().EnableGameLocalizationPreview(PreviewCulture);}else{FTextLocalizationManager::Get().DisableGameLocalizationPreview();}FTextLocalizationManager::Get().WaitForAsyncTasks();return true;
}
#endif
