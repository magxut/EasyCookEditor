// https://github.com/magxut/EasyCookEditor?tab=GPL-3.0-1-ov-file

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SEditableTextBox;
class SMultiLineEditableTextBox;

template<typename ItemType> class SListView;

struct FEasyCookItem
{
	bool bIsFolder = false;
	FString Name; 
	FString DisplayName;
};

class SEasyCookTab : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SEasyCookTab){}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	FReply OnUseSelectionClicked();
	FReply OnClearSelectionClicked();
	FReply OnCopyCommandClicked();
	FReply OnRunCookClicked();
	FReply OnKillCookClicked();
	FReply OnRemoveItemClicked(TSharedPtr<FEasyCookItem> Item);

	TSharedRef<ITableRow> OnGenerateRowForList(TSharedPtr<FEasyCookItem> InItem, const TSharedRef<STableViewBase>& OwnerTable);
	
	void RefreshCommandPreview();
	void RebuildDisplayItems();
	void ResolveContentBrowserSelection();
	void ResolveFolderRecursive(const FString& InPath, TSet<FString>& OutPackageNames) const;
	bool IsPathChildOfExistingFolder(const FString& Path) const;
	bool IsAssetInExistingFolder(const FString& AssetPackagePath) const;
	void SetStatusMessage(const FString& Message);
	EActiveTimerReturnType ClearStatusMessageAfterDelay(double InCurrentTime, float InDeltaTime);
	FString BuildArgsOnlyString() const; 
	FString BuildFullCommandLineString() const; 
	static FString QuoteIfNeeded(const FString& In);
	static void TokenizeRespectingQuotes(const FString& In, TArray<FString>& OutTokens);
	void DetectCulturesFromProjectSettings();
	
	void BeginReadOutput();
	EActiveTimerReturnType OnActiveTimerTick(double InCurrentTime, float InDeltaTime);
	void StopProcess();

private:
	
	TSet<FString> SelectedAssetPackages;
	TSet<FString> SelectedFolders; 

	TArray<TSharedPtr<FEasyCookItem>> DisplayItems;
	TSharedPtr<SListView<TSharedPtr<FEasyCookItem>>> SelectedListView;

	TSharedPtr<SEditableTextBox> PlatformTextBox;
	TSharedPtr<SEditableTextBox> CulturesTextBox; 
	TSharedPtr<SEditableTextBox> ExtraFlagsTextBox;
	TSharedPtr<SMultiLineEditableTextBox> OutputLog;
	TSharedPtr<SEditableTextBox> CommandPreview;
	TSharedPtr<SMultiLineEditableTextBox> StatusMessageBox;

	bool bIterate = false;
	bool bUnversioned = false;
	bool bCompressed = false;
	bool bNoP4 = true;
	bool bUnattended = true;
	bool bStdOut = true;
	bool bCookSinglePackageNoRefs = true;
	
	FProcHandle ProcHandle;
	void* ReadPipe = nullptr;
	void* WritePipe = nullptr;
	bool bCookRunning = false;
	FString OutputBuffer;
};
