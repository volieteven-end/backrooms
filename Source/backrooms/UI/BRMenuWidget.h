#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Online/BRRoomTypes.h"
#include "BRMenuWidget.generated.h"
class UButton;
class UEditableTextBox;
class UComboBoxString;
class UTextBlock;
class UWidgetSwitcher;
class UVerticalBox;
class UCanvasPanel;
class UFont;
class UBackgroundBlur;

UCLASS()
class BACKROOMS_API UBRMenuWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable) void SetPage(int32 Page);
    UFUNCTION(BlueprintCallable) void ShowStatus(const FString& Message);
    UPROPERTY(EditDefaultsOnly, Category="Style") TObjectPtr<UFont> ChineseFont;
protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeDestruct() override;
    virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UWidgetSwitcher> Pages;
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UBackgroundBlur> PageBlur;
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> PlayButton;
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> BrowseButton;
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> QuitButton;
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> StoryButton;
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> CreateBackButton;
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> BrowseBackButton;
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> RefreshButton;
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> JoinButton;
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> ReadyButton;
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> StartButton;
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> LeaveButton;
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UEditableTextBox> NicknameBox;
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UEditableTextBox> RoomNameBox;
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UComboBoxString> CapacityBox;
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> StatusText;
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> LobbySummary;
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> BrowseSummary;
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> RoomTitle;
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> ReadyLabel;
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UVerticalBox> LobbyPlayers;
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UCanvasPanel> RoomRow;
private:
    UFUNCTION() void OpenPlay();
    UFUNCTION() void OpenBrowse();
    UFUNCTION() void Quit();
    UFUNCTION() void Back();
    UFUNCTION() void Create();
    UFUNCTION() void Refresh();
    UFUNCTION() void Join();
    UFUNCTION() void Ready();
    UFUNCTION() void Start();
    UFUNCTION() void Leave();
    UFUNCTION() void DirectoryChanged(const FBRDirectoryReply& Result);
    UFUNCTION() UWidget* GenerateCapacityItem(FString Item);
    void UpdateLobby();
    void ClickSound();
    FString ReadNickname();
    FGuid DisplayedRoomId;
    FString LastRoster;
    double NextRefresh = 0, NextLobbyUpdate = 0;
};
