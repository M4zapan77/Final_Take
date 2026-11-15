#include "StudioLanguageSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Internationalization/Internationalization.h"
#include "Internationalization/Culture.h"
#include "Internationalization/TextLocalizationManager.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

void UStudioLanguageSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);FString Language=TEXT("es");
#if WITH_EDITOR
    if(GIsEditor){OriginalEditorCulture=FInternationalization::Get().GetCurrentCulture()->GetName();bOriginalPreview=FTextLocalizationManager::Get().IsGameLocalizationPreviewEnabled();OriginalPreviewCulture=FTextLocalizationManager::Get().GetConfiguredGameLocalizationPreviewLanguage();}
#endif
    if(GConfig){GConfig->GetString(TEXT("FinalTake.Preferences"),TEXT("Language"),Language,GGameUserSettingsIni);}
    FParse::Value(FCommandLine::Get(),TEXT("language="),Language);
    if(!SetLanguage(Language,false)){SetLanguage(TEXT("es"),false);}
}
void UStudioLanguageSubsystem::Deinitialize()
{
#if WITH_EDITOR
    if(GIsEditor && !OriginalEditorCulture.IsEmpty())
    {FInternationalization::Get().SetCurrentCulture(OriginalEditorCulture);if(bOriginalPreview){FTextLocalizationManager::Get().EnableGameLocalizationPreview(OriginalPreviewCulture);}else{FTextLocalizationManager::Get().DisableGameLocalizationPreview();}FTextLocalizationManager::Get().WaitForAsyncTasks();}
#endif
    Super::Deinitialize();
}
bool UStudioLanguageSubsystem::SetLanguage(const FString& Language,bool Save)
{
    const FString Culture=Language.ToLower();if(Culture!=TEXT("es") && Culture!=TEXT("en")){return false;}
    const bool Changed=GetLanguage()!=Culture;if(!FInternationalization::Get().SetCurrentCulture(Culture)){return false;}
#if WITH_EDITOR
    if(GIsEditor){FTextLocalizationManager::Get().EnableGameLocalizationPreview(Culture);}
#endif
    FTextLocalizationManager::Get().WaitForAsyncTasks();
    if(Save && GConfig){GConfig->SetString(TEXT("FinalTake.Preferences"),TEXT("Language"),*Culture,GGameUserSettingsIni);GConfig->Flush(false,GGameUserSettingsIni);}
    if(Changed){OnLanguageChanged.Broadcast(Culture);}return true;
}
FString UStudioLanguageSubsystem::GetLanguage() const{return FInternationalization::Get().GetCurrentLanguage()->GetTwoLetterISOLanguageName();}
UStudioLanguageSubsystem* UStudioLanguageSubsystem::Find(const UObject* Context)
{const auto* World=Context?Context->GetWorld():nullptr;auto* Instance=World?World->GetGameInstance():nullptr;return Instance?Instance->GetSubsystem<UStudioLanguageSubsystem>():nullptr;}
FText UStudioLanguageSubsystem::ActingEventName(FName Id)
{
    static const TMap<FName,FText> Names={
        {TEXT("Dance"),NSLOCTEXT("ActingNames","Dance","Dance")},{TEXT("Point"),NSLOCTEXT("ActingNames","Point","Point")},
        {TEXT("Scared"),NSLOCTEXT("ActingNames","Scared","Scared")},{TEXT("Faint"),NSLOCTEXT("ActingNames","Faint","Faint")},
        {TEXT("Arrest"),NSLOCTEXT("ActingNames","Arrest","Arrest")},{TEXT("EvilLaugh"),NSLOCTEXT("ActingNames","EvilLaugh","Evil laugh")},
        {TEXT("Scream"),NSLOCTEXT("ActingNames","Scream","Scream")},{TEXT("Cry"),NSLOCTEXT("ActingNames","Cry","Cry")},
        {TEXT("PoliceSiren"),NSLOCTEXT("ActingNames","PoliceSiren","Police siren")},{TEXT("Kiss"),NSLOCTEXT("ActingNames","Kiss","Kiss")},
        {TEXT("Failure"),NSLOCTEXT("ActingNames","Failure","Failure")}};
    const auto* Text=Names.Find(Id);return Text?*Text:FText::FromName(Id);
}
