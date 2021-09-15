function Component()
{
}

Component.prototype.createOperations = function()
{
    component.createOperations();

    if (systemInfo.productType === "windows") {
        component.addOperation("CreateShortcut", "@TargetDir@/EmuDiscer.exe", "@StartMenuDir@/EmuDiscer.lnk", "--open-settings",
            "description=Open EmuDiscer settings");
    }
}

/*
"workingDirectory=@TargetDir@", "iconPath=%TargetDir%/EmuDiscer.exe",
            "iconId=0",
*/