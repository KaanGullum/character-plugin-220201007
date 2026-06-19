param(
    [string]$N8roRoot = $(if ($env:N8RO_RELEASE) { $env:N8RO_RELEASE } else { "C:\N8RO" })
)

$ErrorActionPreference = "Stop"

$ProjectRoot = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)

$PluginSource = Join-Path $ProjectRoot "bin\release\student-220201007-motion-plugin.dll"
$ScenarioSource = Join-Path $ProjectRoot "scenarios\Student220201007MotionDemo.json.gz"
$MissionSource = Join-Path $ProjectRoot "missions\student_motion_sequence.lua"

$PluginTargetDir = Join-Path $N8roRoot "userPlugins\sim"
$ScenarioTargetDir = Join-Path $N8roRoot "data\db\AstsimSchema\Profiles\Scenario"
$MissionTargetDir = Join-Path $N8roRoot "data\resources\missions"

$RequiredFiles = @($PluginSource, $ScenarioSource, $MissionSource)
foreach ($File in $RequiredFiles) {
    if (-not (Test-Path -LiteralPath $File)) {
        throw "Required file is missing: $File"
    }
}

New-Item -ItemType Directory -Path $PluginTargetDir -Force | Out-Null
New-Item -ItemType Directory -Path $ScenarioTargetDir -Force | Out-Null
New-Item -ItemType Directory -Path $MissionTargetDir -Force | Out-Null

Copy-Item -LiteralPath $PluginSource -Destination $PluginTargetDir -Force
Copy-Item -LiteralPath $ScenarioSource -Destination $ScenarioTargetDir -Force
Copy-Item -LiteralPath $MissionSource -Destination $MissionTargetDir -Force

Write-Host "Installed student 220201007 animation package to $N8roRoot"
Write-Host "Scenario: Student220201007MotionDemo"
Write-Host "Mission script: student_motion_sequence.lua"
