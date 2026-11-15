#include "StudioUITheme.h"
#include "Engine/FontFace.h"
#include "Engine/Texture2D.h"
#include "Styling/CoreStyle.h"

UStudioUITheme* UStudioUITheme::LoadDefault()
{
    return LoadObject<UStudioUITheme>(nullptr,TEXT("/Game/OneManStudio/UI/Common/DA_FinalTakeUITheme"),nullptr,LOAD_NoWarn|LOAD_Quiet);
}
UTexture2D* UStudioUITheme::Texture(FName Id) const
{
    const auto* Found=Art.Find(Id);return Found?Found->Get():nullptr;
}
FSlateFontInfo UStudioUITheme::Heading(int32 Size) const
{
    if(!HeadingFace){return FCoreStyle::GetDefaultFontStyle(TEXT("Bold"),Size);}
    if(!HeadingFamily)
    {
        auto Family=MakeShared<FCompositeFont>();
        auto& Entry=Family->DefaultTypeface.Fonts.AddDefaulted_GetRef();Entry.Name=TEXT("Regular");Entry.Font=FFontData(HeadingFace);
        HeadingFamily=Family;
    }
    return FSlateFontInfo(HeadingFamily,Size,TEXT("Regular"));
}
FSlateBrush UStudioUITheme::Brush(FName Id,FMargin Margin) const
{
    FSlateBrush Result;
    if(auto* Image=Texture(Id))
    {
        Result.SetResourceObject(Image);Result.ImageSize=FVector2D(Image->GetSizeX(),Image->GetSizeY());
        Result.DrawAs=Margin==FMargin(0)?ESlateBrushDrawType::Image:ESlateBrushDrawType::Box;
        // Slate box margins are fractions of ImageSize in logical UI units.
        // Use a display-sized brush so original 4K pixels do not become giant borders.
        if(Margin!=FMargin(0)){Result.ImageSize*=FMath::Min(1.0,256.0/FMath::Max(Image->GetSizeX(),Image->GetSizeY()));}
        Result.Margin=Margin;Result.TintColor=FLinearColor::White;
    }
    else{Result.DrawAs=ESlateBrushDrawType::RoundedBox;Result.TintColor=Navy;Result.OutlineSettings.CornerRadii=FVector4(12,12,12,12);Result.OutlineSettings.RoundingType=ESlateBrushRoundingType::FixedRadius;}
    return Result;
}
void UStudioUITheme::StyleButton(UButton* Button,FName Kind,bool bSelected) const
{
    if(!Button){return;}
    FString Normal=TEXT("Button.Normal"),Hover=TEXT("Button.Hover"),Pressed=TEXT("Button.Pressed");
    bool Light=false;FMargin Slice(.16,.34,.16,.34),Padding(16,20);
    if(Kind==TEXT("Primary")){Normal=TEXT("Button.Primary");Light=true;}
    else if(Kind==TEXT("Ready") || Kind==TEXT("Random") || Kind==TEXT("Reset"))
    {Normal=TEXT("Wardrobe.")+Kind.ToString();Light=Kind==TEXT("Ready");}
    else if(Kind==TEXT("Card") || Kind==TEXT("Tab"))
    {
        const FString Prefix=TEXT("Wardrobe.")+Kind.ToString()+TEXT(".");
        Normal=Prefix+(bSelected?TEXT("Selected"):TEXT("Normal"));Hover=Pressed=Prefix+TEXT("Selected");
        // These cards are square and tabs have their authored aspect ratio.
        // Draw their complete artwork; slicing through the painted inner frame
        // would stretch the cream center into the border at small UI sizes.
        Light=true;Slice=FMargin(0);Padding=Kind==TEXT("Card")?FMargin(20):FMargin(8);
    }
    FButtonStyle Style=Button->GetStyle();
    Style.SetNormal(Brush(FName(*Normal),Slice));Style.SetHovered(Brush(FName(*Hover),Slice));Style.SetPressed(Brush(FName(*Pressed),Slice));
    Style.SetNormalForeground(Light?Navy:Cream);Style.SetHoveredForeground(Navy);Style.SetPressedForeground(Navy);
    Style.NormalPadding=Padding;Style.PressedPadding=Padding;
    Button->SetStyle(Style);Button->SetBackgroundColor(FLinearColor::White);Button->SetColorAndOpacity(FLinearColor::White);
}
UStudioArtButton::UStudioArtButton()
{
    SetRenderTransformPivot(FVector2D(.5));
    OnHovered.AddDynamic(this,&UStudioArtButton::Hover);OnUnhovered.AddDynamic(this,&UStudioArtButton::Leave);
    OnPressed.AddDynamic(this,&UStudioArtButton::Press);OnReleased.AddDynamic(this,&UStudioArtButton::Release);
}
void UStudioArtButton::ScaleVisual(float Scale)
{
    // Keep the button's hit rectangle fixed, including clicks in the same frame
    // as a page reflow. Only its visual content receives the tiny scale feedback.
    if(UWidget* Visual=GetContent()){Visual->SetRenderTransformPivot(FVector2D(.5));Visual->SetRenderScale(FVector2D(Scale));}
}
void UStudioArtButton::Hover(){ScaleVisual(1.018);}
void UStudioArtButton::Leave(){ScaleVisual(1);}
void UStudioArtButton::Press(){ScaleVisual(.985);}
void UStudioArtButton::Release(){ScaleVisual(IsHovered()?1.018:1);}
