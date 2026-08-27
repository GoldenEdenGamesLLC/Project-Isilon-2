// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class Project_Isilon_2EditorTarget : TargetRules
{
	public Project_Isilon_2EditorTarget( TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
		ExtraModuleNames.Add("Project_Isilon_2");
		//possibly unnecessary, only meant for .cpp and .h module, currently not in here or implemented
		//ExtraModuleNames.Add("Project_Isilon_2Editor"); //include this for editor target for other tools we might add
	}
}
