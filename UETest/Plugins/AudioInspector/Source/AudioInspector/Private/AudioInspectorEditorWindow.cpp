// Copyright SweejTech Ltd. All Rights Reserved.


#include "AudioInspectorEditorWindow.h"

#include "SSweejActiveSoundsWidget.h"

/** Constructs this widget with InArgs */
void SAudioInspectorEditorWindow::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
		SAssignNew(ActiveSoundsWidget, SSweejActiveSoundsWidget)
	];
}

void SAudioInspectorEditorWindow::SetUpdateInterval(float UpdateInterval)
{
	if (ActiveSoundsWidget)
	{
		ActiveSoundsWidget->SetTickInterval(UpdateInterval);
	}
}

void SAudioInspectorEditorWindow::RebuildActiveSoundsColumns()
{
	if (ActiveSoundsWidget)
	{
		ActiveSoundsWidget->RefreshColumnVisibility();
	}
}
