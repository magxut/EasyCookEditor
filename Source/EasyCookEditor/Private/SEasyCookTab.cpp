// https://github.com/magxut/EasyCookEditor?tab=GPL-3.0-1-ov-file

#include "SEasyCookTab.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "Modules/ModuleManager.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/MessageDialog.h"
#include "HAL/PlatformProcess.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SExpandableArea.h"

#define LOCTEXT_NAMESPACE "SEasyCookTab"

void SEasyCookTab::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
		SNew(SBorder)
		.Padding(4)
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot()
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot().AutoHeight().Padding(8, 8, 8, 4)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("SelectionSectionHeader", "Asset & Folder Selection"))
					.Font(FCoreStyle::Get().GetFontStyle("ExpandableArea.TitleFont"))
				]

				+ SVerticalBox::Slot().AutoHeight().Padding(4, 0, 4, 8)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().Padding(2)
					[
						SNew(SButton)
						.Text(LOCTEXT("UseSelection", "Add from Content Browser"))
						.ToolTipText(LOCTEXT("UseSelectionTip", "Add currently selected assets and folders from the Content Browser"))
						.OnClicked(this, &SEasyCookTab::OnUseSelectionClicked)
					]
					+ SHorizontalBox::Slot().AutoWidth().Padding(2)
					[
						SNew(SButton)
						.Text(LOCTEXT("ClearAll", "Clear All"))
						.ToolTipText(LOCTEXT("ClearAllTip", "Remove all selected assets and folders"))
						.OnClicked(this, &SEasyCookTab::OnClearSelectionClicked)
					]
				]

				+ SVerticalBox::Slot().AutoHeight().Padding(4, 4, 4, 8)
				[
					SAssignNew(StatusMessageBox, SMultiLineEditableTextBox)
					.IsReadOnly(true)
					.AutoWrapText(true)
					.Font(FCoreStyle::Get().GetFontStyle("SmallFont"))
					.Visibility_Lambda([this]() 
					{ 
						return (StatusMessageBox.IsValid() && !StatusMessageBox->GetText().IsEmpty()) 
							? EVisibility::Visible 
							: EVisibility::Collapsed; 
					})
				]

				+ SVerticalBox::Slot().AutoHeight().Padding(4, 0, 4, 8)
				[
					SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush("ToolPanel.GroupBorder"))
					.Padding(8)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 4)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("SelectedItemsLabel", "Selected Items to Cook"))
							.Font(FCoreStyle::Get().GetFontStyle("NormalFontBold"))
						]
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(SBox)
							.HeightOverride(250)
							[
								SAssignNew(SelectedListView, SListView<TSharedPtr<FEasyCookItem>>)
								.ItemHeight(28)
								.ListItemsSource(&DisplayItems)
								.OnGenerateRow(this, &SEasyCookTab::OnGenerateRowForList)
							]
						]
					]
				]

				+ SVerticalBox::Slot().AutoHeight().Padding(4, 4)
				[
					SNew(SSeparator)
				]

				+ SVerticalBox::Slot().AutoHeight().Padding(8, 8, 8, 4)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("SettingsSectionHeader", "Cook Settings"))
					.Font(FCoreStyle::Get().GetFontStyle("ExpandableArea.TitleFont"))
				]

				+ SVerticalBox::Slot().AutoHeight().Padding(4, 0, 4, 4)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(4, 0, 8, 0)
					[
						SNew(SBox)
						.WidthOverride(120)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("PlatformLabel", "Target Platform:"))
						]
					]
					+ SHorizontalBox::Slot().FillWidth(1).Padding(0)
					[
						SAssignNew(PlatformTextBox, SEditableTextBox)
						.HintText(LOCTEXT("PlatformHint", "e.g., WindowsNoEditor, Win64"))
						.Text(FText::FromString(TEXT("WindowsNoEditor")))
					]
				]

				+ SVerticalBox::Slot().AutoHeight().Padding(4, 0, 4, 4)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(4, 0, 8, 0)
					[
						SNew(SBox)
						.WidthOverride(120)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("CulturesLabel", "Cultures:"))
						]
					]
					+ SHorizontalBox::Slot().FillWidth(1).Padding(0, 0, 4, 0)
					[
						SAssignNew(CulturesTextBox, SEditableTextBox)
						.HintText(LOCTEXT("CulturesHint", "Optional: en;fr;de"))
					]
					+ SHorizontalBox::Slot().AutoWidth().Padding(0)
					[
						SNew(SButton)
						.Text(LOCTEXT("DetectCultures", "Auto-Detect"))
						.ToolTipText(LOCTEXT("DetectCulturesTip", "Auto-detect cultures from project settings"))
						.OnClicked_Lambda([this]()
						{
							DetectCulturesFromProjectSettings();
							RefreshCommandPreview();
							return FReply::Handled();
						})
					]
				]

				+ SVerticalBox::Slot().AutoHeight().Padding(4, 4, 4, 8)
				[
					SNew(SExpandableArea)
					.AreaTitle(LOCTEXT("AdvancedOptions", "Advanced Cook Options"))
					.InitiallyCollapsed(true)
					.Padding(8)
					.BodyContent()
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 2)
						[
							SNew(SWrapBox)
							.UseAllottedSize(true)
							+ SWrapBox::Slot().Padding(4, 2)
							[
								SNew(SCheckBox)
								.OnCheckStateChanged_Lambda([this](ECheckBoxState S){ bIterate = (S==ECheckBoxState::Checked); RefreshCommandPreview(); })
								.IsChecked(ECheckBoxState::Unchecked)
								.ToolTipText(LOCTEXT("IterateTip", "Iterative cooking (faster for repeated cooks)"))
								.Content()
								[
									SNew(STextBlock).Text(LOCTEXT("Iterate", "Iterate"))
								]
							]
							+ SWrapBox::Slot().Padding(4, 2)
							[
								SNew(SCheckBox)
								.OnCheckStateChanged_Lambda([this](ECheckBoxState S){ bUnversioned = (S==ECheckBoxState::Checked); RefreshCommandPreview(); })
								.ToolTipText(LOCTEXT("UnversionedTip", "Create unversioned packages"))
								.Content()
								[
									SNew(STextBlock).Text(LOCTEXT("Unversioned", "Unversioned"))
								]
							]
							+ SWrapBox::Slot().Padding(4, 2)
							[
								SNew(SCheckBox)
								.OnCheckStateChanged_Lambda([this](ECheckBoxState S){ bCompressed = (S==ECheckBoxState::Checked); RefreshCommandPreview(); })
								.ToolTipText(LOCTEXT("CompressedTip", "Compress cooked packages"))
								.Content()
								[
									SNew(STextBlock).Text(LOCTEXT("Compressed", "Compressed"))
								]
							]
							+ SWrapBox::Slot().Padding(4, 2)
							[
								SNew(SCheckBox)
								.OnCheckStateChanged_Lambda([this](ECheckBoxState S){ bNoP4 = (S==ECheckBoxState::Checked); RefreshCommandPreview(); })
								.IsChecked(ECheckBoxState::Checked)
								.ToolTipText(LOCTEXT("NoP4Tip", "Disable Perforce integration"))
								.Content()
								[
									SNew(STextBlock).Text(LOCTEXT("NoP4", "No P4"))
								]
							]
							+ SWrapBox::Slot().Padding(4, 2)
							[
								SNew(SCheckBox)
								.OnCheckStateChanged_Lambda([this](ECheckBoxState S){ bUnattended = (S==ECheckBoxState::Checked); RefreshCommandPreview(); })
								.IsChecked(ECheckBoxState::Checked)
								.ToolTipText(LOCTEXT("UnattendedTip", "Run in unattended mode (no prompts)"))
								.Content()
								[
									SNew(STextBlock).Text(LOCTEXT("Unattended", "Unattended"))
								]
							]
							+ SWrapBox::Slot().Padding(4, 2)
							[
								SNew(SCheckBox)
								.OnCheckStateChanged_Lambda([this](ECheckBoxState S){ bStdOut = (S==ECheckBoxState::Checked); RefreshCommandPreview(); })
								.IsChecked(ECheckBoxState::Checked)
								.ToolTipText(LOCTEXT("StdOutTip", "Output to stdout"))
								.Content()
								[
									SNew(STextBlock).Text(LOCTEXT("StdOut", "Stdout"))
								]
							]
							+ SWrapBox::Slot().Padding(4, 2)
							[
								SNew(SCheckBox)
								.OnCheckStateChanged_Lambda([this](ECheckBoxState S){ bCookSinglePackageNoRefs = (S==ECheckBoxState::Checked); RefreshCommandPreview(); })
								.IsChecked(ECheckBoxState::Checked)
								.ToolTipText(LOCTEXT("CookSinglePackageNoRefsTip", "Cook single package without references"))
								.Content()
								[
									SNew(STextBlock).Text(LOCTEXT("CookSinglePackageNoRefs", "No Refs"))
								]
							]
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 8, 0, 2)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("ExtraFlags", "Extra Command Line Flags:"))
							.Font(FCoreStyle::Get().GetFontStyle("SmallFont"))
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 0)
						[
							SAssignNew(ExtraFlagsTextBox, SEditableTextBox)
							.HintText(LOCTEXT("ExtraFlagsHint", "e.g., -fileopenlog \"-DDC=DerivedDataBackendGraph.txt\""))
							.OnTextChanged_Lambda([this](const FText&){ RefreshCommandPreview(); })
						]
					]
				]

				+ SVerticalBox::Slot().AutoHeight().Padding(4, 4)
				[
					SNew(SSeparator)
				]

				+ SVerticalBox::Slot().AutoHeight().Padding(8, 8, 8, 4)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("ExecuteSectionHeader", "Execute Cook"))
					.Font(FCoreStyle::Get().GetFontStyle("ExpandableArea.TitleFont"))
				]

				+ SVerticalBox::Slot().AutoHeight().Padding(4, 0, 4, 4)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("CommandLabel", "Generated Command Line:"))
					.Font(FCoreStyle::Get().GetFontStyle("SmallFont"))
				]

				+ SVerticalBox::Slot().AutoHeight().Padding(4, 0, 4, 8)
				[
					SAssignNew(CommandPreview, SEditableTextBox)
					.IsReadOnly(true)
					.Font(FCoreStyle::Get().GetFontStyle("SmallFont"))
				]

				+ SVerticalBox::Slot().AutoHeight().Padding(4, 0, 4, 8)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().Padding(2)
					[
						SNew(SButton)
						.Text(LOCTEXT("Copy", "Copy to Clipboard"))
						.OnClicked(this, &SEasyCookTab::OnCopyCommandClicked)
					]
					+ SHorizontalBox::Slot().AutoWidth().Padding(2)
					[
						SNew(SButton)
						.Text(LOCTEXT("RunCook", "Run Cook"))
						.ButtonStyle(FCoreStyle::Get(), "FlatButton.Success")
						.OnClicked(this, &SEasyCookTab::OnRunCookClicked)
						.IsEnabled_Lambda([this](){ return !bCookRunning; })
					]
					+ SHorizontalBox::Slot().AutoWidth().Padding(2)
					[
						SNew(SButton)
						.Text(LOCTEXT("KillCook", "Kill Process"))
						.ButtonStyle(FCoreStyle::Get(), "FlatButton.Danger")
						.OnClicked(this, &SEasyCookTab::OnKillCookClicked)
						.IsEnabled_Lambda([this](){ return bCookRunning; })
					]
					]

				+ SVerticalBox::Slot().AutoHeight().Padding(4, 0, 4, 8)
				[
					SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush("ToolPanel.GroupBorder"))
					.Padding(8)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 4)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("OutputLogLabel", "Output Log"))
							.Font(FCoreStyle::Get().GetFontStyle("NormalFontBold"))
						]
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(SBox)
							.HeightOverride(300)
							[
								SAssignNew(OutputLog, SMultiLineEditableTextBox)
								.IsReadOnly(true)
								.AutoWrapText(false)
								.Font(FCoreStyle::Get().GetFontStyle("SmallFont"))
							]
						]
					]
				]
			]
		]
	];

	RebuildDisplayItems();
	RefreshCommandPreview();
}

FReply SEasyCookTab::OnUseSelectionClicked()
{
	ResolveContentBrowserSelection();
	RebuildDisplayItems();
	RefreshCommandPreview();
	return FReply::Handled();
}

FReply SEasyCookTab::OnClearSelectionClicked()
{
	SelectedAssetPackages.Reset();
	SelectedFolders.Reset();
	RebuildDisplayItems();
	RefreshCommandPreview();
	return FReply::Handled();
}

FReply SEasyCookTab::OnRemoveItemClicked(TSharedPtr<FEasyCookItem> Item)
{
	if (!Item.IsValid())
	{
		return FReply::Handled();
	}

	if (Item->bIsFolder)
	{
		SelectedFolders.Remove(Item->Name);
	}
	else
	{
		SelectedAssetPackages.Remove(Item->Name);
	}

	RebuildDisplayItems();
	RefreshCommandPreview();
	return FReply::Handled();
}

TSharedRef<ITableRow> SEasyCookTab::OnGenerateRowForList(TSharedPtr<FEasyCookItem> InItem, const TSharedRef<STableViewBase>& OwnerTable)
{
	const FText TypeIcon = InItem->bIsFolder ? FText::FromString(TEXT("📁")) : FText::FromString(TEXT("📄"));
	const FText DisplayText = FText::FromString(InItem->DisplayName);
	
	return SNew(STableRow<TSharedPtr<FEasyCookItem>>, OwnerTable)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(4, 0, 8, 0)
		[
			SNew(STextBlock)
			.Text(TypeIcon)
			.Font(FCoreStyle::Get().GetFontStyle("NormalFont"))
		]
		+ SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(DisplayText)
			.ToolTipText(FText::FromString(InItem->Name))
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(4, 0)
		[
			SNew(SButton)
			.ButtonStyle(FCoreStyle::Get(), "NoBorder")
			.ContentPadding(FMargin(4, 2))
			.Text(FText::FromString(TEXT("✖")))
			.ToolTipText(LOCTEXT("RemoveItemTip", "Remove this item"))
			.OnClicked(this, &SEasyCookTab::OnRemoveItemClicked, InItem)
		]
	];
}

FString SEasyCookTab::QuoteIfNeeded(const FString& In)
{
	if (In.IsEmpty()) return In;
	const bool bNeedsQuote = In.Contains(TEXT(" ")) || In.Contains(TEXT("\t")) || In.Contains(TEXT("\""));
	if (!bNeedsQuote)
	{
		return In;
	}
	FString Escaped = In;
	Escaped.ReplaceInline(TEXT("\""), TEXT("\\\""));
	return FString::Printf(TEXT("\"%s\""), *Escaped);
}

void SEasyCookTab::TokenizeRespectingQuotes(const FString& In, TArray<FString>& OutTokens)
{
	OutTokens.Reset();
	bool bInQuote = false;
	FString Current;
	for (int32 i = 0; i < In.Len(); ++i)
	{
		const TCHAR C = In[i];
		if (C == '"')
		{
			bInQuote = !bInQuote;
			continue;
		}
		if (!bInQuote && FChar::IsWhitespace(C))
		{
			if (!Current.IsEmpty())
			{
				OutTokens.Add(Current);
				Current.Reset();
			}
		}
		else
		{
			Current.AppendChar(C);
		}
	}
	if (!Current.IsEmpty())
	{
		OutTokens.Add(Current);
	}
}

void SEasyCookTab::ResolveFolderRecursive(const FString& InPath, TSet<FString>& OutPackageNames) const
{
	FAssetRegistryModule& ARM = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	FARFilter Filter;
	Filter.PackagePaths.Add(*InPath);
	Filter.bRecursivePaths = true;
	Filter.bIncludeOnlyOnDiskAssets = false;
	TArray<FAssetData> Found;
	ARM.Get().GetAssets(Filter, Found);
	for (const FAssetData& AD : Found)
	{
		OutPackageNames.Add(AD.PackageName.ToString());
	}
}

bool SEasyCookTab::IsPathChildOfExistingFolder(const FString& Path) const
{
	FString NormalizedPath = Path;
	if (!NormalizedPath.EndsWith(TEXT("/")))
	{
		NormalizedPath += TEXT("/");
	}

	for (const FString& ExistingFolder : SelectedFolders)
	{
		FString NormalizedExisting = ExistingFolder;
		if (!NormalizedExisting.EndsWith(TEXT("/")))
		{
			NormalizedExisting += TEXT("/");
		}

		if (NormalizedPath.StartsWith(NormalizedExisting) && NormalizedPath != NormalizedExisting)
		{
			return true;
		}
	}

	return false;
}

bool SEasyCookTab::IsAssetInExistingFolder(const FString& AssetPackagePath) const
{
	FString AssetFolder = AssetPackagePath;
	int32 LastSlashIndex;
	if (AssetFolder.FindLastChar(TEXT('/'), LastSlashIndex))
	{
		AssetFolder = AssetFolder.Left(LastSlashIndex);
	}

	for (const FString& ExistingFolder : SelectedFolders)
	{
		FString NormalizedExisting = ExistingFolder;
		if (!NormalizedExisting.EndsWith(TEXT("/")))
		{
			NormalizedExisting += TEXT("/");
		}

		FString NormalizedAssetFolder = AssetFolder;
		if (!NormalizedAssetFolder.EndsWith(TEXT("/")))
		{
			NormalizedAssetFolder += TEXT("/");
		}

		if (NormalizedAssetFolder.StartsWith(NormalizedExisting))
		{
			return true;
		}
	}

	return false;
}

void SEasyCookTab::SetStatusMessage(const FString& Message)
{
	if (StatusMessageBox.IsValid())
	{
		StatusMessageBox->SetText(FText::FromString(Message));
		
		if (!Message.IsEmpty())
		{
			RegisterActiveTimer(3.0f, FWidgetActiveTimerDelegate::CreateSP(this, &SEasyCookTab::ClearStatusMessageAfterDelay));
		}
	}
}

EActiveTimerReturnType SEasyCookTab::ClearStatusMessageAfterDelay(double InCurrentTime, float InDeltaTime)
{
	if (StatusMessageBox.IsValid())
	{
		StatusMessageBox->SetText(FText());
	}
	return EActiveTimerReturnType::Stop;
}

void SEasyCookTab::RebuildDisplayItems()
{
	DisplayItems.Reset();
	
	auto GetDisplayName = [](const FString& FullPath, bool bIsFolder) -> FString
	{
		FString DisplayName = FullPath;
		
		if (bIsFolder)
		{
			DisplayName.RemoveFromEnd(TEXT("/"));
			int32 LastSlashIndex;
			if (DisplayName.FindLastChar(TEXT('/'), LastSlashIndex))
			{
				DisplayName = DisplayName.RightChop(LastSlashIndex + 1);
			}
		}
		else
		{
			int32 LastSlashIndex;
			if (DisplayName.FindLastChar(TEXT('/'), LastSlashIndex))
			{
				DisplayName = DisplayName.RightChop(LastSlashIndex + 1);
			}
		}
		
		return DisplayName;
	};
	
	TArray<FString> FoldersArray = SelectedFolders.Array();
	FoldersArray.Sort();
	for (const FString& Path : FoldersArray)
	{
		TSharedPtr<FEasyCookItem> Item = MakeShared<FEasyCookItem>();
		Item->bIsFolder = true;
		Item->Name = Path;
		Item->DisplayName = GetDisplayName(Path, true);
		DisplayItems.Add(Item);
	}

	TArray<FString> AssetsArray = SelectedAssetPackages.Array();
	AssetsArray.Sort();
	for (const FString& Pkg : AssetsArray)
	{
		TSharedPtr<FEasyCookItem> Item = MakeShared<FEasyCookItem>();
		Item->bIsFolder = false;
		Item->Name = Pkg;
		Item->DisplayName = GetDisplayName(Pkg, false);
		DisplayItems.Add(Item);
	}

	if (SelectedListView.IsValid())
	{
		SelectedListView->RequestListRefresh();
	}
}

void SEasyCookTab::ResolveContentBrowserSelection()
{
	FContentBrowserModule& CBM = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	TArray<FAssetData> Assets;
	CBM.Get().GetSelectedAssets(Assets);
	TArray<FString> Paths;
	CBM.Get().GetSelectedFolders(Paths);
	
	if (StatusMessageBox.IsValid())
	{
		StatusMessageBox->SetText(FText());
	}

	TArray<FString> SkippedItems;
	int32 AddedCount = 0;
	
	for (const FString& P : Paths)
	{
		if (SelectedFolders.Contains(P))
		{
			SkippedItems.Add(FString::Printf(TEXT("Folder '%s' is already selected"), *P));
			continue;
		}
		
		if (IsPathChildOfExistingFolder(P))
		{

			FString ParentFolder;
			FString NormalizedP = P;
			if (!NormalizedP.EndsWith(TEXT("/")))
			{
				NormalizedP += TEXT("/");
			}

			for (const FString& ExistingFolder : SelectedFolders)
			{
				FString NormalizedExisting = ExistingFolder;
				if (!NormalizedExisting.EndsWith(TEXT("/")))
				{
					NormalizedExisting += TEXT("/");
				}

				if (NormalizedP.StartsWith(NormalizedExisting))
				{
					ParentFolder = ExistingFolder;
					break;
				}
			}

			SkippedItems.Add(FString::Printf(TEXT("Folder '%s' is already covered by parent folder '%s'"), *P, *ParentFolder));
			continue;
		}
		
		bool bHasChildFolders = false;
		TArray<FString> ChildFolders;
		FString NormalizedP = P;
		if (!NormalizedP.EndsWith(TEXT("/")))
		{
			NormalizedP += TEXT("/");
		}

		for (const FString& ExistingFolder : SelectedFolders)
		{
			FString NormalizedExisting = ExistingFolder;
			if (!NormalizedExisting.EndsWith(TEXT("/")))
			{
				NormalizedExisting += TEXT("/");
			}

			if (NormalizedExisting.StartsWith(NormalizedP) && NormalizedExisting != NormalizedP)
			{
				bHasChildFolders = true;
				ChildFolders.Add(ExistingFolder);
			}
		}
		
		if (bHasChildFolders)
		{
			for (const FString& ChildFolder : ChildFolders)
			{
				SelectedFolders.Remove(ChildFolder);
			}
		}
		
		SelectedFolders.Add(P);
		AddedCount++;
	}
	
	for (const FAssetData& AD : Assets)
	{
		const FString PackageName = AD.PackageName.ToString();
		
		if (SelectedAssetPackages.Contains(PackageName))
		{
			SkippedItems.Add(FString::Printf(TEXT("Asset '%s' is already selected"), *PackageName));
			continue;
		}
		
		if (IsAssetInExistingFolder(PackageName))
		{
			FString ParentFolder;
			FString AssetFolder = PackageName;
			int32 LastSlashIndex;
			if (AssetFolder.FindLastChar(TEXT('/'), LastSlashIndex))
			{
				AssetFolder = AssetFolder.Left(LastSlashIndex);
			}

			for (const FString& ExistingFolder : SelectedFolders)
			{
				FString NormalizedExisting = ExistingFolder;
				if (!NormalizedExisting.EndsWith(TEXT("/")))
				{
					NormalizedExisting += TEXT("/");
				}

				FString NormalizedAssetFolder = AssetFolder;
				if (!NormalizedAssetFolder.EndsWith(TEXT("/")))
				{
					NormalizedAssetFolder += TEXT("/");
				}

				if (NormalizedAssetFolder.StartsWith(NormalizedExisting))
				{
					ParentFolder = ExistingFolder;
					break;
				}
			}

			SkippedItems.Add(FString::Printf(TEXT("Asset '%s' is already covered by folder '%s'"), *PackageName, *ParentFolder));
			continue;
		}
		
		SelectedAssetPackages.Add(PackageName);
		AddedCount++;
	}
	
	if (SkippedItems.Num() > 0)
	{
		FString StatusMessage = FString::Printf(TEXT("⚠ Added %d item(s), skipped %d duplicate(s):\n\n"), AddedCount, SkippedItems.Num());
		
		const int32 MaxDisplayItems = 10;
		for (int32 i = 0; i < FMath::Min(SkippedItems.Num(), MaxDisplayItems); ++i)
		{
			StatusMessage += TEXT("• ") + SkippedItems[i] + TEXT("\n");
		}
		
		if (SkippedItems.Num() > MaxDisplayItems)
		{
			StatusMessage += FString::Printf(TEXT("... and %d more item(s)\n"), SkippedItems.Num() - MaxDisplayItems);
		}

		SetStatusMessage(StatusMessage);
	}
	else if (AddedCount > 0)
	{
		SetStatusMessage(FString::Printf(TEXT("✓ Successfully added %d item(s)"), AddedCount));
	}
}

void SEasyCookTab::DetectCulturesFromProjectSettings()
{
	TArray<FString> ProjectCultures;
	const FString IniPath = FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("DefaultGame.ini"));
	TArray<FString> Values;
	if (GConfig->GetArray(TEXT("/Script/EngineSettings.GeneralProjectSettings"), TEXT("CulturesToCook"), Values, IniPath))
	{
		for (const FString& V : Values) { ProjectCultures.Add(V); }
	}
	if (ProjectCultures.Num() == 0)
	{
		if (GConfig->GetArray(TEXT("/Script/UnrealEd.ProjectPackagingSettings"), TEXT("CulturesToStage"), Values, IniPath))
		{
			for (const FString& V : Values) { ProjectCultures.Add(V); }
		}
	}
	if (ProjectCultures.Num() > 0 && CulturesTextBox.IsValid())
	{
		CulturesTextBox->SetText(FText::FromString(FString::Join(ProjectCultures, TEXT(";"))));
	}
}

FString SEasyCookTab::BuildArgsOnlyString() const
{
	TArray<FString> Tokens;
	const FString ProjectFile = FPaths::ConvertRelativePathToFull(FPaths::GetProjectFilePath());
	Tokens.Add(QuoteIfNeeded(ProjectFile));
	Tokens.Add(TEXT("-run=cook"));

	const FString Platform = PlatformTextBox.IsValid() ? PlatformTextBox->GetText().ToString().TrimStartAndEnd() : TEXT("");
	if (!Platform.IsEmpty())
	{
		Tokens.Add(FString::Printf(TEXT("-targetplatform=%s"), *Platform));
	}
	Tokens.Add(TEXT("-cooksinglepackage"));

	TSet<FString> Pkgs;
	for (const FString& A : SelectedAssetPackages) { Pkgs.Add(A); }
	for (const FString& Path : SelectedFolders) { ResolveFolderRecursive(Path, Pkgs); }

	TArray<FString> SortedPkgs = Pkgs.Array();
	SortedPkgs.Sort();
	for (const FString& Pkg : SortedPkgs)
	{
		Tokens.Add(FString::Printf(TEXT("-map=%s"), *Pkg));
	}

	const FString Cultures = CulturesTextBox.IsValid() ? CulturesTextBox->GetText().ToString().TrimStartAndEnd() : TEXT("");
	if (!Cultures.IsEmpty())
	{
		Tokens.Add(FString::Printf(TEXT("-cookcultures=%s"), *Cultures));
	}

	if (bIterate) Tokens.Add(TEXT("-iterate"));
	if (bUnversioned) Tokens.Add(TEXT("-unversioned"));
	if (bCompressed) Tokens.Add(TEXT("-compressed"));
	if (bNoP4) Tokens.Add(TEXT("-nop4"));
	if (bUnattended) Tokens.Add(TEXT("-unattended"));
	if (bStdOut) Tokens.Add(TEXT("-stdout"));
	if (bCookSinglePackageNoRefs) Tokens.Add(TEXT("-cooksinglepackagenorefs"));

	if (ExtraFlagsTextBox.IsValid())
	{
		const FString Extra = ExtraFlagsTextBox->GetText().ToString();
		TArray<FString> ExtraTokens;
		TokenizeRespectingQuotes(Extra, ExtraTokens);
		for (FString& T : ExtraTokens)
		{
			Tokens.Add(QuoteIfNeeded(T));
		}
	}

	return FString::Join(Tokens, TEXT(" "));
}

FString SEasyCookTab::BuildFullCommandLineString() const
{
	const FString EditorExe = FPlatformProcess::ExecutablePath();
	return FString::Printf(TEXT("%s %s"), *QuoteIfNeeded(EditorExe), *BuildArgsOnlyString());
}

void SEasyCookTab::RefreshCommandPreview()
{
	if (CommandPreview.IsValid())
	{
		CommandPreview->SetText(FText::FromString(BuildFullCommandLineString()));
	}
}

FReply SEasyCookTab::OnCopyCommandClicked()
{
	const FString Cmd = BuildFullCommandLineString();
	FPlatformApplicationMisc::ClipboardCopy(*Cmd);
	return FReply::Handled();
}

void SEasyCookTab::BeginReadOutput()
{
	if (!bCookRunning) return;
	RegisterActiveTimer(0.1f, FWidgetActiveTimerDelegate::CreateSP(this, &SEasyCookTab::OnActiveTimerTick));
}

EActiveTimerReturnType SEasyCookTab::OnActiveTimerTick(double InCurrentTime, float InDeltaTime)
{
	if (!bCookRunning)
	{
		return EActiveTimerReturnType::Stop;
	}
	if (ReadPipe)
	{
		FString NewOutput = FPlatformProcess::ReadPipe(ReadPipe);
		if (!NewOutput.IsEmpty())
		{
			OutputBuffer += NewOutput;
			if (OutputLog.IsValid())
			{
				OutputLog->SetText(FText::FromString(OutputBuffer));
			}
		}
	}
	if (!FPlatformProcess::IsProcRunning(ProcHandle))
	{
		int32 ReturnCode = 0;
		FPlatformProcess::GetProcReturnCode(ProcHandle, &ReturnCode);
		bCookRunning = false;
		ProcHandle.Reset();
		if (WritePipe) { FPlatformProcess::ClosePipe(ReadPipe, WritePipe); ReadPipe=nullptr; WritePipe=nullptr; }
		OutputBuffer += FString::Printf(TEXT("\n[Cook finished] ExitCode=%d\n"), ReturnCode);
		if (OutputLog.IsValid())
		{
			OutputLog->SetText(FText::FromString(OutputBuffer));
		}
		RefreshCommandPreview();
		return EActiveTimerReturnType::Stop;
	}
	return EActiveTimerReturnType::Continue;
}

void SEasyCookTab::StopProcess()
{
	if (bCookRunning)
	{
		FPlatformProcess::TerminateProc(ProcHandle, true);
		bCookRunning = false;
	}
	if (ProcHandle.IsValid())
	{
		ProcHandle.Reset();
	}
	if (WritePipe)
	{
		FPlatformProcess::ClosePipe(ReadPipe, WritePipe);
		ReadPipe=nullptr; WritePipe=nullptr;
	}
}

FReply SEasyCookTab::OnRunCookClicked()
{
	if (bCookRunning)
	{
		return FReply::Handled();
	}
	OutputBuffer.Reset();
	if (OutputLog.IsValid())
	{
		OutputLog->SetText(FText());
	}
	if (SelectedAssetPackages.Num() == 0 && SelectedFolders.Num() == 0)
	{
		OutputBuffer = TEXT("No assets or folders selected. Use the pickers or click 'Use Content Browser Selection' first.\n");
		if (OutputLog.IsValid())
		{
			OutputLog->SetText(FText::FromString(OutputBuffer));
		}
		return FReply::Handled();
	}
	const FString EditorExe = FPlatformProcess::ExecutablePath();
	const FString Params = BuildArgsOnlyString();
	const FString FullCommand = FString::Printf(TEXT("%s %s"), *QuoteIfNeeded(EditorExe), *Params);
	const int32 CmdLimit = 8191;
	const int32 HardLimit = 32000;

	bool bRunViaPowerShell = false;

	if (FullCommand.Len() > HardLimit)
	{
		FText Message = FText::Format(LOCTEXT("CommandTooLong", "The generated command line is too long ({0} characters). The limit is 32,000 characters.\n\nPlease reduce the number of selected assets or folders."), FText::AsNumber(FullCommand.Len()));
		FMessageDialog::Open(EAppMsgType::Ok, Message);
		
		OutputBuffer = TEXT("Command line too long. Cook aborted.\n");
		if (OutputLog.IsValid())
		{
			OutputLog->SetText(FText::FromString(OutputBuffer));
		}
		return FReply::Handled();
	}
	else if (FullCommand.Len() > CmdLimit)
	{
		FText Message = FText::Format(LOCTEXT("CommandLongWarning", "The generated command line is long ({0} characters) and exceeds the standard command prompt limit (8191).\n\nDo you want to run it via a temporary PowerShell script to avoid truncation?"), FText::AsNumber(FullCommand.Len()));
		EAppReturnType::Type Result = FMessageDialog::Open(EAppMsgType::YesNo, Message);
		
		if (Result == EAppReturnType::Yes)
		{
			bRunViaPowerShell = true;
		}
	}

	ReadPipe = nullptr; WritePipe = nullptr;
	FPlatformProcess::CreatePipe(ReadPipe, WritePipe);

	if (bRunViaPowerShell)
	{
		FString ScriptContent = FString::Printf(TEXT("& \"%s\" %s | Out-Host"), *EditorExe, *Params);
		FString TempScriptPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("TempCookScript.ps1"));
		
		if (FFileHelper::SaveStringToFile(ScriptContent, *TempScriptPath))
		{
			FString PSParams = FString::Printf(TEXT("-ExecutionPolicy Bypass -File \"%s\""), *TempScriptPath);
			ProcHandle = FPlatformProcess::CreateProc(TEXT("powershell.exe"), *PSParams, false, true, true, nullptr, 0, nullptr, WritePipe);
		}
		else
		{
			OutputBuffer = TEXT("Failed to save temp PowerShell script.\n");
			if (OutputLog.IsValid())
			{
				OutputLog->SetText(FText::FromString(OutputBuffer));
			}
			return FReply::Handled();
		}
	}
	else
	{
		ProcHandle = FPlatformProcess::CreateProc(*EditorExe, *Params, false, true, true, nullptr, 0, nullptr, WritePipe);
	}

	bCookRunning = ProcHandle.IsValid();
	if (!bCookRunning)
	{
		OutputBuffer = TEXT("Failed to start cook process.\n");
		if (OutputLog.IsValid())
		{
			OutputLog->SetText(FText::FromString(OutputBuffer));
		}
	}
	else
	{
		BeginReadOutput();
	}
	RefreshCommandPreview();
	return FReply::Handled();
}

FReply SEasyCookTab::OnKillCookClicked()
{
	StopProcess();
	RefreshCommandPreview();
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
