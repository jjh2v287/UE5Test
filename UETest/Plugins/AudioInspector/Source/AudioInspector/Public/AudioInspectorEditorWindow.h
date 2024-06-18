// Copyright SweejTech Ltd. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SSweejActiveSoundsWidget;

/**
 * 
 */
class AUDIOINSPECTOR_API SAudioInspectorEditorWindow : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAudioInspectorEditorWindow)
	{}
	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);

	void SetUpdateInterval(float UpdateInterval);

	void RebuildActiveSoundsColumns();

private:

	TSharedPtr< SSweejActiveSoundsWidget > ActiveSoundsWidget;
};
